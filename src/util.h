#ifndef MICRANTHA_PREP_UTIL_H
#define MICRANTHA_PREP_UTIL_H

#include <string>
#include <sys/stat.h>
#include <sstream>

namespace micrantha
{
    namespace prep
    {
        class CStringVector : protected std::vector<char *> {
        public:
            typedef std::vector<char *> container;
            typedef container::value_type value_type;
            typedef container::reference reference;
            typedef container::const_reference  const_reference;

            CStringVector();
            CStringVector(const std::vector<std::string> &args);
            ~CStringVector();
            CStringVector(const CStringVector &) = delete;
            CStringVector(CStringVector &&) = default;
            CStringVector &operator=(const CStringVector &) = delete;
            CStringVector &operator=(CStringVector &&) = default;

            CStringVector &add(const std::string &value);

            char * const *data() const;

            reference operator[](size_t index);

            const_reference operator[](size_t index) const;
        };

        /**
         * runs a command in a forked process
         * @return PREP_SUCCESS or PREP_FAILURE upon error
         */
        int fork_command(const CStringVector &argv, const char *directory = nullptr, const CStringVector &envp = CStringVector());

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
#ifdef _WIN32
                buf << "\\";
#else
                buf << "/";
#endif
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

        /**
         * builds a platform independent directory path given directory names
         */
//        const char *build_sys_path(const char *start, ...) __attribute__((format(printf, 1, 2)));
    }
}

#endif
