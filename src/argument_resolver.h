#ifndef INCLUDE_ARGUMENT_RESOLVER
#define INCLUDE_ARGUMENT_RESOLVER

#include <string>

#include "package_config.h"

namespace arg3
{
    namespace cpppm
    {
        class argument_resolver
        {
        public:
            static const int UNKNOWN = 255;
            argument_resolver(const std::string &arg);
            argument_resolver();

            void set_arg(const std::string &arg);
            std::string arg() const;

            int resolve_package(package_config &config) const;

        private:
            int resolve_package_git(package_config &config) const;
            int resolve_package_directory(package_config &config, const char *path) const;
            int resolve_package_download(package_config &config) const;
            int resolve_package_tar_gz(package_config &config, const char *path) const;
            std::string arg_;
        };
    }
}

#endif

