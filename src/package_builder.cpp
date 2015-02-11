#include "package_builder.h"
#include "package_resolver.h"
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
    namespace prep
    {

        const char *const package_builder::LOCAL_REPO = ".prep";

        const char *const package_builder::GLOBAL_REPO = "/usr/local/share/prep";

        string package_builder::get_path() const
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

        void package_builder::set_global(bool value)
        {
            global_ = value;
        }

        void package_builder::initialize()
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

        int package_builder::build_package(const package &config, const char *path)
        {
            if (!strcmp(config.build_system(), "autotools"))
            {
                return build_autotools(path);
            }
            else if (!strcmp(config.build_system(), "cmake"))
            {
                return build_cmake(path);
            }
            else
            {
                printf("unknown build system '%s'", config.build_system());
                return EXIT_FAILURE;
            }

        }

        int package_builder::build(const package_config &config, const char *path)
        {
            assert(config.is_loaded());

            for (package_dependency &p : config.dependencies())
            {
                printf("Building dependency %s...\n", p.name());

                package_resolver resolver;

                resolver.set_location(p.location());

                if (resolver.resolve_package(&p))
                {
                    return EXIT_FAILURE;
                }

                if (build_package(p, resolver.working_dir().c_str()))
                {
                    return EXIT_FAILURE;
                }
            }

            return build_package(config, path);
        }

        const char *const package_builder::get_home_dir() const
        {
            char *homedir = getenv("HOME");

            if (homedir == NULL)
            {
                uid_t uid = getuid();
                struct passwd *pw = getpwuid(uid);

                if (pw == NULL)
                {
                    printf("Failed to get user home directory\n");
                    exit(EXIT_FAILURE);
                }

                homedir = pw->pw_dir;
            }

            return homedir;
        }

        int package_builder::build_cmake(const char *path)
        {
            char buf[BUFSIZ + 1] = {0};

            snprintf(buf, BUFSIZ, "-DCMAKE_INSTALL_PREFIX:PATH=%s", get_path().c_str());

            const char *cmake_args[] = { "/bin/sh", "-c", "cmake", buf, ".", NULL };

            if (fork_command(cmake_args, path))
            {
                perror("unable to execute cmake");
                return EXIT_FAILURE;
            }

            const char *make_args[] = { "/bin/sh", "-c", "make", "install", NULL };

            if (fork_command(make_args, path))
            {
                perror("unable to execute make");
                return EXIT_FAILURE;
            }

            return EXIT_SUCCESS;
        }

        int package_builder::build_autotools(const char *path)
        {
            char buf[BUFSIZ + 1] = {0};

            snprintf(buf, BUFSIZ, "--prefix=%s", get_path().c_str());

            const char *configure_args[] = { "/bin/sh", "-c", "./configure", buf, NULL };

            if (fork_command(configure_args, path))
            {
                perror("unable to execute configure");
                return EXIT_FAILURE;
            }

            const char *make_args[] = { "/bin/sh", "-c", "make", "install", NULL };

            if (fork_command(make_args, path))
            {
                perror("unable to execute make");
                return EXIT_FAILURE;
            }

            return EXIT_SUCCESS;
        }

        int package_builder::build_from_folder(const char *path)
        {
            package_config config;

            if (config.load(path))
            {
                return build(config, path);
            }
            else
            {
                printf("Error %s is not a valid prep package\n", path);
                return 1;
            }
        }
    }
}