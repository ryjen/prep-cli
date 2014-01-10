#include "repository.h"
#include <cerrno>
#include <cassert>
#include <pwd.h>
#include <uuid/uuid.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include "util.h"

namespace arg3
{
    namespace cpppm
    {

        const char *const repository::LOCAL_REPO = ".cpppm";

        const char *const repository::GLOBAL_REPO = "/usr/local/share/cpppm";

        string repository::get_path() const
        {
            if (global_)
            {
                return GLOBAL_REPO;
            }
            else
            {
                string buf(get_home_dir());
                buf += "/";
                buf += LOCAL_REPO;
                return buf;
            }
        }

        void repository::set_global(bool value)
        {
            global_ = value;
        }

        void repository::initialize()
        {
            string repo_path = get_path();

            mkdir(repo_path.c_str(), 0700);

            char *path = getenv("PATH");

            char *dir = strtok(path, ":");

            while (dir != NULL)
            {
                if (repo_path == dir)
                    break;

                dir = strtok(NULL, ":");
            }

            if (dir == NULL)
            {
                std::cout << get_path() << " is not added to your PATH, you should set this to include packages\n";
            }
        }

        int repository::get(const package_config &config)
        {
            assert(config.is_loaded());

            if (!strcmp(config.build_system(), "autotools"))
            {
                return build_autotools(config.path().c_str());
            }
            else if (!strcmp(config.build_system(), "cmake"))
            {
                return build_cmake(config.path().c_str());
            }

            printf("unknown build system\n");
            return 1;
        }

        const char *const repository::get_home_dir() const
        {
            char *homedir = getenv("HOME");

            if (homedir == NULL)
            {
                uid_t uid = getuid();
                struct passwd *pw = getpwuid(uid);

                if (pw == NULL)
                {
                    printf("Failed to get user home directory\n");
                    return "";
                }

                homedir = pw->pw_dir;
            }

            return homedir;
        }

        int repository::build_cmake(const char *path)
        {
            char buf[BUFSIZ + 1] = {0};

            snprintf(buf, BUFSIZ, "%s/cmake -DCMAKE_INSTALL_PREFIX:PATH=%s/%s . && make all install", path, get_home_dir(), get_path().c_str());

            return pipe_command(buf);
        }

        int repository::build_autotools(const char *path)
        {
            char buf[BUFSIZ + 1] = {0};

            snprintf(buf, BUFSIZ, "%s/configure --prefix=%s/%s && make all install", path, get_home_dir(), get_path().c_str());

            return pipe_command(buf);
        }

        int repository::get_from_folder(const char *path)
        {
            package_config config(path);

            if (config.load())
            {
                return get(config);
            }
            else
            {
                printf("Error %s is not a valid cpppm package\n", path);
                return 1;
            }
        }
    }
}