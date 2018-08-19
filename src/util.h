#ifndef MICRANTHA_PREP_UTIL_H
#define MICRANTHA_PREP_UTIL_H

#include <string>
#include <sys/stat.h>
#include <sstream>

namespace micrantha {
    namespace prep {

        namespace string {
            bool equals(const std::string &left, const std::string &right);
        }

        namespace filesystem {
#ifdef _WIN32
            constexpr const char *const DIR_SYM = "\\";
#else
            constexpr const char *const DIR_SYM = "/";
#endif

            constexpr const static int DIR_PERMS = 755;

            using path = std::string;

            /**
             * removes an entire directory hierarchy
             */
            int remove_directory(const path &path);

            /**
             * tests if a directory exists
             * @return PREP_SUCCESS if exists, PREP_FAILURE if it doesn't or PREP_ERROR upon error
             */
            int directory_exists(const path &path);

            int directory_empty(const path &path);

            /**
             * copies an entire directory to another directory
             * @param overwrite set to true to overwrite the to directory
             * @return PREP_SUCESS or PREP_FAILURE upon error
             */
            int copy_directory(const path &from, const path &to, bool overwrite = false);

            /**
             * tests if a file exists
             */
            int file_exists(const path &path);

            /**
             * tests if a file is executable
             */
            bool is_file_executable(const path &path);

            /**
             * makes a temporary directory and assigns the name to the buffer
             * @return the directory name
             */
            path make_temp_dir();


            /**
             * create a directory path
             * @return PREP_SUCCESS or PREP_ERROR upon error
             */
            int create_path(const path &dir, mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO);

            /**
             * copies one file to another given their names
             */
            int copy_file(const path &from, const path &to);

            void build_path(std::ostream &buf);

            template<class A0, class... Args>
            void build_path(std::ostream &buf, const A0 &path, const Args &... args) {

                buf << DIR_SYM;

                buf << path;

                build_path(buf, args...);
            }

            template<class... Args>
            path build_path(const std::string &path, const Args &... args) {
                std::ostringstream buf;
                buf << path;
                filesystem::build_path(buf, args...);
                return buf.str();
            }
        }

        namespace io {

            /**
             * write a line to a file descriptor
             * @param fd the file descriptor
             * @param line the string to print
             * @return the number of characters written or -1 on error
             */
            ssize_t write_line(int fd, const std::string &line);

            /**
             * reads a line from a file descriptor
             * @param fd the file descriptor
             * @param buf the buffer to store the line
             * @return the number of characters read or -1 on error
             */
            ssize_t read_line(int fd, std::string &buf);

            /**
             * utility method for variadic print
             * @param os the output stream
             * @return the output stream
             */
            std::ostream &print(std::ostream &os);
            std::ostream &println(std::ostream &os);

            /**
             * variadic print
             * @tparam A0 the type of argument
             * @tparam Args the remaining arguments
             * @param os the output stream
             * @param a0 the argument
             * @param args the remaining arguments
             * @return
             */
            template <class A0, class... Args>
            std::ostream &print(std::ostream &os, const A0 &a0, const Args &... args)
            {
                // print the argument
                os << a0;
                // print the remaining arguments
                return print(os, args...);
            }
            template <class A0, class... Args>
            std::ostream &println(std::ostream &os, const A0 &a0, const Args &... args)
            {
                // print the argument
                os << a0;
                // print the remaining arguments
                return println(os, args...);
            }

            /**
             * variadic print
             * @tparam Args the arguments
             * @param os the output stream
             * @param args the arguments
             * @return the output stream
             */
            template <class... Args>
            std::ostream &print(std::ostream &os, const Args &... args)
            {
                // pass the first argument to the printer
                return print(os, args...);
            }
            template <class... Args>
            std::ostream &println(std::ostream &os, const Args &... args)
            {
                // pass the first argument to the printer
                return println(os, args...);
            }
            /**
             * variadic print
             * @tparam Args the type of arguments
             * @param args the arguments
             * @return the output stream
             */
            template <class... Args>
            std::ostream &print(const Args &... args)
            {
                return print(std::cout, args...);
            }
            template <class... Args>
            std::ostream &println(const Args &... args)
            {
                return println(std::cout, args...);
            }
        }

        namespace process {

            constexpr static const int NotFound = 127;
            constexpr static const int NotAvailable = 128;

            /**
             * runs a command in a forked process
             * @return PREP_SUCCESS or PREP_FAILURE upon error
             */
            int fork_command(const std::string &command, char *const argv[], const char *directory, char *const envp[]);

        }

    }
}

#endif
