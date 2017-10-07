
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <fts.h>
#include <iostream>
#include <map>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"
#include "decompressor.h"
#include "log.h"
#include "repository.h"
#include "util.h"

namespace micrantha
{
    namespace prep
    {
        const char *const Repository::get_local_repo()
        {
            char buf[BUFSIZ] = { 0 };

            if (!getcwd(buf, BUFSIZ)) {
                log::error("no current directory");
                return nullptr;
            }

            const char *prefix       = buf;
            const char *current_path = build_sys_path(prefix, LOCAL_REPO_NAME, NULL);

            while (directory_exists(current_path) == 1) {
                prefix = build_sys_path(prefix, "..", NULL);
                if (directory_exists(prefix) == 1) {
                    break;
                }
                current_path = build_sys_path(prefix, LOCAL_REPO_NAME, NULL);
            }

            return current_path;
        }

        int Repository::initialize(const options &opts)
        {
            if (opts.global) {
                path_ = GLOBAL_REPO;
            } else {
                path_ = get_local_repo();
            }

            return PREP_SUCCESS;
        }

        int Repository::validate(const Options &opts) const
        {
            if (path_.empty()) {
                return PREP_FAILURE;
            }

            if (!directory_exists(path_.c_str())) {
                std::string buf;

                // TODO: save no result, add an non-interactive flag
                printf("Create local repository %s? (Y/n) ", path_.c_str());

                std::getline(std::cin, buf);

                if (!buf.empty() && buf[0] != 10 && toupper(buf[0]) != 'Y') {
                    return PREP_FAILURE;
                }

                auto lib = build_sys_path(path_.c_str(), "lib", nullptr);

                mkpath(lib, 0700);
            }

            char *temp          = getenv("PATH");
            std::string binPath = build_sys_path(path_.c_str(), BIN_FOLDER, NULL);

            if (temp != nullptr) {
                char buf[BUFSIZ] = { 0 };

                strncpy(buf, temp, BUFSIZ);

                temp = strtok(buf, ":");

                while (temp != nullptr) {
                    if (binPath == temp) {
                        break;
                    }

                    temp = strtok(nullptr, ":");
                }
            }

            if (temp == nullptr && opts.verbose) {
                log::warn(binPath, " is not added to your PATH");
            }

            return PREP_SUCCESS;
        }

        int Repository::validate_plugins(const options &opts) const
        {
            const std::string pluginPath = get_plugin_path();

            if (!directory_exists(pluginPath.c_str())) {
                if (mkpath(pluginPath.c_str(), 0777)) {
                    log::perror(errno);
                    return PREP_FAILURE;
                }

                if (init_plugins(opts, pluginPath) == PREP_FAILURE) {
                    return PREP_FAILURE;
                }
            }

            return PREP_SUCCESS;
        }

        int Repository::init_plugins(const options &opts, const std::string &path) const
        {
            decompressor unzip(default_plugins_zip, default_plugins_size, path);

            return unzip.decompress();
        }

        std::string Repository::get_bin_path() const
        {
            return build_sys_path(path_.c_str(), BIN_FOLDER, NULL);
        }

        std::string Repository::get_plugin_path() const
        {
            return build_sys_path(path_.c_str(), PLUGIN_FOLDER, NULL);
        }

        std::string Repository::get_build_path(const std::string &package_name) const
        {
            return build_sys_path(path_.c_str(), KITCHEN_FOLDER, BUILD_FOLDER, package_name.c_str(), NULL);
        }

        std::string Repository::get_install_path(const std::string &package_name) const
        {
            return build_sys_path(path_.c_str(), KITCHEN_FOLDER, INSTALL_FOLDER, package_name.c_str(), NULL);
        }

        std::string Repository::get_meta_path(const std::string &package_name) const
        {
            return build_sys_path(path_.c_str(), KITCHEN_FOLDER, META_FOLDER, package_name.c_str(), NULL);
        }

        int Repository::save_meta(const Package &config) const
        {
            if (path_.empty()) {
                log::error("uninitialized repository path");
                return PREP_FAILURE;
            }

            if (!config.has_name()) {
                log::error("configuration has no name");
                return PREP_FAILURE;
            }

            std::string metaDir =
                build_sys_path(path_.c_str(), KITCHEN_FOLDER, META_FOLDER, config.name().c_str(), NULL);

            if (!directory_exists(metaDir.c_str())) {
                if (mkpath(metaDir.c_str(), 0777)) {
                    log::perror(errno);
                    return PREP_FAILURE;
                }
            }

            std::ofstream out(build_sys_path(metaDir.c_str(), VERSION_FILE, NULL));

            if (!out.is_open()) {
                log::error("unable to save version for ", config.name());
                return PREP_FAILURE;
            }

            if (!config.version().empty()) {
                out << config.version() << std::endl;
            } else if (!config.location().empty()) {
                out << config.location() << std::endl;
            }

            out.close();

            if (config.has_path()) {
                log::trace("copying ", config.path(), " to ", metaDir, "...");

                if (copy_file(config.path(), build_sys_path(metaDir.c_str(), PACKAGE_FILE, NULL))) {
                    log::error("no unable to copy package file ", config.path());
                }
            }

            return PREP_SUCCESS;
        }

        int Repository::has_meta(const Package &config) const
        {
            std::string metaDir =
                build_sys_path(path_.c_str(), KITCHEN_FOLDER, META_FOLDER, config.name().c_str(), NULL);

            if (!directory_exists(metaDir.c_str())) {
                return PREP_FAILURE;
            }

            std::ifstream in(build_sys_path(metaDir.c_str(), VERSION_FILE, NULL));

            std::string info;
            in >> info;
            in.close();

            if (!config.version().empty()) {
                if (strcmp(info.c_str(), config.version().c_str()) >= 0) {
                    return PREP_SUCCESS;
                }
            } else if (!config.location().empty()) {
                if (!strcmp(info.c_str(), config.location().c_str())) {
                    return PREP_SUCCESS;
                }
            }

            return PREP_FAILURE;
        }

        int Repository::dependency_count(const std::string &package_name, const Options &opts) const
        {
            DIR *dir;
            struct dirent *d_ent;
            char buf[PATH_MAX + 1] = { 0 };
            struct stat st         = {};
            int count              = 0;
            std::string metaDir;

            if (path_.empty()) {
                log::error("No repository path defined!");
                return PREP_FAILURE;
            }

            metaDir = build_sys_path(path_.c_str(), KITCHEN_FOLDER, META_FOLDER, NULL);

            if (!directory_exists(metaDir.c_str())) {
                log::error(metaDir, " is not a directory");
                return 0;
            }

            dir = opendir(metaDir.c_str());

            if (dir == nullptr) {
                log::error("unable to open file system [", strerror(errno), "]" );
                return 0;
            }

            while ((d_ent = readdir(dir)) != nullptr) {
                if (d_ent->d_name[0] == '.') {
                    continue;
                }

                if (!strcmp(d_ent->d_name, package_name.c_str())) {
                    continue;
                }

                // get the repo path
                snprintf(buf, PATH_MAX, "%s/%s", metaDir.c_str(), d_ent->d_name);

                // and check if it already exists
                if (stat(buf, &st)) {
                    log::debug(buf, " doesn't exist??, skipping ", strerror(errno));
                    continue;
                }

                if (!S_ISDIR(st.st_mode)) {
                    log::debug("Not a directory [", buf, "]");
                    continue;
                }

                PackageConfig temp;

                if (temp.load(buf, opts) == PREP_SUCCESS) {
                    count += temp.dependency_count(package_name);
                }
            }

            return count;
        }

        int Repository::link_directory(const std::string &path) const
        {
            FTS *file_system       = nullptr;
            FTSENT *parent         = nullptr;
            int rval               = PREP_SUCCESS;
            char buf[PATH_MAX + 1] = { 0 };

            if (!directory_exists(path.c_str())) {
                log::error(path, " is not a directory");
                return PREP_FAILURE;
            }

            char *const paths[] = { (char *const)path.c_str(), nullptr };

            file_system = fts_open(paths, FTS_COMFOLLOW | FTS_NOCHDIR, nullptr);

            if (file_system == nullptr) {
                log::error("unable to open file system [", strerror(errno), "]");
                return PREP_FAILURE;
            }

            while ((parent = fts_read(file_system)) != nullptr) {
                struct stat st = {};

                // get the repo path
                snprintf(buf, PATH_MAX, "%s%s", path_.c_str(), parent->fts_path + path.length());

                // check the install file is a directory
                if (parent->fts_info == FTS_D) {
                    if (stat(buf, &st) == 0) {
                        if (S_ISDIR(st.st_mode)) {
                            log::trace("directory ", buf, " already exists");
                        } else {
                            log::error("non-directory file already found for ", buf);
                            rval = PREP_FAILURE;
                        }
                        continue;
                    }

                    // doesn't exist, create the repo path directory
                    if (mkdir(buf, 0777)) {
                        log::error("Could not create ", buf, " [", strerror(errno), "]");
                        rval = PREP_FAILURE;
                        break;
                    }

                    continue;
                }

                if (parent->fts_info != FTS_F) {
                    log::debug("skipping non-regular file ", buf);
                    continue;
                }

                if (lstat(buf, &st) == 0) {
                    if (!S_ISLNK(st.st_mode)) {
                        log::error("File already exists and is not a link");
                        rval = PREP_FAILURE;
                        break;
                    }

                    if (unlink(buf)) {
                        log::error("unable to unlink existing [", strerror(errno), "]");
                        continue;
                    }
                }

                log::debug("linking [", parent->fts_path, "] to [", buf, "]");

                if (symlink(parent->fts_path, buf)) {
                    log::error("unable to link [", strerror(errno), "]");
                    rval = PREP_FAILURE;
                    break;
                }
            }

            fts_close(file_system);

            return rval;
        }

        int Repository::unlink_directory(const std::string &path) const
        {
            FTS *file_system       = nullptr;
            FTSENT *parent         = nullptr;
            int rval               = PREP_SUCCESS;
            char buf[PATH_MAX + 1] = { 0 };
            std::string installDir;

            if (!directory_exists(path.c_str())) {
                log::error(path, " does not exist.");
                return PREP_FAILURE;
            }

            char *const paths[] = { (char *const)path.c_str(), nullptr };

            file_system = fts_open(paths, FTS_COMFOLLOW | FTS_NOCHDIR, nullptr);

            if (file_system == nullptr) {
                log::error("unable to open file system [", strerror(errno), "]");
                return PREP_FAILURE;
            }

            size_t plength = path.length();

            while ((parent = fts_read(file_system)) != nullptr) {
                struct stat st = {};

                if (parent->fts_info == FTS_D) {
                    log::debug("skipping directory ", parent->fts_path);
                    continue;
                }

                // get the repo path
                snprintf(buf, PATH_MAX, "%s%s", path_.c_str(), parent->fts_path + plength);

                if (lstat(buf, &st)) {
                    log::debug(buf, " not found (", strerror(errno), "), skipping");
                    continue;
                }

                if (S_ISDIR(st.st_mode)) {
                    log::debug("skipping directory ", buf);
                    continue;
                }

                if (!S_ISLNK(st.st_mode)) {
                    log::error(buf, " is not a link, skipping");
                    rval = PREP_FAILURE;
                    continue;
                }

                log::debug("unlinking [", buf, "]");

                if (unlink(buf)) {
                    log::error("unable to unlink [", strerror(errno), "]");
                    rval = PREP_FAILURE;
                    break;
                }
            }

            fts_close(file_system);

            return rval;
        }

        int Repository::execute(const std::string &executable, int argc, char *const *argv) const
        {
            if (executable.empty()) {
                return PREP_FAILURE;
            }

            const auto **args = (const char **)calloc(argc + 2U, sizeof(const char *));

            args[0] = build_sys_path(get_bin_path().c_str(), executable.c_str(), nullptr);

            for (int i = 1; i < argc; i++) {
                args[i] = argv[i - 1];
            }

            int rval = fork_command(args, ".", nullptr);

            free(args);

            return rval;
        }

        int Repository::load_plugins(const Options &opts)
        {
            std::string path = get_plugin_path();

            if (!directory_exists(path.c_str())) {
                return PREP_SUCCESS;
            }

            struct dirent *d = nullptr;
            DIR *dir         = opendir(path.c_str());

            if (dir == nullptr) {
                log::perror(errno);
                return PREP_FAILURE;
            }

            log::info("loading plugins");

            while ((d = readdir(dir)) != nullptr) {
                if (d->d_name[0] == '.') {
                    continue;
                }

                auto pluginPath = build_sys_path(path.c_str(), d->d_name, nullptr);

                auto plugin = std::make_shared<Plugin>(d->d_name);

                switch (plugin->load(pluginPath)) {
                case PREP_SUCCESS:
                    plugin->set_verbose(opts.verbose);
                    plugins_.push_back(plugin);
                    if (!plugin->is_internal()) {
                        log::trace("loaded plugin ",color::m(plugin->name())," version [",color::y(plugin->version()),"]");
                    }
                    break;
                case PREP_FAILURE:
                    log::warn("unable to load plugin [",d->d_name,"]" );
                    break;
                case PREP_ERROR:
                    log::trace("skipping non-plugin [", d->d_name, "]");
                    break;
                default:
                    break;
                }
            }

            closedir(dir);

            plugins_.sort(
                [](const std::shared_ptr<Plugin> &a, const std::shared_ptr<Plugin> &b) { return a->is_internal(); });

            for (const auto &plugin : plugins_) {
                vt100::Progress progress;

                log::info("initializing ", color::m(plugin->name()), " [", color::y(plugin->version()), "]");

                if (plugin->on_load() == PREP_ERROR) {
                    return PREP_FAILURE;
                }
            }

            return PREP_SUCCESS;
        }

        int Repository::notify_plugins_resolve(const Package &config, const resolver_callback &callback)
        {
            log::trace("checking plugins for resolving [", config.name(), "]...");

            for (const auto &plugin : plugins_) {
                auto result = plugin->on_resolve(config);
                if (result == PREP_SUCCESS) {
                    log::info("resolved ", color::m(config.name()), " from plugin ", color::c(plugin->name()));
                    if (callback) {
                        callback(result);
                    }
                    return PREP_SUCCESS;
                }
            }
            return PREP_FAILURE;
        }

        int Repository::notify_plugins_resolve(const std::string &location, const resolver_callback &callback)
        {
            log::trace("checking plugins for resolving [", location, "]...");

            for (const auto &plugin : plugins_) {
                auto result = plugin->on_resolve(location);

                if (result == PREP_SUCCESS) {
                    log::info("resolved ", color::m(location), " from plugin ", color::c(plugin->name()));
                    if (callback) {
                        callback(result);
                    }
                    return PREP_SUCCESS;
                }
            }
            return PREP_FAILURE;
        }

        int Repository::notify_plugins_install(const Package &config)
        {
            log::trace("checking plugins for install of [", config.name(), "]...");

            for (const auto &plugin : plugins_) {
                if (plugin->on_install(config, path_) == PREP_SUCCESS) {
                    log::info("installed ", color::m(config.name()), " from plugin ", color::c(plugin->name()));
                    return PREP_SUCCESS;
                }
            }
            return PREP_FAILURE;
        }

        int Repository::notify_plugins_remove(const Package &config)
        {
            log::trace("checking plugins for removal of [", config.name(), "]...");

            for (const auto &plugin : plugins_) {
                if (plugin->on_remove(config, path_) == PREP_SUCCESS) {
                    log::info("removed ", color::m(config.name()), " from plugin ", color::c(plugin->name()));
                    return PREP_SUCCESS;
                }
            }
            return PREP_FAILURE;
        }

        std::shared_ptr<Plugin> Repository::get_plugin_by_name(const std::string &name) const
        {
            for (const auto &plugin : plugins_) {
                if (plugin->name() == name) {
                    return plugin;
                }
            }
            return nullptr;
        }

        int Repository::notify_plugins_build(const Package &config, const std::string &sourcePath,
                                             const std::string &buildPath, const std::string &installPath)
        {
            for (const auto &name : config.build_system()) {
                auto plugin = get_plugin_by_name(name);

                if (!plugin) {
                    log::error("No plugin [", name, "] found");
                    return PREP_FAILURE;
                }

                log::info("Building ", color::m(config.name()), " with ", color::c(plugin->name()));

                if (plugin->on_build(config, sourcePath, buildPath, installPath) == PREP_FAILURE) {
                    return PREP_FAILURE;
                }
            }
            return PREP_SUCCESS;
        }
    }
}
