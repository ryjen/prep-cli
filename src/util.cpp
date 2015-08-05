#include "config.h"
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
#include <libgen.h>
#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#include <curl/easy.h>
#endif
#include <string>
#include "log.h"

namespace arg3
{
    namespace prep
    {
        bool str_cmp(const char *astr, const char *bstr) {
            if (astr == NULL || bstr == NULL) {
                return true;
            }

            return strcasecmp(astr, bstr);
        }

        int make_temp_file(char *buffer, size_t size)
        {
            strncpy(buffer, "/tmp/prep-XXXXXX", size);

            return mkstemp (buffer);
        }

        char *make_temp_dir(char *buffer, size_t size)
        {
            strncpy(buffer, "/tmp/prep-XXXXXX", size);

            return mkdtemp (buffer);
        }

        int fork_command(const char *argv[], const char *directory, char *const envp[])
        {
            int rval = EXIT_FAILURE;

            pid_t pid = fork();

            if (pid == -1)
            {
                log_errno(errno);
                return EXIT_FAILURE;
            }

            if (pid == 0)
            {
                if (chdir(directory))
                {
                    log_error("unable to change directory [%s]", directory);
                    return EXIT_FAILURE;
                }

                for (int i = 0; argv[i] != NULL; i++) {
                    printf("%s ", argv[i]);
                }
                puts(":");

                // we are the child
                execve(argv[0], (char *const *) &argv[0], envp);

                exit(EXIT_FAILURE);   // exec never returns
            }
            else
            {
                int status = 0;

                waitpid(pid, &status, 0);

                if (WIFEXITED(status))
                {
                    rval = WEXITSTATUS(status);
                }
                else
                {
                    log_error("child process did not exit cleanly");
                }
            }

            return rval;
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

                while (fgets(line, BUFSIZ, out) != NULL) {
                    fputs(line, stdout);
                }

                if (pclose(out) != -1) {
                    rval = EXIT_SUCCESS;
                }
            }

            if (directory) {
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
                    log_error("%s [%s]", strerror(errno), path);
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

        int file_exists(const char *path)
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
                if (S_ISREG(s.st_mode))
                {
                    /* it's a dir */
                    return 1;
                }
                else
                {
                    return 0;
                }
            }
        }

        int mkpath(const char *dir, mode_t mode)
        {
            struct stat sb;
            char buf[PATH_MAX] = {0};
            char *slash = NULL;
            int done = 0;

            if (!dir) {
                errno = EINVAL;
                return 1;
            }

            strncpy(buf, dir, PATH_MAX);

            slash = buf;

            while (!done) {
                slash += strspn(slash, "/");
                slash += strcspn(slash, "/");

                done = (*slash == '\0');
                *slash = '\0';

                if (stat(buf, &sb)) {
                    if (errno != ENOENT || (mkdir(buf, mode) &&
                        errno != EEXIST)) {
                        log_warn("%s", buf);
                        return (-1);
                    }
                } else if (!S_ISDIR(sb.st_mode)) {
                    log_warn("%s: %s", buf, strerror(ENOTDIR));
                    return (-1);
                }

                *slash = '/';
            }

            return (0);
        }

#ifdef HAVE_LIBCURL
        size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
        {
            size_t written = fwrite(ptr, size, nmemb, stream);
            return written;
        }
#endif

        int download_to_temp_file(const char *url, std::string &filename)
        {
#ifdef HAVE_LIBCURL
            char temp [BUFSIZ + 1] = {0};
            FILE *fp = NULL;
            int fd = -1, httpCode = 0;
            CURLcode res;
            CURL *curl = curl_easy_init();

            if (curl == NULL || url == NULL || *url == '\0') {
                return EXIT_FAILURE;
            }

            log_debug("Downloading %s", url);

            strncpy(temp, "/tmp/prep-XXXXXX", BUFSIZ);

            if ((fd = mkstemp (temp)) < 0 || (fp = fdopen(fd, "wb")) == NULL) {
                return EXIT_FAILURE;
            }

            filename = temp;

            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            fclose(fp);

            if (res != CURLE_OK) {
                log_error("%s", curl_easy_strerror(res));
                return EXIT_FAILURE;
            }

            res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

            if (res != CURLE_OK || httpCode != 200) {
                log_error("%d code from server", httpCode);
                return EXIT_FAILURE;
            }

            return EXIT_SUCCESS;
#else
            log_error("libcurl not installed or configured");
            return EXIT_FAILURE;
#endif
        }

    }
}