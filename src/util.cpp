#include <dirent.h>
#include <fts.h>
#include <libgen.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include "config.h"
#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#include <curl/easy.h>
#endif
#include <pwd.h>
#include <string>
#include "common.h"
#include "log.h"

namespace arg3
{
    namespace prep
    {
        bool str_cmp(const char *astr, const char *bstr)
        {
            if (astr == NULL || bstr == NULL) {
                return true;
            }

            return strcasecmp(astr, bstr);
        }

        bool str_empty(const char *astr)
        {
            return astr == NULL || *astr == '\0';
        }

        int make_temp_file(char *buffer, size_t size)
        {
            strncpy(buffer, "/tmp/prep-XXXXXX", size);

            return mkstemp(buffer);
        }

        char *make_temp_dir(char *buffer, size_t size)
        {
            strncpy(buffer, "/tmp/prep-XXXXXX", size);

            return mkdtemp(buffer);
        }

        int fork_command(const char *argv[], const char *directory, char *const envp[])
        {
            int rval = EXIT_FAILURE;

            pid_t pid = fork();

            if (pid == -1) {
                log_errno(errno);
                return EXIT_FAILURE;
            }

            if (pid == 0) {
                if (chdir(directory)) {
                    log_error("unable to change directory [%s]", directory);
                    return EXIT_FAILURE;
                }

                // we are the child
                execve(argv[0], (char *const *)&argv[0], envp);

                exit(EXIT_FAILURE);  // exec never returns
            } else {
                int status = 0;

                waitpid(pid, &status, 0);

                if (WIFEXITED(status)) {
                    rval = WEXITSTATUS(status);
                } else if (WIFSIGNALED(status)) {
                    int sig = WTERMSIG(status);

                    log_error("child process signal %d", sig);

                    if (WCOREDUMP(status)) {
                        log_error("child produced core dump");
                    }
                } else if (WIFSTOPPED(status)) {
                    int sig = WSTOPSIG(status);

                    log_error("child process stopped signal %d", sig);
                } else {
                    log_error("child process did not exit cleanly");
                }
            }

            return rval;
        }

        int pipe_command(const char *buf, const char *directory)
        {
            char cur_dir[MAXPATHLEN + 1] = {0};

            if (directory) {
                if (!getcwd(cur_dir, MAXPATHLEN)) {
                    return EXIT_FAILURE;
                }

                if (chdir(directory)) {
                    return EXIT_FAILURE;
                }
            }

            FILE *out = popen(buf, "r");
            int rval = EXIT_FAILURE;

            if (out != NULL) {
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

        int remove_directory(const char *dir)
        {
            int ret = PREP_SUCCESS;
            FTS *ftsp = NULL;
            FTSENT *curr = NULL;

            // Cast needed (in C) because fts_open() takes a "char * const *", instead
            // of a "const char * const *", which is only allowed in C++. fts_open()
            // does not modify the argument.
            char *files[] = {(char *)dir, NULL};

            // FTS_NOCHDIR  - Avoid changing cwd, which could cause unexpected behavior
            //                in multithreaded programs
            // FTS_PHYSICAL - Don't follow symlinks. Prevents deletion of files outside
            //                of the specified directory
            // FTS_XDEV     - Don't cross filesystem boundaries
            ftsp = fts_open(files, FTS_NOCHDIR | FTS_PHYSICAL | FTS_XDEV, NULL);

            if (!ftsp) {
                log_error("failed to open %s [%s]", dir, strerror(errno));
                return PREP_FAILURE;
            }

            while ((curr = fts_read(ftsp))) {
                switch (curr->fts_info) {
                    case FTS_NS:
                    case FTS_DNR:
                    case FTS_ERR:
                        log_error("%s: fts_read error: %s", curr->fts_accpath, strerror(curr->fts_errno));
                        break;

                    case FTS_DC:
                    case FTS_DOT:
                    case FTS_NSOK:
                        // Not reached unless FTS_LOGICAL, FTS_SEEDOT, or FTS_NOSTAT were
                        // passed to fts_open()
                        break;

                    case FTS_D:
                        // Do nothing. Need depth-first search, so directories are deleted
                        // in FTS_DP
                        break;

                    case FTS_DP:
                    case FTS_F:
                    case FTS_SL:
                    case FTS_SLNONE:
                    case FTS_DEFAULT:
                        if (remove(curr->fts_accpath) < 0) {
                            log_error("%s: Failed to remove: %s", curr->fts_path, strerror(errno));
                            ret = PREP_FAILURE;
                        }
                        break;
                }
            }

            if (ftsp) {
                fts_close(ftsp);
            }

            return ret;
        }

        int directory_exists(const char *path)
        {
            struct stat s;
            int err = stat(path, &s);
            if (-1 == err) {
                if (ENOENT == errno) {
                    /* does not exist */
                    return 0;
                } else {
                    log_error("%s [%s]", strerror(errno), path);
                    exit(1);
                }
            } else {
                if (S_ISDIR(s.st_mode)) {
                    /* it's a dir */
                    return 1;
                } else {
                    return 2;
                }
            }
        }

        int file_exists(const char *path)
        {
            struct stat s;
            int err = stat(path, &s);
            if (-1 == err) {
                if (ENOENT == errno) {
                    /* does not exist */
                    return 0;
                } else {
                    perror("stat");
                    exit(1);
                }
            } else {
                if (S_ISREG(s.st_mode)) {
                    /* it's a dir */
                    return 1;
                } else {
                    return 0;
                }
            }
        }

        int mkpath(const char *file_path, mode_t mode)
        {
            char *p;
            if (!file_path || !*file_path) {
                return 1;
            }
            for (p = strchr(file_path + 1, '/'); p; p = strchr(p + 1, '/')) {
                *p = '\0';
                if (mkdir(file_path, mode) == -1) {
                    if (errno != EEXIST) {
                        *p = '/';
                        return -1;
                    }
                }
                *p = '/';
            }
            if (mkdir(file_path, mode) == -1) {
                if (errno != EEXIST) {
                    return -1;
                }
            }
            return 0;
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
            char temp[BUFSIZ + 1] = {0};
            FILE *fp = NULL;
            int fd = -1, httpCode = 0;
            CURLcode res;
            CURL *curl = curl_easy_init();

            if (curl == NULL || url == NULL || *url == '\0') {
                return EXIT_FAILURE;
            }

            log_debug("Downloading %s", url);

            strncpy(temp, "/tmp/prep-XXXXXX", BUFSIZ);

            if ((fd = mkstemp(temp)) < 0 || (fp = fdopen(fd, "wb")) == NULL) {
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

            if (res != CURLE_OK || httpCode < 200 || httpCode >= 300) {
                log_error("%d code from server", httpCode);
                return EXIT_FAILURE;
            }

            return EXIT_SUCCESS;
#else
            log_error("libcurl not installed or configured");
            return EXIT_FAILURE;
#endif
        }

        int copy_file(const std::string &from, const std::string &to)
        {
            std::ifstream src(from, std::ios::binary);

            if (!src.is_open()) {
                return PREP_FAILURE;
            }

            std::ofstream dst(to, std::ios::binary);

            if (!dst.is_open()) {
                return PREP_FAILURE;
            }

            dst << src.rdbuf();

            return PREP_SUCCESS;
        }

        std::string get_user_home_dir()
        {
            static std::string home_dir_path;

            if (home_dir_path.empty()) {
                char *homedir = getenv("HOME");

                if (homedir == NULL) {
                    uid_t uid = getuid();
                    struct passwd *pw = getpwuid(uid);

                    if (pw == NULL) {
                        log_error("Failed to get user home directory");
                        exit(PREP_FAILURE);
                    }

                    homedir = pw->pw_dir;
                }

                home_dir_path = homedir;
            }

            return home_dir_path;
        }

        const char *build_sys_path(const char *start, ...)
        {
            static char sbuf[3][PATH_MAX + 1];
            static int i = 0;

            i++, i %= 3;

            va_list args;
            char *buf = sbuf[i];

            memset(buf, 0, PATH_MAX);

            va_start(args, start);

            if (start != NULL) {
                strcat(buf, start);
            }

            const char *next = va_arg(args, const char *);

            while (next != NULL) {
#ifdef _WIN32
                if (buf[strlen(buf) - 1] != '\\') strcat(buf, "\\");
#else
                if (buf[strlen(buf) - 1] != '/') strcat(buf, "/");
#endif
                strcat(buf, next);

                next = va_arg(args, const char *);
            }

            va_end(args);

            return buf;
        }

        int prompt_to_add_path_to_shell_rc(const char *shellrc, const char *path)
        {
            char buf[BUFSIZ] = {0};

            char *home = getenv("HOME");

            if (!home) {
                return PREP_FAILURE;
            }

            snprintf(buf, BUFSIZ, "%s/%s", home, shellrc);

            if (!file_exists(buf)) {
                return PREP_FAILURE;
            }

            printf("Would you like to add prep to your PATH in %s? (y/N) ", shellrc);

            int ch = getchar();

            if (ch != 10 || toupper(ch) == 'Y') {
                std::ofstream rcfile(buf, std::ios::app);

                rcfile << "export PATH=\"$PATH:" << path << "\"";
            }

            return PREP_SUCCESS;
        }

        bool can_exec_file(const std::string &file)
        {
            struct stat  st;

            if (stat(file.c_str(), &st) < 0)
                return false;
            if ((st.st_mode & S_IEXEC) != 0)
                return true;
            return false;
        }
    }
}
