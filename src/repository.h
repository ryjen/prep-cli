
#ifndef MICRANTHA_PREP_REPOSITORY_H
#define MICRANTHA_PREP_REPOSITORY_H

#include <list>
#include <memory>
#include <string>

#include "package_config.h"
#include "plugin.h"

namespace micrantha {
    namespace prep {
        /**
         * a representation of a repository.  can be local or global
         */
        class Repository {
        public:
#ifdef _WIN32
            constexpr static const char *const UNKNOWN_INSTALL_FOLDER = "C:\\Windows\\Temp\\Unknown";
            constexpr static const char *const GLOBAL_REPO = "C:\\prep";
#else
            constexpr static const char *UNKNOWN_INSTALL_FOLDER = "/tmp/unknown";
            constexpr static const char *GLOBAL_REPO = "/usr/local/share/prep";
#endif
            /**
             * the repository name
             */
            constexpr static const char *LOCAL_REPO_NAME = ".prep";

            /**
             * meta data folder in the repository
             */
            constexpr static const char *META_FOLDER = "meta";

            /**
             * install folder in the repository
             */
            constexpr static const char *INSTALL_FOLDER = "install";

            /**
             * build folder in the repository
             */
            constexpr static const char *BUILD_FOLDER = "build";


            /**
             * source folder in the repository
             */
            constexpr static const char *SOURCE_FOLDER = "source";

            /**
             * kitchen folder in the repository
             */
            constexpr static const char *KITCHEN_FOLDER = "kitchen";

            /**
             * binary folder in the repository
             */
            constexpr static const char *BIN_FOLDER = "bin";

            /**
             * plugins folder in the repository
             */
            constexpr static const char *PLUGIN_FOLDER = "plugins";

            /**
             * version information file
             */
            constexpr static const char *VERSION_FILE = "version";

            /**
             * the file name for package configuration
             */
            constexpr static const char *PACKAGE_FILE = "package.json";

            /**
             * a callback for resolving plugins
             */
            typedef std::function<void(const Plugin::Result &result)> resolver_callback;

            /**
             * gets the user local repository on the system
             */
            static std::string const get_local_repo();

            /**
             * unlinks a folder in this repository
             */
            int unlink_directory(const std::string &path) const;

            /**
             * links a folder in this repository
             */
            int link_directory(const std::string &path) const;

            /**
             * saves meta data for a package
             */
            int save_meta(const Package &config) const;

            /**
             * tests for meta data on a package
             */
            int has_meta(const Package &config) const;

            /**
             * counts the dependencies for a package name in the entire repository
             */
            int dependency_count(const std::string &package_name, const Options &opts) const;

            /**
             * initializes the repository with the options
             * @return PREP_SUCCESS or PREP_FAILURE upon error
             */
            int initialize(const Options &opts);

            /**
             * install path property
             */
            std::string get_install_path(const std::string &package_name) const;

            /**
             * meta path property
             */
            std::string get_meta_path(const std::string &package_name) const;

            // bin path property
            std::string get_bin_path() const;

            // build path property
            std::string get_build_path(const std::string &package_name) const;


            // build path property
            std::string get_source_path(const std::string &package_name) const;


            // plugin path property
            std::string get_plugin_path() const;

            /**
             * check if a package exists in either the global or local repository
             * @param config the package config to check
             * @return true if a package matching the config exists
             */
            bool exists(const Package &config) const;

            /**
             * validates the repository
             * @return PREP_SUCCESS or PREP_FAILURE if not valid
             */
            int validate(const Options &opts) const;

            /**
             * validates the plugins
             * @return PREP_SUCCESS or PREP_FAILURE if any plugin invalid
             */
            int validate_plugins(const Options &opts) const;

            /**
             * runs the install callback on plugins for a config
             */
            int notify_plugins_add(const Package &config);

            /**
             * runs the resolve callback on plugins for a config
             */
            Plugin::Result notify_plugins_resolve(const Package &config);

            /**
             * runs the resolve callback on plugins for a config
             */
            Plugin::Result notify_plugins_resolve(const std::string &location);

            /**
             * runs the remove callback on plugins for a config
             */
            int notify_plugins_remove(const Package &config);

            /**
             * runs the build callback on plugins for a config
             */
            int notify_plugins_build(const Package &config, const std::string &sourcePath, const std::string &buildPath,
                                     const std::string &installPath);

            /**
             * runs the build callback on plugins for a config
             */
            int notify_plugins_test(const Package &config, const std::string &sourcePath, const std::string &buildPath);

            /**
             * runs the build callback on plugins for a config
             */
            int
            notify_plugins_install(const Package &config, const std::string &sourcePath, const std::string &buildPath);

            /**
             * gets a plugin by name
             */
            std::shared_ptr<Plugin> get_plugin_by_name(const std::string &name) const;

            /**
             * loads the plugins
             * @return PREP_SUCCESS or PREP_FAILURE upon error
             */
            int load_plugins(const Options &opts);

        private:
            /**
             * initializes / loads the plugins
             * @param opts the command line options
             * @param path the repository path
             * @return PREP_SUCCESS if successful, otherwise PREP_FAILURE
             */
            int init_plugins(const Options &opts, const std::string &path) const;

            std::list<std::shared_ptr<Plugin>> validPlugins_;

            // a list of plugins
            std::list<std::shared_ptr<Plugin>> plugins_;
            // the repository path
            std::string path_;
        };
    }
}

#endif
