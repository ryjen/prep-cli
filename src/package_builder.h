#ifndef INCLUDE_PACKAGE_BUILDER_H
#define INCLUDE_PACKAGE_BUILDER_H

#include "package_config.h"
#include "environment.h"
#include "repository.h"

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
            int remove(const package &config, options &opts);
            int remove(const std::string &package_name, options &opts);
            int link_package(const package &config) const;
            int unlink_package(const package &config) const;
        private:
            int build_package(const package &p, const char *path);

            int build_autotools(const package &config, const char *path, const char *toPath);
            int build_cmake(const package &config, const char *path, const char *toPath);
            int build_make(const package &config, const char *path);

            string get_install_dir(const package &config) const;

            environment env_;

            repository repo_;

            bool force_build_;
        };
    }
}

#endif
