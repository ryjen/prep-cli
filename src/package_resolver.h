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
            package_resolver();

            int resolve_package(package &config, const options &opts, const std::string &path);

            bool is_temp_path() const;

            std::string working_dir() const;
        private:
            int resolve_package_git(package &config, const options &opts);
            int resolve_package_directory(package &config, const options &opts, const char *path);
            int resolve_package_download(package &config, const options &opts);
            int resolve_package_archive(package &config, const options &opts, const char *path);
            string workingDir_;
            bool isTemp_;

            static const char *const PACKAGE_FILE;
        };
    }
}

#endif

