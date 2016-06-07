#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <dirent.h>
#include <libgen.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#include <curl/easy.h>
#endif
#include <fts.h>
#include <pwd.h>
#include <string>
#include "common.h"
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

namespace arg3
{
    namespace prep
    {
        const char *const repository::get_local_repo()
        {
            char buf[BUFSIZ] = {0};

            if (!getcwd(buf, BUFSIZ)) {
                log_error("no current directory");
                return nullptr;
            }

            const char *prefix = buf;
            const char *current_path = build_sys_path(prefix, LOCAL_REPO_NAME, NULL);

            while(directory_exists(current_path) == 1) {
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

            if (load_plugins() == PREP_FAILURE) {
                log_error("unable to load plugins");
                return PREP_FAILURE;
            }

            return PREP_SUCCESS;
        }

        int repository::validate() const
        {
            if (path_.empty()) {
                return PREP_FAILURE;
            }

            if (!directory_exists(path_.c_str())) {
                int ch;

                printf("OK to create folder %s? (Y/n) ", path_.c_str());

                ch = getchar();

                if (ch != 10 && toupper(ch) != 'Y') {
                    return PREP_FAILURE;
                }

                mkdir(path_.c_str(), 0700);
            }

            char *temp = getenv("PATH");
            string binPath = build_sys_path(path_.c_str(), BIN_FOLDER, NULL);

            if (temp != NULL) {
                char buf[BUFSIZ] = {0};

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

        std::string repository::get_build_path(const string &package_name) const
        {
            return build_sys_path(path_.c_str(), KITCHEN_FOLDER, BUILD_FOLDER, package_name.c_str(), NULL);
        }

        std::string repository::get_install_path() const
        {
            return build_sys_path(path_.c_str(), KITCHEN_FOLDER, INSTALL_FOLDER, NULL);
        }

        std::string repository::get_install_path(const string &package_name) const
        {
            return build_sys_path(path_.c_str(), KITCHEN_FOLDER, INSTALL_FOLDER, package_name.c_str(), NULL);
        }

        std::string repository::get_meta_path() const
        {
            return build_sys_path(path_.c_str(), KITCHEN_FOLDER, META_FOLDER, NULL);
        }

        std::string repository::get_meta_path(const string &package_name) const
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

            string metaDir = build_sys_path(path_.c_str(), KITCHEN_FOLDER, META_FOLDER, config.name().c_str(), NULL);

            if (!directory_exists(metaDir.c_str())) {
                if (mkpath(metaDir.c_str(), 0777)) {
                    log_errno(errno);
                    return PREP_FAILURE;
                }
            }

            ofstream out(build_sys_path(metaDir.c_str(), "version", NULL));

            if (!out.is_open()) {
                log_error("unable to save version for %s", config.name().c_str());
                return PREP_FAILURE;
            }

            if (!config.version().empty()) {
                out << config.version() << endl;
            } else if (!config.location().empty()) {
                out << config.location() << endl;
            }

            out.close();

            if (config.has_path()) {
                log_trace("copying %s to %s...", config.path().c_str(), metaDir.c_str());

                if (copy_file(config.path(), build_sys_path(metaDir.c_str(), "package.json", NULL))) {
                    log_error("no unable to copy package file %s", config.path().c_str());
                }
            }

            return PREP_SUCCESS;
        }

        int repository::has_meta(const package &config) const
        {
            string metaDir = build_sys_path(path_.c_str(), KITCHEN_FOLDER, META_FOLDER, config.name().c_str(), NULL);

            if (!directory_exists(metaDir.c_str())) {
                return PREP_FAILURE;
            }

            ifstream in(build_sys_path(metaDir.c_str(), "version", NULL));

            string info;
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

        int repository::package_dependency_count(const package &config, const string &package_name, const options &opts) const
        {
            int count = 0;

            for (const package_dependency &dep : config.dependencies()) {
                if (!strcmp(dep.name().c_str(), package_name.c_str())) {
                    count++;
                }
                count += package_dependency_count(dep, package_name, opts);
            }

            return count;
        }

        int repository::dependency_count(const string &package_name, const options &opts) const
        {
            DIR *dir;
            struct dirent *d_ent;
            int rval = PREP_SUCCESS;
            char buf[PATH_MAX + 1] = {0};
            struct stat st;
            size_t count = 0;
            string metaDir;

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
                    count += package_dependency_count(temp, package_name, opts);
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
            FTS *file_system = NULL;
            FTSENT *child = NULL;
            FTSENT *parent = NULL;
            int rval = PREP_SUCCESS;
            char buf[PATH_MAX + 1] = {0};

            if (!directory_exists(path.c_str())) {
                log_error("%s is not a directory", path.c_str());
                return PREP_FAILURE;
            }

            char *const paths[] = {(char *const)path.c_str(), NULL};

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
            FTS *file_system = NULL;
            FTSENT *child = NULL;
            FTSENT *parent = NULL;
            int rval = PREP_SUCCESS;
            char buf[PATH_MAX + 1] = {0};
            string installDir;

            if (!directory_exists(path.c_str())) {
                log_error("%s does not exist.", path.c_str());
                return PREP_FAILURE;
            }

            char *const paths[] = {(char *const)path.c_str(), NULL};

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

            const char **args = (const char**) calloc(argc + 2, sizeof(const char*));

            args[0] = build_sys_path(get_bin_path().c_str(), executable.c_str(), NULL);

            for(int i = 1; i < argc; i++) {
                args[i] = argv[i-1];
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

            ifstream is(build_sys_path(path_.c_str(), KITCHEN_FOLDER, HISTORY_FILE, NULL));

            std::map<std::string, std::string> mps;
            std::insert_iterator<std::map<std::string, std::string> > mpsi(mps, mps.begin());

            const std::istream_iterator<std::pair<std::string, std::string> > eos;
            std::istream_iterator<std::pair<std::string, std::string> > its(is);

            std::copy(its, eos, mpsi);

            is.close();

            return mps[location];
        }

        void repository::save_history(const std::string &location, const std::string &working_dir) const
        {
            if (location.empty()) {
                return;
            }

            ifstream is(build_sys_path(path_.c_str(), KITCHEN_FOLDER, HISTORY_FILE, NULL));

            std::map<std::string, std::string> mps;
            std::insert_iterator<std::map<std::string, std::string> > mpsi(mps, mps.begin());

            const std::istream_iterator<std::pair<std::string, std::string> > eos;
            std::istream_iterator<std::pair<std::string, std::string> > its(is);

            std::copy(its, eos, mpsi);

            is.close();

            mps[location] = working_dir;

            ofstream os(build_sys_path(path_.c_str(), HISTORY_FILE, NULL));

            std::copy(mps.begin(), mps.end(), std::ostream_iterator<std::pair<std::string, std::string> >(os, "\n"));

            os.close();
        }

        int repository::load_plugins()
        {
            std::string path = get_plugin_path();
            struct stat st;

            if (!directory_exists(path.c_str())) {
                log_warn("No plugin directory");
                return PREP_SUCCESS;
            }

            struct dirent *d = NULL;
            DIR *dir = opendir(path.c_str());

            if (dir == NULL) {
                log_errno(errno);
                return PREP_FAILURE;
            }

            while((d = readdir(dir)) != NULL) {

                if (d->d_name[0] == '.') {
                    continue;
                }

                auto pluginPath = build_sys_path(path.c_str(), d->d_name, NULL);

                if (!directory_exists(pluginPath)) {
                    continue;
                }

                auto plugin = std::make_shared<arg3::prep::plugin>(d->d_name);

                try {
                    if (plugin->load(pluginPath) == PREP_SUCCESS) {
                        plugins_.push_back(plugin);
                    } else {
                        log_warn("unable to load plugin [%s]", d->d_name);
                    }
                } catch (const std::string &not_a_plugin) {
                    log_trace("skipping non-plugin [%s]", not_a_plugin.c_str());
                }
            }

            closedir(dir);

            for(auto &plugin : plugins_) {
                plugin->on_load();
            }

            return PREP_SUCCESS;
        }

        int repository::plugin_install(const package &config)
        {
            log_trace("checking plugins for install of [%s]...", config.name().c_str());

            for(auto plugin : plugins_) {
                if (plugin->on_install(config) == PREP_SUCCESS) {
                    log_info("installed [%s] from plugin", config.name().c_str());
                    return PREP_SUCCESS;
                }
            }
            return PREP_FAILURE;
        }

        int repository::plugin_remove(const package &config)
        {
            log_trace("checking plugins for removal of [%s]...", config.name().c_str());

            for (auto plugin : plugins_) {
                if (plugin->on_remove(config) == PREP_SUCCESS) {
                    log_info("removed [%s] from plugin", config.name().c_str());
                    return PREP_SUCCESS;
                }
            }
            return PREP_FAILURE;
        }

        std::shared_ptr<plugin> repository::get_plugin_by_name(const std::string &name) const {
            for(auto &plugin : plugins_) {
                if (plugin->name() == name) {
                    return plugin;
                }
            }
            return nullptr;
        }


        int repository::plugin_build(const package &config, const std::string &sourcePath, const std::string &buildPath, const std::string &installPath)
        {
            for (auto &name : config.build_system()) {
                auto plugin = get_plugin_by_name(name);

                if (!plugin) {
                    log_error("No plugin [%s] found", name.c_str());
                    return PREP_FAILURE;
                }

                log_debug("Running [%s] for \x1b[1;35m%s\x1b[0m", plugin->name().c_str(), config.name().c_str());

                if(plugin->on_build(config, sourcePath, buildPath, installPath) == PREP_FAILURE) {
                    return PREP_FAILURE;
                }
            }
            return PREP_SUCCESS;
        }
    }
}
