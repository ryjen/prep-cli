#ifndef INCLUDE_PACKAGE_BUILDER_H
#define INCLUDE_PACKAGE_BUILDER_H

#include "package_config.h"

namespace arg3
{
    namespace prep
    {
        class package_builder
        {
        public:
            int initialize(const options &opts);
            int build(const package &config, options &opts, const std::string &path);
            int build_from_folder(options &opts, const char *path);
            string repo_path() const;
        private:
            int build_package(const package &p, const char *path);
            string build_cflags(const package &config, const std::string &varName) const;
            string build_ldflags(const package &config, const std::string &varName) const;
            int build_autotools(const package &config, const char *path, const char *toPath);
            int build_cmake(const package &config, const char *path, const char *toPath);
            int build_make(const package &config, const char *path);
            string build_path(const package &config) const;
            string get_home_dir() const;
            string get_install_path(const package &config) const;
            std::string exists_in_history(const std::string &) const;
            void save_history(const std::string &, const std::string &) const;
            int save_meta(const package &config) const;
            int has_meta(const package &config) const;
            int link(const package &config, const string &fromPath, const string &toPath);

            string repo_path_;

            static const char *const LOCAL_REPO;

            static const char *const GLOBAL_REPO;

            static const char *const HISTORY_FILE;

            static const char *const META_FOLDER;

            static const char *const INSTALL_FOLDER;
        };
    }
}

#endif
