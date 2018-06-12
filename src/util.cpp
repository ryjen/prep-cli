
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <fts.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <deque>

#include "common.h"
#include "log.h"

namespace micrantha {
    namespace prep {

        namespace string {

            bool equals(const std::string &left, const std::string &right) {
                return !strcasecmp(left.c_str(), right.c_str());
            }
        }
        namespace io {

            // writes a line to a file descriptor
            ssize_t write_line(int fd, const std::string &line) {
                ssize_t n1 = write(fd, line.c_str(), line.length());

                if (n1 < 0) {
                    return n1;
                }

                auto n2 = write(fd, "\n", 1);

                if (n2 < 0) {
                    return n2;
                }

                return n1 + n2;
            }

            // reads a line from a file descriptor
            ssize_t read_line(int fd, std::string &buf) {
                ssize_t numRead; /* # of bytes fetched by last read() */
                size_t totRead;  /* Total bytes read so far */
                char ch;

                totRead = 0;

                buf.clear();

                for (;;) {
                    numRead = read(fd, &ch, 1);

                    if (numRead == -1) {
                        if (errno == EINTR) /* Interrupted --> restart read() */
                            continue;

                        return -1;
                    }

                    if (numRead == 0) { /* EOF */
                        break;
                    }


                    if (ch != '\n' && ch != '\r') {
                        buf += ch;
                        totRead++;
                        continue;
                    }

                    if (ch == '\n')
                        break;
                }

                return totRead;
            }

            // utility for variadic print
            std::ostream &print(std::ostream &os) {
                return os;
            }
            std::ostream &println(std::ostream &os) {
                os << std::endl;
                return os;
            }
        }


        namespace process {
            int fork_command(const std::string &command, char *const argv[], const char *directory, char *const envp[]) {
                int rval = EXIT_FAILURE;

                pid_t pid = fork();

                if (pid == -1) {
                    log::perror(errno);
                    return EXIT_FAILURE;
                }

                if (pid == 0) {
                    if (directory != nullptr && chdir(directory)) {
                        log::perror("unable to change directory ", directory);
                        return EXIT_FAILURE;
                    }

                    // we are the child
                    execve(command.c_str(), argv, envp);

                    exit(EXIT_FAILURE); // exec never returns
                } else {
                    int status = 0;

                    rval = waitpid(pid, &status, 0);

                    if (rval == -1) {
                        log::perror("child process did not return");
                        return EXIT_FAILURE;
                    }

                    if (WIFEXITED(status)) {
                        rval = WEXITSTATUS(status);
                    } else if (WIFSIGNALED(status)) {
                        int sig = WTERMSIG(status);

                        log::perror("child process signal ", sig);

                        if (WCOREDUMP(status)) {
                            log::perror("child produced core dump");
                        }
                    } else if (WIFSTOPPED(status)) {
                        int sig = WSTOPSIG(status);

                        log::perror("child process stopped signal ", sig);
                    } else {
                        log::perror("child process did not exit cleanly");
                    }
                }

                return rval;
            }
        }

        namespace filesystem {

            int remove_directory(const path &dir) {
                int ret = PREP_SUCCESS;
                FTS *ftsp = nullptr;
                FTSENT *curr = nullptr;

                // Cast needed (in C) because fts_open() takes a "char * const *", instead
                // of a "const char * const *", which is only allowed in C++. fts_open()
                // does not modify the argument.
                char *files[] = {const_cast<char *>(dir.c_str()), nullptr};

                // FTS_NOCHDIR  - Avoid changing cwd, which could cause unexpected behavior
                //                in multithreaded programs
                // FTS_PHYSICAL - Don't follow symlinks. Prevents deletion of files outside
                //                of the specified directory
                // FTS_XDEV     - Don't cross filesystem boundaries
                ftsp = fts_open(files, FTS_NOCHDIR | FTS_PHYSICAL | FTS_XDEV, nullptr);

                if (!ftsp) {
                    log::perror("failed to open ", dir);
                    return PREP_FAILURE;
                }

                while ((curr = fts_read(ftsp))) {
                    switch (curr->fts_info) {
                        case FTS_NS:
                        case FTS_DNR:
                        case FTS_ERR:
                            log::error(curr->fts_accpath, ": ", strerror(curr->fts_errno));
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
                                log::perror(curr->fts_path, ": Failed to remove");
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

            int directory_exists(const path &path) {
                struct stat s{
                };
                int err = stat(path.c_str(), &s);
                if (-1 == err) {
                    if (ENOENT == errno) {
                        /* does not exist */
                        return PREP_ERROR;
                    } else {
                        log::perror(path);
                        exit(1);
                    }
                } else {
                    if (S_ISDIR(s.st_mode)) {
                        /* it's a dir */
                        return PREP_SUCCESS;
                    } else {
                        return PREP_FAILURE;
                    }
                }
            }

            int directory_empty(const path &path) {
                FTS *ftsp = nullptr;
                FTSENT *curr = nullptr;

                // Cast needed (in C) because fts_open() takes a "char * const *", instead
                // of a "const char * const *", which is only allowed in C++. fts_open()
                // does not modify the argument.
                char *files[] = {const_cast<char *>(path.c_str()), nullptr};

                // FTS_NOCHDIR  - Avoid changing cwd, which could cause unexpected behavior
                //                in multithreaded programs
                // FTS_PHYSICAL - Don't follow symlinks. Prevents deletion of files outside
                //                of the specified directory
                // FTS_XDEV     - Don't cross filesystem boundaries
                ftsp = fts_open(files, FTS_NOCHDIR | FTS_COMFOLLOW, nullptr);

                if (!ftsp) {
                    log::perror("failed to open ", path);
                    return PREP_ERROR;
                }

                while ((curr = fts_read(ftsp))) {
                    switch (curr->fts_info) {
                        case FTS_NS:
                        case FTS_DNR:
                        case FTS_ERR:
                            log::error(curr->fts_accpath, ": ", strerror(curr->fts_errno));
                            fts_close(ftsp);
                            return PREP_ERROR;

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
                           fts_close(ftsp);
                           return PREP_FAILURE;
                        default:
                            break;
                    }
                }

                fts_close(ftsp);

                return PREP_SUCCESS;
            }

            int file_exists(const path &path) {
                struct stat s{
                };
                int err = stat(path.c_str(), &s);
                if (-1 == err) {
                    if (ENOENT != errno) {
                        perror("stat");
                    }
                    /* does not exist */
                    return PREP_ERROR;
                } else {
                    if (S_ISREG(s.st_mode)) {
                        /* it's a file */
                        return PREP_SUCCESS;
                    } else {
                        return PREP_FAILURE;
                    }
                }
            }

            bool is_file_executable(const path &path) {
                struct stat s{
                };
                int err = stat(path.c_str(), &s);
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

            int create_path(const path &path, mode_t mode) {
                char *p = nullptr;
                char *file_path = nullptr;

                if (path.empty()) {
                    return PREP_FAILURE;
                }

                file_path = const_cast<char *>(path.c_str());

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

            int copy_file(const path &from, const path &to) {
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

                if (directory_exists(from) != PREP_SUCCESS) {
                    log::perror(from, " is not a directory");
                    return PREP_FAILURE;
                }

                if (directory_exists(to) != PREP_SUCCESS) {
                    if (create_path(to, 0777)) {
                        log::perror(errno);
                        return PREP_FAILURE;
                    }
                }

                char *const paths[] = {(char *const) from.c_str(), nullptr};

                file_system = fts_open(paths, FTS_COMFOLLOW | FTS_NOCHDIR, nullptr);

                if (file_system == nullptr) {
                    log::perror("unable to open file system");
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
                                log::perror("non-directory file already found for ", buf);
                                rval = PREP_FAILURE;
                            }
                            continue;
                        }

                        // doesn't exist, create the repo path directory
                        if (mkdir(buf, 0777)) {
                            log::perror("Could not create ", buf);
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

            void build_path(std::ostream &buf) {}

            path make_temp_dir() {
                char buf[BUFSIZ] = {0};

                strncpy(buf, "/tmp/prep-XXXXXX", BUFSIZ);

                auto temp = mkdtemp(buf);

                if (temp == nullptr) {
                    return "";
                }

                return temp;
            }

        }
    }
}

