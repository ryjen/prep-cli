#ifndef MICRANTHA_PREP_UTIL_H
#define MICRANTHA_PREP_UTIL_H

#include <string>
#include <sys/stat.h>

namespace micrantha
{
    namespace prep
    {
        /**
         * runs a command in a forked process
         * @return PREP_SUCCESS or PREP_FAILURE upon error
         */
        int fork_command(const char *argv[], const char *directory, char *const envp[]);

        /**
         * removes an entire directory hierarchy
         */
        int remove_directory(const char *path);

        /**
         * tests if a directory exists
         * @return PREP_SUCCESS or PREP_FAILURE upon error
         */
        int directory_exists(const char *path);

        /**
         * copies an entire directory to another directory
         * @param overwrite set to true to overwrite the to directory
         * @return PREP_SUCESS or PREP_FAILURE upon error
         */
        int copy_directory(const std::string &from, const std::string &to, bool overwrite = false);

        /**
         * tests if a file exists
         */
        bool file_exists(const char *path);

        /**
         * tests if a file is executable
         */
        bool file_executable(const char *path);

        /**
         * makes a temporary directory and assigns the name to the buffer
         * @return the directory name
         */
        char *make_temp_dir(char *buffer, size_t size);

        /**
         * create a directory path
         * @return PREP_SUCCESS or PREP_ERROR upon error
         */
        int mkpath(const char *dir, mode_t mode);

        /**
         * copies one file to another given their names
         */
        int copy_file(const std::string &from, const std::string &to);

        /**
         * builds a platform independent directory path given directory names
         */
        const char *build_sys_path(const char *start, ...) __attribute__((format(printf, 1, 2)));

        /**
         * updates various common shell config files to add prep to the path
         * @return PREP_SUCESS or PREP_FAILURE
         */
        int prompt_to_add_path_to_shell_rc(const char *shellrc, const char *path);

        namespace vt100
        {
            constexpr static const char *const BACK = "\033[1D";

            void update_progress();
        }
        namespace color
        {
            typedef enum { BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE } Type;

            constexpr static const char *const CLEAR = "\033[0m";

            namespace options
            {
                constexpr static const int TERMINATE  = 1;
                constexpr static const int BACKGROUND = (1 << 1);
            }

            std::string colorize(color::Type color, const std::string &value, unsigned attribute = 0,
                                 int flags = options::TERMINATE);
            std::string B(const std::string &value, unsigned attribute = 0, int flags = options::TERMINATE);
            std::string r(const std::string &value, unsigned attribute = 0, int flags = options::TERMINATE);
            std::string g(const std::string &value, unsigned attribute = 0, int flags = options::TERMINATE);
            std::string y(const std::string &value, unsigned attribute = 0, int flags = options::TERMINATE);
            std::string b(const std::string &value, unsigned attribute = 0, int flags = options::TERMINATE);
            std::string m(const std::string &value, unsigned attribute = 0, int flags = options::TERMINATE);
            std::string c(const std::string &value, unsigned attribute = 0, int flags = options::TERMINATE);
            std::string w(const std::string &value, unsigned attribute = 0, int flags = options::TERMINATE);
        }
    }
}

#endif
