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
         * downloads a url endpoint to a temporary file
         * NOTE: required libcurl enabled
         * @return PREP_SUCCESS or PREP_FAILURE upon error
         */
        int download_to_temp_file(const char *url, std::string &filename);

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
    }
}

#endif
