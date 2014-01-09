#include "repository.h"
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <dirent.h>
#include <git2/clone.h>
#include <git2/errors.h>
#include <git2/types.h>
#include <cstdio>
#include <cassert>
#include <sys/types.h>
#include <pwd.h>
#include <uuid/uuid.h>
#include <iostream>

namespace arg3
{
    namespace cpppm
    {
        typedef struct
        {

        } progress_data;

        int fetch_progress(
            const git_transfer_progress *stats,
            void *payload)
        {
            progress_data *pd = (progress_data *)payload;

            printf("%d/%d %.0f%%\n", stats->indexed_objects, stats->total_objects, (float) stats->indexed_objects / (float) stats->total_objects * 100.f);

            return 0;
        }

        void checkout_progress(
            const char *path,
            size_t cur,
            size_t tot,
            void *payload)
        {
            progress_data *pd = (progress_data *)payload;
            printf("%ld/%ld %.0f%%\n", cur, tot, (float) cur / (float) tot * 100.f);

        }

        int remove_directory(const char *path)
        {
            DIR *d = opendir(path);
            size_t path_len = strlen(path);
            int r = -1;

            if (d)
            {
                struct dirent *p;

                r = 0;

                while (!r && (p = readdir(d)))
                {
                    int r2 = -1;
                    char *buf;
                    size_t len;

                    /* Skip the names "." and ".." as we don't want to recurse on them. */
                    if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
                    {
                        continue;
                    }

                    len = path_len + strlen(p->d_name) + 2;
                    buf = (char *) calloc(1, len);

                    if (buf)
                    {
                        struct stat statbuf;

                        snprintf(buf, len, "%s/%s", path, p->d_name);

                        if (!stat(buf, &statbuf))
                        {
                            if (S_ISDIR(statbuf.st_mode))
                            {
                                r2 = remove_directory(buf);
                            }
                            else
                            {
                                r2 = unlink(buf);
                            }
                        }

                        free(buf);
                    }

                    r = r2;
                }

                closedir(d);
            }

            if (!r)
            {
                r = rmdir(path);
            }

            return r;
        }

        int pipe_command(const char *buf)
        {
            FILE *out = popen(buf, "r");

            if (out == NULL)
            {
                perror("unable to run configuration");
                exit(1);
            }

            size_t cap = 0;
            char *line = NULL;
            ssize_t size;

            while ((size = getline(&line, &cap, out)) > 0)
                fwrite(line, cap, 1, stdout);

            if (pclose(out) == -1)
            {
                perror("could not build");
                return 1;
            }
            return 0;
        }

        const char *const repository::LOCAL_REPO = ".cpppm";

        const char *const repository::GLOBAL_REPO = "/usr/local/share/cpppm";

        string repository::get_path() const
        {
            if (global_)
                return GLOBAL_REPO;
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
            mkdir(get_path().c_str(), 0700);

            char *path = getenv("PATH");

            char *dir = strtok(path, ":");

            string repo_path = get_path();

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

        int repository::get_from_git(const std::string &url)
        {
            char buffer [BUFSIZ + 1] = {0};
            strncpy(buffer, "/tmp/cpppm-XXXXXX", BUFSIZ);

            mkdtemp (buffer);

            progress_data d = {};
            git_clone_options opts = GIT_CLONE_OPTIONS_INIT;
            git_checkout_opts checkout_opts = GIT_CHECKOUT_OPTS_INIT;

            checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE_CREATE;
            checkout_opts.progress_cb = checkout_progress;
            checkout_opts.progress_payload = &d;
            opts.checkout_opts = checkout_opts;
            opts.remote_callbacks.transfer_progress = &fetch_progress;
            opts.remote_callbacks.payload = &d;

            git_repository *repo = NULL;
            int error = git_clone(&repo, url.c_str(), buffer, &opts);
            if (error < 0)
            {
                const git_error *e = giterr_last();
                printf("Error %d/%d: %s\n", error, e->klass, e->message);
                return 1;
            }
            git_repository_free(repo);

            int rval = get_from_folder(buffer);

            remove_directory(buffer);

            return rval;
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