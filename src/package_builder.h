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
            void initialize();
            void set_global(bool value);
            int build(const package_config &config, const char *path);
            int build_from_folder(const char *path);
        private:
            int build_package(const package &p, const char *path);
            string build_cflags() const;
            string build_ldflags() const;
            int build_autotools(const char *path);
            int build_cmake(const char *path);
            string get_path() const;
            const char *const get_home_dir() const;
            bool global_;

            static const char *const LOCAL_REPO;

            static const char *const GLOBAL_REPO;
        };
    }
}

#endif
