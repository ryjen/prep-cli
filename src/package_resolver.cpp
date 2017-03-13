#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#ifdef HAVE_LIBGIT2
#include <git2.h>
#endif
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif
#include <cassert>
#include "common.h"
#include "decompressor.h"
#include "log.h"
#include "package_resolver.h"
#include "repository.h"
#include "util.h"

namespace rj
{
    namespace prep
    {
        namespace helper
        {
#ifdef HAVE_LIBGIT2
            int fetch_progress(const git_transfer_progress *stats, void *payload)
            {
                printf("\x1b[Afetching %d/%d %.0f%% \n\r", stats->indexed_objects, stats->total_objects,
                       (float)stats->indexed_objects / (float)stats->total_objects * 100.f);
                return 0;
            }

            void checkout_progress(const char *path, size_t cur, size_t tot, void *payload)
            {
                printf("\x1b[Acheckout %ld/%ld %.0f%% \n\r", cur, tot, (float)cur / (float)tot * 100.f);
            }

            void build_checkout_opts(git_checkout_options *opts)
            {
                if (opts == NULL) {
                    return;
                }
#if (LIBGIT2_SOVERSION >= 23)
                opts->checkout_strategy = GIT_CHECKOUT_SAFE;
#else
                opts->checkout_strategy = GIT_CHECKOUT_SAFE_CREATE;
#endif
                opts->progress_cb = checkout_progress;
                opts->progress_payload = NULL;
            }

            void build_fetch_opts(git_fetch_options *opts)
            {
                if (opts == NULL) {
                    return;
                }

#if (LIBGIT2_SOVERSION >= 23)
                opts->callbacks.transfer_progress = &fetch_progress;
                opts->callbacks.payload = NULL;
#endif
            }

            int init_package_submodule(git_submodule *submodule, const char *name, void *payload)
            {
                git_submodule_update_options opts = GIT_SUBMODULE_UPDATE_OPTIONS_INIT;

                opts.checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
                opts.fetch_opts = GIT_FETCH_OPTIONS_INIT;

                build_checkout_opts(&opts.checkout_opts);

                build_fetch_opts(&opts.fetch_opts);
#if (LIBGIT2_SOVERSION >= 23)
                opts.clone_checkout_strategy = GIT_CHECKOUT_SAFE;
#else
                opts.clone_checkout_strategy = GIT_CHECKOUT_SAFE_CREATE;
#endif

                int error = git_submodule_update(submodule, 1, &opts);

                return error;
            }
#endif
        }
        package_resolver::package_resolver() : isTemp_(false)
        {
        }

        bool package_resolver::is_temp_path() const
        {
            return isTemp_;
        }

        std::string package_resolver::package_dir() const
        {
            return directory_;
        }

        int package_resolver::resolve_package_git(package &config, const options &opts, const char *url)
        {
#ifdef HAVE_LIBGIT2
            char buffer[BUFSIZ + 1] = {0};
            strncpy(buffer, "/tmp/prep-XXXXXX", BUFSIZ);

            mkdtemp(buffer);

            git_libgit2_init();
            git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;

            helper::build_checkout_opts(&checkout_opts);

            git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
            clone_opts.checkout_opts = checkout_opts;

#if (LIBGIT2_SOVERSION >= 23)
            helper::build_fetch_opts(&clone_opts.fetch_opts);
#else
            clone_opts.remote_callbacks.transfer_progress = &fetch_progress;
            clone_opts.remote_callbacks.payload = NULL;
#endif

            git_repository *repo = NULL;

            printf("\n");

            int error = git_clone(&repo, url, buffer, &clone_opts);
            if (error < 0) {
                const git_error *e = giterr_last();
                log_error("%d/%d: %s\n", error, e->klass, e->message);
                return PREP_FAILURE;
            }

            isTemp_ = true;

            printf("\n");

            error = git_submodule_foreach(repo, helper::init_package_submodule, this);

            git_repository_free(repo);

            if (error < 0) {
                const git_error *e = giterr_last();
                log_error("%d/%d: %s\n", error, e->klass, e->message);
                return PREP_FAILURE;
            }

            return resolve_package_directory(config, opts, buffer);
#else
            log_error("libgit2 is not installed or configured.");
            return PREP_FAILURE;
#endif
        }

        int package_resolver::resolve_package_download(package &config, const options &opts, const char *url)
        {
            string buffer;

            if (download_to_temp_file(url, buffer)) {
                log_error("unable to download %s", opts.location.c_str());
                return PREP_FAILURE;
            }

            int rval = resolve_package_archive(config, opts, buffer.c_str());

            unlink(buffer.c_str());

            return rval;
        }

        int package_resolver::resolve_package_archive(package &config, const options &opts, const char *path)
        {
            log_trace("resolving archive [%s]...", path);

            decompressor d(path);

            if (d.decompress()) {
                log_error("unable to decompress %s", path);
                return PREP_FAILURE;
            }

            isTemp_ = true;

            // decompress
            return resolve_package_directory(config, opts, d.outputPath().c_str());
        }

        int package_resolver::resolve_package_directory(package &config, const options &opts, const char *path)
        {
            log_trace("resolving package directory [%s]...", path);

            directory_ = path;

            return config.load(path, opts);
        }

        int package_resolver::resolve_package(package &config, const options &opts)
        {
            return resolve_package(config, opts, opts.location);
        }

        int package_resolver::resolve_package(package &config, const options &opts, const std::string &path)
        {
            int fileType = directory_exists(path.c_str());

            int status = PREP_FAILURE;

            if (fileType == 1) {
                status = resolve_package_directory(config, opts, path.c_str());
            } else if (fileType == 2) {
                status = resolve_package_archive(config, opts, path.c_str());
            } else if (path.find("://") != std::string::npos) {
                if (path.rfind(".git") != std::string::npos || path.find("git://") != std::string::npos) {
                    status = resolve_package_git(config, opts, path.c_str());
                } else {
                    status = resolve_package_download(config, opts, path.c_str());
                }
            }

            return status;
        }
    }
}
