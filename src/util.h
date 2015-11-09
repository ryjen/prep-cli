#ifndef INCLUDE_UTIL_H
#define INCLUDE_UTIL_H

#include <string>

namespace arg3
{
    namespace prep
    {
        bool str_cmp(const char *astr, const char *bstr);

        bool str_empty(const char *str);

        int fork_command(const char *argv[], const char *directory, char *const envp[]);

        int pipe_command(const char *buf, const char *directory);

        int remove_directory(const char *path);

        int directory_exists(const char *path);

        int file_exists(const char *path);

        int download_to_temp_file(const char *url, std::string &filename);

        void checkout_progress(const char *path, size_t cur, size_t tot, void *payload);

        int make_temp_file(char *buffer, size_t size);

        char *make_temp_dir(char *buffer, size_t size);

        int mkpath(const char *dir, mode_t mode);

        int copy_file(const std::string &from, const std::string &to);

        std::string get_user_home_dir();

        const char* build_sys_path(const char *start, ...)  __attribute__((format(printf, 1, 2)));

        int prompt_to_add_path_to_shell_rc(const char *shellrc, const char *path);
    }
}

#endif
