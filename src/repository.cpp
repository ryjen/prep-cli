#include "repository.h"
#include <sys/stat.h>
#include <git2/types.h>
#include <git2/clone.h>
#include <git2/errors.h>
#include <unistd.h>

namespace arg3
{
    namespace cpppm
    {
        typedef struct
        {

        } progress_data;

        int fetch_progress(
            const git_transfer_progress *stats,
            void *payload)
        {
            progress_data *pd = (progress_data *)payload;
            /* Do something with network transfer progress */
            printf("%d/%d %.0f%%", stats->indexed_objects, stats->total_objects, stats->indexed_objects / stats->total_objects * 100.f);

            return 0;
        }

        void checkout_progress(
            const char *path,
            size_t cur,
            size_t tot,
            void *payload)
        {
            progress_data *pd = (progress_data *)payload;
            /* Do something with checkout progress */
        }


        string repository::get_path() const
        {
            if (global_)
                return "/usr/local/share/cppm";
            else
                return ".cppm";
        }

        void repository::set_global(bool value)
        {
            global_ = value;
        }

        void repository::initialize()
        {
            mkdir(get_path().c_str(), 0700);
        }

        void repository::get(const std::string &url)
        {
            char buffer [BUFSIZ + 1] = {0};
            strncpy(buffer, "/tmp/cpppm-XXXXXX", BUFSIZ);

            mkdtemp (buffer);

            progress_data d = {};
            git_clone_options opts = GIT_CLONE_OPTIONS_INIT;
            git_checkout_opts checkout_opts = GIT_CHECKOUT_OPTS_INIT;

            checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE_CREATE;
            checkout_opts.progress_cb = checkout_progress;
            checkout_opts.progress_payload = &d;
            // clone_opts.checkout_opts = checkout_opts;
            // clone_opts.remote_callbacks.transfer_progress = &fetch_progress;
            // clone_opts.remote_callbacks.payload = &d;

            git_repository *repo = NULL;
            int error = git_clone(&repo, url.c_str(), buffer, &opts);
            if (error < 0)
            {
                const git_error *e = giterr_last();
                printf("Error %d/%d: %s\n", error, e->klass, e->message);
                exit(error);
            }
            git_repository_free(repo);

            string config_file(buffer);

            config_file += "/cpppm.conf";

            package_config config;

            if (config.load(config_file))
            {
                rename(buffer, (get_path() + "/" + config.name()).c_str());
            }
            else
            {
                printf("Error %s is not a valid cpppm package\n", url.c_str());
            }
        }
    }
}