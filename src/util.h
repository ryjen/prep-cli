#ifndef MICRANTHA_PREP_UTIL_H
#define MICRANTHA_PREP_UTIL_H

#include <string>
#include <sys/stat.h>
#include <sstream>

namespace micrantha
{
    namespace prep
    {

#ifdef _WIN32
        constexpr const char *const DIR_SYM = "\\";
#else
        constexpr const char *const DIR_SYM = "/";
#endif
        /**
         * runs a command in a forked process
         * @return PREP_SUCCESS or PREP_FAILURE upon error
         */
        int fork_command(const std::string &command, char *const argv[], const char *directory, char *const envp[]);

        /**
         * removes an entire directory hierarchy
         */
        int remove_directory(const std::string &path);

        /**
         * tests if a directory exists
         * @return PREP_SUCCESS if exists, PREP_FAILURE if it doesn't or PREP_ERROR upon error
         */
        int directory_exists(const std::string &path);

        /**
         * copies an entire directory to another directory
         * @param overwrite set to true to overwrite the to directory
         * @return PREP_SUCESS or PREP_FAILURE upon error
         */
        int copy_directory(const std::string &from, const std::string &to, bool overwrite = false);

        /**
         * tests if a file exists
         */
        bool file_exists(const std::string &path);

        /**
         * tests if a file is executable
         */
        bool file_executable(const std::string &path);

        /**
         * makes a temporary directory and assigns the name to the buffer
         * @return the directory name
         */
        std::string make_temp_dir();

        /**
         * posix standard forkpty()
         * @param master the master file descriptor
         * @return the process id of the child process and zero for the parent process
         */
        pid_t posix_forkpty(int *master, char *name, struct termios *termp, struct winsize *winp);

        /**
         * create a directory path
         * @return PREP_SUCCESS or PREP_ERROR upon error
         */
        int mkpath(const std::string &dir, mode_t mode);

        /**
         * copies one file to another given their names
         */
        int copy_file(const std::string &from, const std::string &to);

        namespace internal {

            void build_sys_path(std::ostream &buf);

            template<class A0, class... Args>
            void build_sys_path(std::ostream &buf, const A0 &path, const Args &... args) {

                buf << DIR_SYM;

                buf << path;

                build_sys_path(buf, args...);
            }
        }

        template <class... Args>
        std::string build_sys_path(const std::string &path, const Args &... args)
        {
            std::ostringstream buf;
            buf << path;
            internal::build_sys_path(buf, args...);
            return buf.str();
        }
    }
}

#endif
