#ifndef MICRANTHA_PREP_REPOSITORY_H
#define MICRANTHA_PREP_REPOSITORY_H

#include <list>
#include <memory>
#include <string>

#include "package_config.h"
#include "plugin.h"

namespace micrantha
{
    namespace prep
    {
        class repository
        {
           public:
#ifdef _WIN32
            constexpr static const char *const UNKNOWN_INSTALL_FOLDER = "C:\\Windows\\Temp\\Unknown";
            constexpr static const char *const GLOBAL_REPO = "C:\\prep";
#else
            constexpr static const char *const UNKNOWN_INSTALL_FOLDER = "/tmp/unknown";
            constexpr static const char *const GLOBAL_REPO = "/usr/local/share/prep";
#endif
            /**
             * the repository name
             */
            constexpr static const char *const LOCAL_REPO_NAME = ".prep";

            /**
             * meta data folder in the repository
             */
            constexpr static const char *const META_FOLDER = "meta";

            /**
             * install folder in the repository
             */
            constexpr static const char *const INSTALL_FOLDER = "install";

            /**
             * build folder in the repository
             */
            constexpr static const char *const BUILD_FOLDER = "build";

            /**
             * kitchen folder in the repository
             */
            constexpr static const char *const KITCHEN_FOLDER = "kitchen";

            /**
             * binary folder in the repository
             */
            constexpr static const char *const BIN_FOLDER = "bin";

            /**
             * plugins folder in the repository
             */
            constexpr static const char *const PLUGIN_FOLDER = "plugins";

            /**
             * version information file
             */
            constexpr static const char *const VERSION_FILE = "version";

            /**
             * the file name for package configuration
             */
            constexpr static const char *const PACKAGE_FILE = "package.json";

            /**
             * a callback for resolving plugins
             */
            typedef std::function<void(const plugin::Result &result)> resolver_callback;

            /**
             * gets the user local repository on the system
             */
            static const char *const get_local_repo();

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
            int save_meta(const package &config) const;

            /**
             * tests for meta data on a package
             */
            int has_meta(const package &config) const;

            /**
             * counts the dependencies for a package name in the entire repository
             */
            int dependency_count(const std::string &package_name, const options &opts) const;

            /**
             * initializes the repository with the options
             * @return PREP_SUCCESS or PREP_FAILURE upon error
             */
            int initialize(const options &opts);

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

            // plugin path property
            std::string get_plugin_path() const;

            /**
             * validates the repository
             * @return PREP_SUCCESS or PREP_FAILURE if not valid
             */
            int validate() const;

            /**
             * validates the plugins
             * @return PREP_SUCCESS or PREP_FAILURE if any plugin invalid
             */
            int validate_plugins(const options &opts) const;

            /**
             * executes a binary from the repository bin path
             */
            int execute(const std::string &executable, int argc, char *const *argv) const;

            /**
             * runs the install callback on plugins for a config
             */
            int notify_plugins_install(const package &config);

            /**
             * runs the resolve callback on plugins for a config
             */
            int notify_plugins_resolve(const package &config, const resolver_callback &callback = nullptr);

            /**
             * runs the resolve callback on plugins for a config
             */
            int notify_plugins_resolve(const std::string &location, const resolver_callback &callback = nullptr);

            /**
             * runs the remove callback on plugins for a config
             */
            int notify_plugins_remove(const package &config);

            /**
             * runs the build callback on plugins for a config
             */
            int notify_plugins_build(const package &config, const std::string &sourcePath, const std::string &buildPath,
                                     const std::string &installPath);

            /**
             * gets a plugin by name
             */
            std::shared_ptr<plugin> get_plugin_by_name(const std::string &name) const;

            /**
             * loads the plugins
             * @return PREP_SUCCESS or PREP_FAILURE upon error
             */
            int load_plugins();

           private:
            int init_plugins(const options &opts, const std::string &path) const;

            // a list of plugins
            std::list<std::shared_ptr<plugin>> plugins_;
            // the repository path
            std::string path_;
        };
    }
}

#endif
