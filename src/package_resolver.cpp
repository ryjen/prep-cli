#include "config.h"
#include "package_resolver.h"
#include "util.h"
#include "log.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_LIBGIT2
#include <git2.h>
#endif
#ifdef HAVE_LIBCURL
#include <curl/curl.h>
#include <curl/easy.h>
#endif
#ifdef HAVE_LIBZ
#include <zlib.h>
#endif
#include <cassert>
#include <decompressor.h>

namespace arg3
{
    namespace prep
    {
        package_resolver::package_resolver(): isTemp_(false)
        {}

        bool package_resolver::is_temp_path() const
        {
            return isTemp_;
        }

        std::string package_resolver::working_dir() const
        {
            return workingDir_;
        }

#ifdef HAVE_LIBGIT2
        int fetch_progress(const git_transfer_progress *stats, void *payload)
        {
            printf("\033[Afetching %d/%d %.0f%%\n\r", stats->indexed_objects, stats->total_objects,
                   (float) stats->indexed_objects / (float) stats->total_objects * 100.f);

            return 0;
        }

        void checkout_progress(const char *path, size_t cur, size_t tot, void *payload)
        {
            printf("\033[Acheckout %ld/%ld %.0f%%\n\r", cur, tot, (float) cur / (float) tot * 100.f);
        }
#endif

        int package_resolver::resolve_package_git(package &config, const options &opts)
        {
#ifdef HAVE_LIBGIT2
            char buffer [BUFSIZ + 1] = {0};
            strncpy(buffer, "/tmp/prep-XXXXXX", BUFSIZ);

            mkdtemp (buffer);

            git_libgit2_init();
            git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
            checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE_CREATE;
            checkout_opts.progress_cb = checkout_progress;
            checkout_opts.progress_payload = NULL;

            git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
            clone_opts.checkout_opts = checkout_opts;
            clone_opts.remote_callbacks.transfer_progress = &fetch_progress;
            clone_opts.remote_callbacks.payload = NULL;

            git_repository *repo = NULL;
            int error = git_clone(&repo, opts.location.c_str(), buffer, &clone_opts);
            if (error < 0)
            {
                const git_error *e = giterr_last();
                log_error("%d/%d: %s\n", error, e->klass, e->message);
                return EXIT_FAILURE;
            }
            git_repository_free(repo);

            isTemp_ = true;

            return resolve_package_directory(config, opts, buffer);
#else
            log_error("libgit2 is not installed or configured.");
            return EXIT_FAILURE;
#endif
        }

#ifdef HAVE_LIBCURL
        size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
        {
            size_t written = fwrite(ptr, size, nmemb, stream);
            return written;
        }
#endif

        int package_resolver::resolve_package_download(package &config, const options &opts)
        {
#ifdef HAVE_LIBCURL
            char buffer [BUFSIZ + 1] = {0};
            FILE *fp;
            int fd = -1;
            CURL *curl = curl_easy_init();

            if (curl == NULL) {
                return EXIT_FAILURE;
            }

            strncpy(buffer, "/tmp/prep-XXXXXX", BUFSIZ);

            if ((fd = mkstemp (buffer)) < 0 || (fp = fdopen(fd, "wb")) == NULL) {
                return EXIT_FAILURE;
            }

            curl_easy_setopt(curl, CURLOPT_URL, opts.location.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            fclose(fp);

            int rval = resolve_package_archive(config, opts, buffer);

            unlink(buffer);

            return rval;
#else
            log_error("libcurl not installed or configured");
            return EXIT_FAILURE;
#endif
        }

        int package_resolver::resolve_package_archive(package &config, const options &opts, const char *path)
        {
            log_debug("resolving archive %s...\n", path);

            decompressor d(path);

            if (d.decompress()) {
                return EXIT_FAILURE;
            }

            isTemp_ = true;

            // decompress
            return resolve_package_directory(config, opts, d.outputPath().c_str());
        }

        int package_resolver::resolve_package_directory(package &config, const options &opts, const char *path)
        {
            log_debug("resolving directory %s...", path);

            workingDir_ = path;

            return config.load(path, opts);
        }

        int package_resolver::resolve_package(package &config, const options &opts, const std::string &path)
        {
            log_debug("resolving package %s...", path.c_str());

            int fileType = directory_exists(path.c_str());

            if (fileType == 1)
            {
                return resolve_package_directory(config, opts, path.c_str());
            }
            else if (fileType == 2)
            {
                return resolve_package_archive(config, opts, path.c_str());
            }
            else if (path.find("://") != std::string::npos)
            {
                if (path.rfind(".git") != std::string::npos || path.find("git://") != std::string::npos)
                {
                    return resolve_package_git(config, opts);
                }
                else
                {
                    return resolve_package_download(config, opts);
                }
            }
            else
            {
                return EXIT_FAILURE;
            }
        }
    }
}