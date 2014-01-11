
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <cerrno>
#include <sys/param.h>

namespace arg3
{
    namespace cpppm
    {

        int make_temp_file(char *buffer, size_t size)
        {
            strncpy(buffer, "/tmp/cpppm-XXXXXX", size);

            return mkstemp (buffer);
        }

        char *make_temp_dir(char *buffer, size_t size)
        {
            strncpy(buffer, "/tmp/cpppm-XXXXXX", size);

            return mkdtemp (buffer);
        }

        int pipe_command(const char *buf, const char *directory)
        {

            char cur_dir[MAXPATHLEN + 1] = {0};

            if (directory)
            {
                if (!getcwd(cur_dir, MAXPATHLEN))
                {
                    return EXIT_FAILURE;
                }

                if (chdir(directory))
                {
                    return EXIT_FAILURE;
                }
            }

            FILE *out = popen(buf, "r");
            int rval = EXIT_FAILURE;

            if (out != NULL)
            {
                char line[BUFSIZ + 1] = {0};

                while (fgets(line, BUFSIZ, out) != NULL)
                    fputs(line, stdout);

                if (pclose(out) != -1)
                    rval = EXIT_SUCCESS;
            }

            if (directory)
            {
                chdir(cur_dir);
            }
            return rval;
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


        int directory_exists(const char *path)
        {
            struct stat s;
            int err = stat(path, &s);
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

    }
}