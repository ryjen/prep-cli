#ifndef INCLUDE_ARGUMENT_RESOLVER
#define INCLUDE_ARGUMENT_RESOLVER

#include <string>

#include "package_config.h"

namespace arg3
{
    namespace prep
    {
        /* resolves a package from a download, git, archive or folder */
        class package_resolver
        {
        public:
            static const int UNKNOWN = 255;
            package_resolver(const std::string &arg);
            package_resolver();

            void set_location(const std::string &arg);
            std::string location() const;

            int resolve_package(package *config);

            bool is_temp_path() const;

            std::string working_dir() const;
        private:
            int resolve_package_git(package *config);
            int resolve_package_directory(package *config, const char *path);
            int resolve_package_download(package *config);
            int resolve_package_archive(package *config, const char *path);
            std::string location_;
            std::string workingDir_;
            bool isTemp_;
        };
    }
}

#endif

