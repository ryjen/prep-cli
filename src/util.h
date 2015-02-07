
namespace arg3
{
    namespace prep
    {

        int pipe_command(const char *buf, const char *directory);

        int remove_directory(const char *path);

        int directory_exists(const char *path);

        void checkout_progress(const char *path, size_t cur, size_t tot, void *payload);

        int make_temp_file(char *buffer, size_t size);

        char *make_temp_dir(char *buffer, size_t size);

    }
}