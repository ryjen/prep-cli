#ifndef RJ_PREP_PACKAGE_RESOLVER_H
#define RJ_PREP_PACKAGE_RESOLVER_H

#include <string>

#include "package_config.h"

namespace rj
{
    namespace prep
    {
        /* resolves a package from a download, git, archive or folder */
        class package_resolver
        {
           public:
            package_resolver();

            int resolve_package(package &config, const options &opts);
            int resolve_package(package &config, const options &opts, const std::string &path);

            bool is_temp_path() const;

            std::string package_dir() const;

           private:
            int resolve_package_git(package &config, const options &opts, const char *url);
            int resolve_package_directory(package &config, const options &opts, const char *path);
            int resolve_package_download(package &config, const options &opts, const char *url);
            int resolve_package_archive(package &config, const options &opts, const char *path);
            string directory_;
            bool isTemp_;

            static const char *const PACKAGE_FILE;
        };
    }
}

#endif
