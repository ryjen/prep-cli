#include "argument_resolver.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

namespace arg3
{
    namespace cpppm
    {

        int directory_exists(const char *path)
        {
            struct stat s;
            int err = stat("/path/to/possible_dir", &s);
            if (-1 == err)
            {
                if (ENOENT == errno)
                {
                    /* does not exist */
                    return 0;
                }
                else
                {
                    perror("stat");
                    exit(1);
                }
            }
            else
            {
                if (S_ISDIR(s.st_mode))
                {
                    /* it's a dir */
                    return 1;
                }
                else
                {
                    return 2;
                }
            }
        }

        argument_resolver::argument_resolver()
        {}

        argument_resolver::argument_resolver(const std::string &arg) : arg_(arg)
        {

        }

        std::string argument_resolver::arg() const
        {
            return arg_;
        }

        void argument_resolver::set_arg(const std::string &arg)
        {
            arg_ = arg;
        }

        int argument_resolver::resolve_package() const
        {
            int fileType = directory_exists(arg);

            if (fileType == 1)
            {
                return resolve_package_directory(arg);
            }
            else if (fileType == 2)
            {
                if (strstr(arg, ".tar.gz") || strstr(arg, ".tgz"))
                {
                    // decompress tarball to temp folder
                    // run on temp folder
                    return resolve_package_tar_gz(arg);
                }
                else
                {
                    return UNKNOWN;
                }
            }
            else if (strstr(arg, "://"))
            {
                if (strstr(arg, ".git") || strstr(arg, "git://"))
                {
                    // clone git to temp folder
                    const char *path = clone_git_repo();

                    if (!path)
                        return UNKNOWN;

                    // run on temp folder
                    return resolve_package_directory(path);
                }
                else
                {
                    // download url to temp file
                    // run on temp file
                }
            }
            else
            {
                return UNKNOWN;
            }
        }
    }
}