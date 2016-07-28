#ifndef RJ_PREP_REPOSITORY_H
#define RJ_PREP_REPOSITORY_H

#include <string>
#include <memory>
#include <vector>
#include "package_config.h"
#include "package_resolver.h"
#include "plugin.h"

namespace rj
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

            constexpr static const char *const HISTORY_FILE = "history";

            constexpr static const char *const LOCAL_REPO_NAME = ".prep";

            constexpr static const char *const META_FOLDER = "meta";

            constexpr static const char *const INSTALL_FOLDER = "install";

            constexpr static const char *const BUILD_FOLDER = "build";

            constexpr static const char *const KITCHEN_FOLDER = "kitchen";

            constexpr static const char *const BIN_FOLDER = "bin";

            constexpr static const char *const PLUGIN_FOLDER = "plugins";

            constexpr static const char *const VERSION_FILE = "version";

            constexpr static const char *const PACKAGE_FILE = "package.json";

            static const char *const get_local_repo();

            int unlink_directory(const std::string &path) const;
            int link_directory(const std::string &path) const;

            std::string exists_in_history(const std::string &) const;
            void save_history(const std::string &, const std::string &) const;
            int save_meta(const package &config) const;
            int has_meta(const package &config) const;

            int dependency_count(const package &config, const options &opts) const;
            int dependency_count(const string &package_name, const options &opts) const;

            int initialize(const options &opts);

            std::string get_install_path() const;
            std::string get_install_path(const string &package_name) const;

            std::string get_meta_path() const;
            std::string get_meta_path(const string &package_name) const;

            std::string get_bin_path() const;

            std::string get_build_path() const;
            std::string get_build_path(const string &package_name) const;

            std::string get_plugin_path() const;

            int validate() const;
            int validate_plugins() const;
            int execute(const std::string &executable, int argc, char *const *argv) const;

            int plugin_install(const package &config);

            int plugin_resolve(const package &config);

            int plugin_remove(const package &config);

            int plugin_build(const package &config, const std::string &sourcePath, const std::string &buildPath, const std::string &installPath);

            std::shared_ptr<plugin> get_plugin_by_name(const std::string &name) const;

            int load_plugins();

           private:
            int package_dependency_count(const package &config, const string &package_name, const options &opts) const;

            std::vector<std::shared_ptr<plugin>> plugins_;

            std::string path_;
        };
    }
}

#endif
