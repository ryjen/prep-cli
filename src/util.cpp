
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <fts.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"
#include "log.h"
#include "util.h"

namespace micrantha {
    namespace prep {

        char *make_temp_dir(char *buffer, size_t size) {
            strncpy(buffer, "/tmp/prep-XXXXXX", size);

            return mkdtemp(buffer);
        }

        int fork_command(const char *argv[], const char *directory, char *const envp[]) {
            int rval = EXIT_FAILURE;

            pid_t pid = fork();

            if (pid == -1) {
                log::perror(errno);
                return EXIT_FAILURE;
            }

            if (pid == 0) {
                if (chdir(directory)) {
                    log::perror("unable to change directory [%s]", directory);
                    return EXIT_FAILURE;
                }

                // we are the child
                execve(argv[0], (char *const *) &argv[0], envp);

                exit(EXIT_FAILURE); // exec never returns
            } else {
                int status = 0;

                waitpid(pid, &status, 0);

                if (WIFEXITED(status)) {
                    rval = WEXITSTATUS(status);
                } else if (WIFSIGNALED(status)) {
                    int sig = WTERMSIG(status);

                    log::perror("child process signal %d", sig);

                    if (WCOREDUMP(status)) {
                        log::perror("child produced core dump");
                    }
                } else if (WIFSTOPPED(status)) {
                    int sig = WSTOPSIG(status);

                    log::perror("child process stopped signal %d", sig);
                } else {
                    log::perror("child process did not exit cleanly");
                }
            }

            return rval;
        }

        int remove_directory(const char *dir) {
            int ret = PREP_SUCCESS;
            FTS *ftsp = nullptr;
            FTSENT *curr = nullptr;

            // Cast needed (in C) because fts_open() takes a "char * const *", instead
            // of a "const char * const *", which is only allowed in C++. fts_open()
            // does not modify the argument.
            char *files[] = {(char *) dir, nullptr};

            // FTS_NOCHDIR  - Avoid changing cwd, which could cause unexpected behavior
            //                in multithreaded programs
            // FTS_PHYSICAL - Don't follow symlinks. Prevents deletion of files outside
            //                of the specified directory
            // FTS_XDEV     - Don't cross filesystem boundaries
            ftsp = fts_open(files, FTS_NOCHDIR | FTS_PHYSICAL | FTS_XDEV, nullptr);

            if (!ftsp) {
                log::perror("failed to open %s [%s]", dir, strerror(errno));
                return PREP_FAILURE;
            }

            while ((curr = fts_read(ftsp))) {
                switch (curr->fts_info) {
                    case FTS_NS:
                    case FTS_DNR:
                    case FTS_ERR:
                        log::perror("%s: fts_read error: %s", curr->fts_accpath, strerror(curr->fts_errno));
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
                            log::perror("%s: Failed to remove: %s", curr->fts_path, strerror(errno));
                            ret = PREP_FAILURE;
                        }
                        break;
                    default:
                        break;
                }
            }

            fts_close(ftsp);

            return ret;
        }

        int directory_exists(const char *path) {
            struct stat s{
            };
            int err = stat(path, &s);
            if (-1 == err) {
                if (ENOENT == errno) {
                    /* does not exist */
                    return 0;
                } else {
                    log::perror("%s [%s]", strerror(errno), path);
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

        bool file_exists(const char *path) {
            struct stat s{
            };
            int err = stat(path, &s);
            if (-1 == err) {
                if (ENOENT != errno) {
                    perror("stat");
                }
                /* does not exist */
                return false;
            } else {
                if (S_ISREG(s.st_mode)) {
                    /* it's a file */
                    return true;
                } else {
                    return false;
                }
            }
        }

        bool file_executable(const char *path) {
            struct stat s{
            };
            int err = stat(path, &s);
            if (-1 == err) {
                if (ENOENT != errno) {
                    perror("stat");
                }
                /* does not exist */
                return false;
            } else {
                if (S_ISREG(s.st_mode)) {
                    return (s.st_mode & S_IEXEC) != 0;
                } else {
                    return false;
                }
            }
        }

        int mkpath(const char *file_path, mode_t mode) {
            char *p = nullptr;

            if (!file_path || !*file_path) {
                return PREP_FAILURE;
            }
            for (p = const_cast<char *>(strchr(file_path + 1, '/')); p; p = strchr(p + 1, '/')) {
                *p = '\0';
                if (mkdir(file_path, mode) == -1) {
                    if (errno != EEXIST) {
                        *p = '/';
                        return PREP_ERROR;
                    }
                }
                *p = '/';
            }
            if (mkdir(file_path, mode) == -1) {
                if (errno != EEXIST) {
                    return PREP_ERROR;
                }
            }
            return PREP_SUCCESS;
        }

        int copy_file(const std::string &from, const std::string &to) {
            struct stat fst{
            }, tst{};

            if (stat(from.c_str(), &fst)) {
                log::perror(errno);
                return PREP_FAILURE;
            }

            std::ifstream src(from, std::ios::binary);

            if (!src.is_open()) {
                return PREP_FAILURE;
            }

            std::ofstream dst(to, std::ios::binary);

            if (!dst.is_open()) {
                return PREP_FAILURE;
            }

            dst << src.rdbuf();

            src.close();
            dst.close();

            if (stat(to.c_str(), &tst)) {
                log::perror(errno);
                return PREP_FAILURE;
            }

            if (chown(to.c_str(), fst.st_uid, fst.st_gid)) {
                log::perror(errno);
                return PREP_FAILURE;
            }

            if (chmod(to.c_str(), fst.st_mode)) {
                log::perror(errno);
                return PREP_FAILURE;
            }
            return PREP_SUCCESS;
        }

        int copy_directory(const std::string &from, const std::string &to, bool overwrite) {
            FTS *file_system = nullptr;
            FTSENT *parent = nullptr;
            int rval = PREP_SUCCESS;
            char buf[PATH_MAX + 1] = {0};
            struct stat st{
            };

            if (!directory_exists(from.c_str())) {
                log::perror("%s is not a directory", from.c_str());
                return PREP_FAILURE;
            }

            if (!directory_exists(to.c_str())) {
                if (mkdir(to.c_str(), 0777)) {
                    log::perror(errno);
                    return PREP_FAILURE;
                }
            }

            char *const paths[] = {(char *const) from.c_str(), nullptr};

            file_system = fts_open(paths, FTS_COMFOLLOW | FTS_NOCHDIR, nullptr);

            if (file_system == nullptr) {
                log::perror("unable to open file system [%s]", strerror(errno));
                return PREP_FAILURE;
            }

            while ((parent = fts_read(file_system)) != nullptr) {
                // get the repo path
                snprintf(buf, PATH_MAX, "%s%s", to.c_str(), parent->fts_path + from.length());

                // check the install file is a directory
                if (parent->fts_info == FTS_D) {
                    if (stat(buf, &st) == 0) {
                        if (S_ISDIR(st.st_mode)) {
                            log::trace("directory ", buf, " already exists");
                        } else {
                            log::perror("non-directory file already found for %s", buf);
                            rval = PREP_FAILURE;
                        }
                        continue;
                    }

                    // doesn't exist, create the repo path directory
                    if (mkdir(buf, 0777)) {
                        log::perror("Could not create %s [%s]", buf, strerror(errno));
                        rval = PREP_FAILURE;
                        break;
                    }

                    continue;
                }

                if (parent->fts_info != FTS_F) {
                    log::debug("skipping non-regular file ", buf);
                    continue;
                }

                if (!overwrite) {
                    if (stat(buf, &st) == 0) {
                        log::debug("skipping existing regular file file ", buf);
                        continue;
                    }
                }

                log::debug("copying [", parent->fts_path, "] to [", buf, "]");

                if (copy_file(parent->fts_path, buf) == PREP_FAILURE) {
                    log::perror("unable to link [", strerror(errno), "]");
                    rval = PREP_FAILURE;
                    break;
                }
            }

            fts_close(file_system);

            return rval;
        }

        const char *build_sys_path(const char *start, ...) {
            static char sbuf[3][PATH_MAX + 1];
            static int i = 0;

            i++, i %= 3;

            va_list args;
            char *buf = sbuf[i];

            memset(buf, 0, PATH_MAX);

            va_start(args, start);

            if (start != nullptr) {
                strcat(buf, start);
            }

            const char *next = va_arg(args, const char *);

            while (next != nullptr) {
#ifdef _WIN32
                if (buf[strlen(buf) - 1] != '\\')
                    strcat(buf, "\\");
#else
                if (buf[strlen(buf) - 1] != '/')
                    strcat(buf, "/");
#endif
                strcat(buf, next);

                next = va_arg(args, const char *);
            }

            va_end(args);

            return buf;
        }
    }
}

