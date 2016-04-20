#ifndef ARG3_PREP_REPOSITORY_H
#define ARG3_PREP_REPOSITORY_H

#include <string>
#include "package_config.h"

namespace arg3
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

            static const char *const get_local_repo();

            int unlink_directory(const char *path) const;
            int link_directory(const char *path) const;

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

            int validate() const;

           private:
            int package_dependency_count(const package &config, const string &package_name, const options &opts) const;

            std::string path_;
        };
    }
}

#endif
