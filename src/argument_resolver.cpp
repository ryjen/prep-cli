#include "argument_resolver.h"
#include "config.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <git2/clone.h>
#include <git2/errors.h>
#include <git2/types.h>
#include "util.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <zlib.h>
#include <cassert>

namespace arg3
{
    namespace cpppm
    {

        argument_resolver::argument_resolver()
        {}

        argument_resolver::argument_resolver(const std::string &arg) : arg_(arg)
        {

        }

        std::string argument_resolver::arg() const
        {
            return arg_;
        }

        void argument_resolver::set_arg(const std::string &arg)
        {
            arg_ = arg;
        }

        int fetch_progress(const git_transfer_progress *stats, void *payload)
        {
            printf("%d/%d %.0f%%\n", stats->indexed_objects, stats->total_objects,
                   (float) stats->indexed_objects / (float) stats->total_objects * 100.f);

            return 0;
        }

        void checkout_progress(const char *path, size_t cur, size_t tot, void *payload)
        {
            printf("%ld/%ld %.0f%%\n", cur, tot, (float) cur / (float) tot * 100.f);
        }

        int argument_resolver::resolve_package_git(package_config &config) const
        {
            char buffer [BUFSIZ + 1] = {0};
            strncpy(buffer, "/tmp/cpppm-XXXXXX", BUFSIZ);

            mkdtemp (buffer);

            git_checkout_opts checkout_opts = GIT_CHECKOUT_OPTS_INIT;
            checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE_CREATE;
            checkout_opts.progress_cb = checkout_progress;
            checkout_opts.progress_payload = NULL;

            git_clone_options opts = GIT_CLONE_OPTIONS_INIT;
            opts.checkout_opts = checkout_opts;
            opts.remote_callbacks.transfer_progress = &fetch_progress;
            opts.remote_callbacks.payload = NULL;

            git_repository *repo = NULL;
            int error = git_clone(&repo, arg_.c_str(), buffer, &opts);
            if (error < 0)
            {
                const git_error *e = giterr_last();
                printf("Error %d/%d: %s\n", error, e->klass, e->message);
                return EXIT_FAILURE;
            }
            git_repository_free(repo);

            int rval = resolve_package_directory(config, buffer);

            remove_directory(buffer);

            return rval;
        }

        size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
        {
            size_t written = fwrite(ptr, size, nmemb, stream);
            return written;
        }

        int argument_resolver::resolve_package_download(package_config &config) const
        {
#ifdef HAVE_LIBCURL
            char buffer [BUFSIZ + 1] = {0};
            FILE *fp;
            int fd = -1;
            CURL *curl = curl_easy_init();

            if (curl == NULL)
                return EXIT_FAILURE;

            strncpy(buffer, "/tmp/cpppm-XXXXXX", BUFSIZ);

            if ((fd = mkstemp (buffer)) < 0 || (fp = fdopen(fd, "wb")) == NULL)
                return EXIT_FAILURE;

            curl_easy_setopt(curl, CURLOPT_URL, arg_.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            fclose(fp);

            int rval = resolve_package_tar_gz(config, buffer);

            unlink(buffer);

            return rval;
#else
            return EXIT_FAILURE;
#endif
        }

#ifdef HAVE_LIBZ
        class decompressor
        {
        public:
            decompressor() : status_(-1)
            {}

            ~decompressor()
            {

                if (status_ == Z_OK)
                {
                    inflateEnd(&strm);
                    status_ = -1;
                }

                if (out_ != NULL)
                {
                    fclose(out_);
                    out_ = NULL;
                }
                if (in_ != NULL)
                {
                    fclose(in_);
                    in_ = NULL;
                }

            }

#define windowBits 15
#define ENABLE_ZLIB_GZIP 32

            int init(const char *path)
            {
                char buffer [BUFSIZ + 1] = {0};
                int fd = -1;

                strncpy(buffer, "/tmp/cpppm-XXXXXX", BUFSIZ);

                if ((fd = mkstemp (buffer)) == -1 || (out_ = fdopen(fd, "wb")) == NULL)
                    return EXIT_FAILURE;

                if ((in_ = fopen (path, "rb")) == NULL)
                {
                    return EXIT_FAILURE;
                }

                strm.zalloc = Z_NULL;
                strm.zfree = Z_NULL;
                strm.opaque = Z_NULL;
                strm.avail_in = 0;
                strm.next_in = Z_NULL;

                if ((status_ = inflateInit2 (& strm, windowBits | ENABLE_ZLIB_GZIP)) != Z_OK)
                {
                    fprintf(stderr, "unable to initialize decompression\n");
                    return EXIT_FAILURE;
                }

                return EXIT_SUCCESS;
            }

            int decompress()
            {

                unsigned char in[BUFSIZ] = {0};
                unsigned char out[BUFSIZ] = {0};
                int ret;

                do
                {
                    strm.avail_in = fread(in, 1, BUFSIZ, in_);

                    if (ferror(in_))
                    {
                        return EXIT_FAILURE;
                    }
                    if (strm.avail_in == 0)
                        break;
                    strm.next_in = in;

                    do
                    {
                        strm.avail_out = BUFSIZ;
                        strm.next_out = out;

                        ret = inflate(&strm, Z_NO_FLUSH);
                        assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
                        switch (ret)
                        {
                        case Z_NEED_DICT:
                            ret = Z_DATA_ERROR;     /* and fall through */
                        case Z_DATA_ERROR:
                        case Z_MEM_ERROR:
                            return ret;
                        }

                        int have = BUFSIZ - strm.avail_out;
                        if (fwrite(out, 1, have, out_) != have || ferror(out_))
                        {
                            return EXIT_FAILURE;
                        }
                    }
                    while (strm.avail_out == 0);
                    /* done when inflate() says it's done */
                }
                while (ret != Z_STREAM_END);

                return ret == Z_STREAM_END ? EXIT_SUCCESS : EXIT_FAILURE;
            }
        private:
            FILE *in_;
            FILE *out_;
            z_stream strm;
            int status_;
        };
#endif

        int argument_resolver::resolve_package_tar_gz(package_config &config, const char *path) const
        {
#ifdef HAVE_LIBZ
            decompressor d;

            d.init(path);

            // decompress
            return resolve_package_directory(config, path);
#else
            return EXIT_FAILURE;
#endif
        }

        int argument_resolver::resolve_package_directory(package_config &config, const char *path) const
        {
            config.set_path(path);

            return config.load();
        }

        int argument_resolver::resolve_package(package_config &config) const
        {
            int fileType = directory_exists(arg_.c_str());

            if (fileType == 1)
            {
                return resolve_package_directory(config, arg_.c_str());
            }
            else if (fileType == 2)
            {
                if (arg_.rfind(".tar.gz") != std::string::npos || arg_.rfind(".tgz") != std::string::npos)
                {
                    // decompress tarball to temp folder
                    // run on temp folder
                    return resolve_package_tar_gz(config, arg_.c_str());
                }
                else
                {
                    return EXIT_FAILURE;
                }
            }
            else if (arg_.find("://") != std::string::npos)
            {
                if (arg_.rfind(".git") != std::string::npos || arg_.find("git://") != std::string::npos)
                {
                    return resolve_package_git(config);
                }
                else
                {
                    return resolve_package_download(config);
                }
            }
            else
            {
                return EXIT_FAILURE;
            }
        }
    }
}