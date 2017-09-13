#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <fts.h>
#include <iostream>
#include <iterator>
#include <libgen.h>
#include <map>
#include <pwd.h>
#include <string>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#include <curl/easy.h>
#endif
#include "common.h"
#include "decompressor.h"
#include "log.h"
#include "repository.h"
#include "util.h"

// for std::copy
namespace std
{
    istream &operator>>(istream &is, pair<string, string> &ps)
    {
        return is >> ps.first >> ps.second;
    }
    ostream &operator<<(ostream &os, const pair<const string, string> &ps)
    {
        return os << ps.first << " " << ps.second;
    }
}

namespace micrantha
{
    namespace prep
    {
        namespace helper
        {
            extern bool is_valid_plugin_path(const std::string &path);
            extern bool is_plugin_support(const std::string &path);
        }

        const char *const repository::get_local_repo()
        {
            char buf[BUFSIZ] = { 0 };

            if (!getcwd(buf, BUFSIZ)) {
                log_error("no current directory");
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

        int repository::initialize(const options &opts)
        {
            if (opts.global) {
                path_ = GLOBAL_REPO;
            } else {
                path_ = get_local_repo();
            }

            return PREP_SUCCESS;
        }

        int repository::validate() const
        {
            if (path_.empty()) {
                return PREP_FAILURE;
            }

            if (!directory_exists(path_.c_str())) {
                std::string buf;

                printf("OK to create folder %s? (Y/n) ", path_.c_str());

                std::getline(std::cin, buf);

                if (!buf.empty() && buf[0] != 10 && toupper(buf[0]) != 'Y') {
                    return PREP_FAILURE;
                }

                mkdir(path_.c_str(), 0700);
            }

            char *temp          = getenv("PATH");
            std::string binPath = build_sys_path(path_.c_str(), BIN_FOLDER, NULL);

            if (temp != NULL) {
                char buf[BUFSIZ] = { 0 };

                strncpy(buf, temp, BUFSIZ);

                temp = strtok(buf, ":");

                while (temp != NULL) {
                    if (binPath == temp) {
                        break;
                    }

                    temp = strtok(NULL, ":");
                }
            }

            if (temp == NULL) {
                log_warn("%s is not added to your PATH", binPath.c_str());
            }
            return PREP_SUCCESS;
        }

        int repository::validate_plugins(const options &opts) const
        {
            const std::string globalPath = build_sys_path(GLOBAL_REPO, PLUGIN_FOLDER, NULL);

            struct dirent *d             = NULL;
            DIR *dir                     = NULL;
            const std::string pluginPath = get_plugin_path();

            if (!directory_exists(pluginPath.c_str())) {
                if (mkdir(pluginPath.c_str(), 0777)) {
                    log_errno(errno);
                    return PREP_FAILURE;
                }
            }

            dir = opendir(globalPath.c_str());

            if (dir == NULL) {
                if (init_plugins(opts, globalPath) == PREP_FAILURE) {
                    return PREP_FAILURE;
                }

                dir = opendir(globalPath.c_str());

                if (dir == NULL) {
                    log_errno(errno);
                    return PREP_FAILURE;
                }
            }

            while ((d = readdir(dir)) != NULL) {
                if (d->d_name[0] == '.') {
                    continue;
                }

                const std::string globalPlugin = build_sys_path(globalPath.c_str(), d->d_name, NULL);

                bool is_plugin = helper::is_valid_plugin_path(globalPlugin);

                if (!is_plugin && !helper::is_plugin_support(globalPlugin)) {
                    continue;
                }

                const std::string localPath = build_sys_path(pluginPath.c_str(), d->d_name, NULL);

                if (is_plugin) {
                    std::string buf;

                    if (!helper::is_valid_plugin_path(localPath)) {
                        printf("OK to add plugin %s? (Y/n) ", d->d_name);

                        std::getline(std::cin, buf);

                        if (!buf.empty() && buf[0] != 10 && toupper(buf[0]) != 'Y') {
                            continue;
                        }
                    }
                }

                if (copy_directory(globalPlugin, localPath, false)) {
                    log_error("unable to copy [%s] to [%s]", globalPlugin.c_str(), localPath.c_str());
                }
            }

            closedir(dir);

            return PREP_SUCCESS;
        }

        int repository::init_plugins(const options &opts, const std::string &path) const
        {
            decompressor unzip(opts.executable, path);

            return unzip.decompress(true);
        }

        std::string repository::get_bin_path() const
        {
            return build_sys_path(path_.c_str(), BIN_FOLDER, NULL);
        }

        std::string repository::get_plugin_path() const
        {
            return build_sys_path(path_.c_str(), PLUGIN_FOLDER, NULL);
        }

        std::string repository::get_build_path() const
        {
            return build_sys_path(path_.c_str(), KITCHEN_FOLDER, BUILD_FOLDER, NULL);
        }

        std::string repository::get_build_path(const std::string &package_name) const
        {
            return build_sys_path(path_.c_str(), KITCHEN_FOLDER, BUILD_FOLDER, package_name.c_str(), NULL);
        }

        std::string repository::get_install_path() const
        {
            return build_sys_path(path_.c_str(), KITCHEN_FOLDER, INSTALL_FOLDER, NULL);
        }

        std::string repository::get_install_path(const std::string &package_name) const
        {
            return build_sys_path(path_.c_str(), KITCHEN_FOLDER, INSTALL_FOLDER, package_name.c_str(), NULL);
        }

        std::string repository::get_meta_path() const
        {
            return build_sys_path(path_.c_str(), KITCHEN_FOLDER, META_FOLDER, NULL);
        }

        std::string repository::get_meta_path(const std::string &package_name) const
        {
            return build_sys_path(path_.c_str(), KITCHEN_FOLDER, META_FOLDER, package_name.c_str(), NULL);
        }

        int repository::save_meta(const package &config) const
        {
            if (path_.empty()) {
                log_error("uninitialized repository path");
                return PREP_FAILURE;
            }

            if (!config.has_name()) {
                log_error("configuration has no name");
                return PREP_FAILURE;
            }

            std::string metaDir =
                build_sys_path(path_.c_str(), KITCHEN_FOLDER, META_FOLDER, config.name().c_str(), NULL);

            if (!directory_exists(metaDir.c_str())) {
                if (mkpath(metaDir.c_str(), 0777)) {
                    log_errno(errno);
                    return PREP_FAILURE;
                }
            }

            std::ofstream out(build_sys_path(metaDir.c_str(), VERSION_FILE, NULL));

            if (!out.is_open()) {
                log_error("unable to save version for %s", config.name().c_str());
                return PREP_FAILURE;
            }

            if (!config.version().empty()) {
                out << config.version() << std::endl;
            } else if (!config.location().empty()) {
                out << config.location() << std::endl;
            }

            out.close();

            if (config.has_path()) {
                log_trace("copying %s to %s...", config.path().c_str(), metaDir.c_str());

                if (copy_file(config.path(), build_sys_path(metaDir.c_str(), PACKAGE_FILE, NULL))) {
                    log_error("no unable to copy package file %s", config.path().c_str());
                }
            }

            return PREP_SUCCESS;
        }

        int repository::has_meta(const package &config) const
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
                    log_warn("      using cached version of %s (%s)", config.name().c_str(), info.c_str());
                    return PREP_SUCCESS;
                }
            } else if (!config.location().empty()) {
                if (!strcmp(info.c_str(), config.location().c_str())) {
                    log_warn("      using cached version of %s (%s)", config.name().c_str(), info.c_str());
                    return PREP_SUCCESS;
                }
            }

            return PREP_FAILURE;
        }

        int repository::dependency_count(const std::string &package_name, const options &opts) const
        {
            DIR *dir;
            struct dirent *d_ent;
            int rval               = PREP_SUCCESS;
            char buf[PATH_MAX + 1] = { 0 };
            struct stat st;
            size_t count = 0;
            std::string metaDir;

            if (path_.empty()) {
                log_error("No repository path defined!");
                return PREP_FAILURE;
            }

            metaDir = build_sys_path(path_.c_str(), KITCHEN_FOLDER, META_FOLDER, NULL);

            if (!directory_exists(metaDir.c_str())) {
                log_error("%s is not a directory", metaDir.c_str());
                return 0;
            }

            dir = opendir(metaDir.c_str());

            if (dir == NULL) {
                log_error("unable to open file system [%s]", strerror(errno));
                return 0;
            }

            while ((d_ent = readdir(dir)) != NULL) {
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
                    log_debug("%s doesn't exist??, skipping", buf, strerror(errno));
                    continue;
                }

                if (!S_ISDIR(st.st_mode)) {
                    log_debug("Not a directory [%s]", buf);
                    continue;
                }

                package_config temp;

                if (temp.load(buf, opts) == PREP_SUCCESS) {
                    count += temp.dependency_count(package_name);
                }
            }

            return count;
        }

        int repository::dependency_count(const package &config, const options &opts) const
        {
            if (!config.is_loaded()) {
                log_error("config is not loaded");
                return -1;
            }

            return dependency_count(config.name(), opts);
        }


        int repository::link_directory(const std::string &path) const
        {
            FTS *file_system       = NULL;
            FTSENT *child          = NULL;
            FTSENT *parent         = NULL;
            int rval               = PREP_SUCCESS;
            char buf[PATH_MAX + 1] = { 0 };

            if (!directory_exists(path.c_str())) {
                log_error("%s is not a directory", path.c_str());
                return PREP_FAILURE;
            }

            char *const paths[] = { (char *const)path.c_str(), NULL };

            file_system = fts_open(paths, FTS_COMFOLLOW | FTS_NOCHDIR, NULL);

            if (file_system == NULL) {
                log_error("unable to open file system [%s]", strerror(errno));
                return PREP_FAILURE;
            }

            while ((parent = fts_read(file_system)) != NULL) {
                struct stat st;

                // get the repo path
                snprintf(buf, PATH_MAX, "%s%s", path_.c_str(), parent->fts_path + path.length());

                // check the install file is a directory
                if (parent->fts_info == FTS_D) {
                    if (stat(buf, &st) == 0) {
                        if (S_ISDIR(st.st_mode)) {
                            log_trace("directory %s already exists", buf);
                        } else {
                            log_error("non-directory file already found for %s", buf);
                            rval = PREP_FAILURE;
                        }
                        continue;
                    }

                    // doesn't exist, create the repo path directory
                    if (mkdir(buf, 0777)) {
                        log_error("Could not create %s [%s]", buf, strerror(errno));
                        rval = PREP_FAILURE;
                        break;
                    }

                    continue;
                }

                if (parent->fts_info != FTS_F) {
                    log_debug("skipping non-regular file %s", buf);
                    continue;
                }

                if (lstat(buf, &st) == 0) {
                    if (!S_ISLNK(st.st_mode)) {
                        log_error("File already exists and is not a link");
                        rval = PREP_FAILURE;
                        break;
                    }

                    if (unlink(buf)) {
                        log_error("unable to unlink existing [%s]", strerror(errno));
                        continue;
                    }
                }

                log_debug("linking [%s] to [%s]", parent->fts_path, buf);

                if (symlink(parent->fts_path, buf)) {
                    log_error("unable to link [%s]", strerror(errno));
                    rval = PREP_FAILURE;
                    break;
                }
            }

            fts_close(file_system);

            return rval;
        }


        int repository::unlink_directory(const std::string &path) const
        {
            FTS *file_system       = NULL;
            FTSENT *child          = NULL;
            FTSENT *parent         = NULL;
            int rval               = PREP_SUCCESS;
            char buf[PATH_MAX + 1] = { 0 };
            std::string installDir;

            if (!directory_exists(path.c_str())) {
                log_error("%s does not exist.", path.c_str());
                return PREP_FAILURE;
            }

            char *const paths[] = { (char *const)path.c_str(), NULL };

            file_system = fts_open(paths, FTS_COMFOLLOW | FTS_NOCHDIR, NULL);

            if (file_system == NULL) {
                log_error("unable to open file system [%s]", strerror(errno));
                return PREP_FAILURE;
            }

            size_t plength = path.length();

            while ((parent = fts_read(file_system)) != NULL) {
                struct stat st;

                if (parent->fts_info == FTS_D) {
                    log_debug("skipping directory %s", parent->fts_path);
                    continue;
                }

                // get the repo path
                snprintf(buf, PATH_MAX, "%s%s", path_.c_str(), parent->fts_path + plength);

                if (lstat(buf, &st)) {
                    log_debug("%s not found (%s), skipping", buf, strerror(errno));
                    continue;
                }

                if (S_ISDIR(st.st_mode)) {
                    log_debug("skipping directory %s", buf);
                    continue;
                }

                if (!S_ISLNK(st.st_mode)) {
                    log_error("%s is not a link, skipping", buf);
                    rval = PREP_FAILURE;
                    continue;
                }

                log_debug("unlinking [%s]", buf);

                if (unlink(buf)) {
                    log_error("unable to unlink [%s]", strerror(errno));
                    rval = PREP_FAILURE;
                    break;
                }
            }

            fts_close(file_system);

            return rval;
        }

        int repository::execute(const std::string &executable, int argc, char *const *argv) const
        {
            if (executable.empty()) {
                return PREP_FAILURE;
            }

            const char **args = (const char **)calloc(argc + 2, sizeof(const char *));

            args[0] = build_sys_path(get_bin_path().c_str(), executable.c_str(), NULL);

            for (int i = 1; i < argc; i++) {
                args[i] = argv[i - 1];
            }

            int rval = fork_command(args, ".", nullptr);

            free(args);

            return rval;
        }


        std::string repository::exists_in_history(const std::string &location) const
        {
            if (location.empty()) {
                return location;
            }

            std::ifstream is(build_sys_path(path_.c_str(), KITCHEN_FOLDER, HISTORY_FILE, NULL));

            std::map<std::string, std::string> mps;
            std::insert_iterator<std::map<std::string, std::string>> mpsi(mps, mps.begin());

            const std::istream_iterator<std::pair<std::string, std::string>> eos;
            std::istream_iterator<std::pair<std::string, std::string>> its(is);

            std::copy(its, eos, mpsi);

            is.close();

            return mps[location];
        }

        void repository::save_history(const std::string &location, const std::string &working_dir) const
        {
            if (location.empty()) {
                return;
            }

            std::ifstream is(build_sys_path(path_.c_str(), KITCHEN_FOLDER, HISTORY_FILE, NULL));

            std::map<std::string, std::string> mps;
            std::insert_iterator<std::map<std::string, std::string>> mpsi(mps, mps.begin());

            const std::istream_iterator<std::pair<std::string, std::string>> eos;
            std::istream_iterator<std::pair<std::string, std::string>> its(is);

            std::copy(its, eos, mpsi);

            is.close();

            mps[location] = working_dir;

            std::ofstream os(build_sys_path(path_.c_str(), HISTORY_FILE, NULL));

            std::copy(mps.begin(), mps.end(), std::ostream_iterator<std::pair<std::string, std::string>>(os, "\n"));

            os.close();
        }

        int repository::load_plugins()
        {
            std::string path = get_plugin_path();
            struct stat st;

            if (!directory_exists(path.c_str())) {
                if (mkdir(path.c_str(), 0777)) {
                    log_errno(errno);
                    return PREP_FAILURE;
                }
                return PREP_SUCCESS;
            }

            struct dirent *d = NULL;
            DIR *dir         = opendir(path.c_str());

            if (dir == NULL) {
                log_errno(errno);
                return PREP_FAILURE;
            }

            while ((d = readdir(dir)) != NULL) {
                if (d->d_name[0] == '.') {
                    continue;
                }

                auto pluginPath = build_sys_path(path.c_str(), d->d_name, NULL);

                if (!helper::is_valid_plugin_path(pluginPath)) {
                    continue;
                }

                auto plugin = std::make_shared<micrantha::prep::plugin>(d->d_name);

                switch (plugin->load(pluginPath)) {

                case PREP_SUCCESS:
                    plugins_.push_back(plugin);
                    break;
                case PREP_FAILURE:
                    log_warn("unable to load plugin [%s]", d->d_name);
                    break;
                case PREP_ERROR:
                    log_trace("skipping non-plugin [%s]", d->d_name);
                    break;
                }
            }

            closedir(dir);

            for (auto &plugin : plugins_) {
                if (plugin->on_load() == PREP_ERROR) {
                    return PREP_FAILURE;
                }
            }

            return PREP_SUCCESS;
        }

        int repository::notify_plugins_resolve(const package &config, const resolver_callback &callback)
        {
            log_trace("checking plugins for resolving [%s]...", config.name().c_str());

            for (auto plugin : plugins_) {
                if (plugin->on_resolve(config) == PREP_SUCCESS) {
                    log_info("resolved [%s] from plugin [%s]", config.name().c_str(), plugin->name().c_str());
                    if (callback) {
                        callback(plugin);
                    }
                    return PREP_SUCCESS;
                }
            }
            return PREP_FAILURE;
        }

        int repository::notify_plugins_resolve(const std::string &location, const resolver_callback &callback)
        {
            log_trace("checking plugins for resolving [%s]...", location.c_str());

            for (auto plugin : plugins_) {
                if (plugin->on_resolve(location) == PREP_SUCCESS) {
                    log_info("resolved [%s] from plugin [%s]", location.c_str(), plugin->name().c_str());
                    if (callback) {
                        callback(plugin);
                    }
                    return PREP_SUCCESS;
                }
            }
            return PREP_FAILURE;
        }

        int repository::notify_plugins_install(const package &config)
        {
            log_trace("checking plugins for install of [%s]...", config.name().c_str());

            for (auto plugin : plugins_) {
                if (plugin->on_install(config, path_) == PREP_SUCCESS) {
                    log_info("installed [%s] from plugin [%s]", config.name().c_str(), plugin->name().c_str());
                    return PREP_SUCCESS;
                }
            }
            return PREP_FAILURE;
        }

        int repository::notify_plugins_remove(const package &config)
        {
            log_trace("checking plugins for removal of [%s]...", config.name().c_str());

            for (auto plugin : plugins_) {
                if (plugin->on_remove(config, path_) == PREP_SUCCESS) {
                    log_info("removed [%s] from plugin [%s]", config.name().c_str(), plugin->name().c_str());
                    return PREP_SUCCESS;
                }
            }
            return PREP_FAILURE;
        }

        std::shared_ptr<plugin> repository::get_plugin_by_name(const std::string &name) const
        {
            for (auto &plugin : plugins_) {
                if (plugin->name() == name) {
                    return plugin;
                }
            }
            return nullptr;
        }


        int repository::notify_plugins_build(const package &config, const std::string &sourcePath,
                                             const std::string &buildPath, const std::string &installPath)
        {
            for (auto &name : config.build_system()) {
                auto plugin = get_plugin_by_name(name);

                if (!plugin) {
                    log_error("No plugin [%s] found", name.c_str());
                    return PREP_FAILURE;
                }

                if (plugin->on_build(config, sourcePath, buildPath, installPath) == PREP_FAILURE) {
                    return PREP_FAILURE;
                }
            }
            return PREP_SUCCESS;
        }
    }
}
