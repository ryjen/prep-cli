
namespace arg3
{
    namespace cpppm
    {

        int pipe_command(const char *buf);

        int remove_directory(const char *path);

        int directory_exists(const char *path);

        void checkout_progress(const char *path, size_t cur, size_t tot, void *payload);

    }
}