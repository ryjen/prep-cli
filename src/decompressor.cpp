
#include "decompressor.h"

#ifdef HAVE_LIBARCHIVE
#include <archive_entry.h>
#endif

#include <cstdlib>

namespace arg3
{
    namespace prep
    {
#ifdef HAVE_LIBARCHIVE
        static int copy_data(struct archive *ar, struct archive *aw)
        {
            int r;
            const void *buff;
            size_t size;
#if ARCHIVE_VERSION_NUMBER >= 3000000
            int64_t offset;
#else
            off_t offset;
#endif

            for (;;)
            {
                r = archive_read_data_block(ar, &buff, &size, &offset);
                if (r == ARCHIVE_EOF)
                    return (ARCHIVE_OK);
                if (r != ARCHIVE_OK)
                    return (r);
                r = archive_write_data_block(aw, buff, size, offset);
                if (r != ARCHIVE_OK)
                {
                    printf("unable to write archive data block %d:%s", r, archive_error_string(aw));
                    return (r);
                }
            }
        }
#endif

        decompressor::decompressor(const char *path) : path_(path)
#ifdef HAVE_LIBARCHIVE
            , in_(NULL), out_(NULL)
#endif
        {
        }

        decompressor::~decompressor()
        {
            cleanup();
        }

        std::string decompressor::outputPath() const
        {
            return outPath_;
        }
        std::string decompressor::inputPath() const
        {
            return path_;
        }

        void decompressor::cleanup()
        {
#ifdef HAVE_LIBARCHIVE
            if (in_ != NULL)
            {
                archive_read_close(in_);
                archive_read_finish(in_);
                in_ = NULL;
            }

            if (out_ != NULL)
            {

                archive_write_close(out_);
                archive_write_finish(out_);
                out_ = NULL;
            }
#endif
        }
        int decompressor::decompress()
        {

#ifdef HAVE_LIBARCHIVE
            int r;
            struct archive_entry *entry;

            if (in_ != NULL || out_ != NULL)
            {
                return EXIT_FAILURE;
            }

            in_ = archive_read_new();

            archive_read_support_format_all(in_);
            archive_read_support_compression_all(in_);

            if ((r = archive_read_open_filename(in_, path_.c_str(), 10240)))
            {
                printf("unable to open %s %d: %s\n", path_.c_str(), r, archive_error_string(in_));
                cleanup();
                return EXIT_FAILURE;
            }

            out_ = archive_write_disk_new();

            archive_write_disk_set_options(out_, ARCHIVE_EXTRACT_TIME);

            // check the first entry for a directory
            r = archive_read_next_header(in_, &entry);

            if (r < ARCHIVE_OK)
            {
                printf("error extracting %s - %d:%s\n", path_.c_str(), r, archive_error_string(in_));
                cleanup();
                return EXIT_SUCCESS;
            }

            const char *temp = archive_entry_pathname( entry );

            if (temp[strlen(temp) - 1] == '/')
                outPath_ .assign(temp, strlen(temp) - 1);
            else
                outPath_ = ".";

            while (r == ARCHIVE_OK)
            {
                if (archive_write_header(out_, entry) != ARCHIVE_OK)
                    printf("unable to write header from decompression %d:%s", r, archive_error_string(in_));
                else
                {
                    copy_data(in_, out_);
                    if (archive_write_finish_entry(out_) != ARCHIVE_OK)
                    {
                        printf("unable finish archive write %d:%s\n", r, archive_error_string(out_));
                        cleanup();
                        return EXIT_FAILURE;
                    }
                }

                r = archive_read_next_header(in_, &entry);
            }

            if (r < ARCHIVE_OK)
            {
                printf("unable to extract %s - %d: %s\n", path_.c_str(), r, archive_error_string(in_));
                cleanup();
                return EXIT_FAILURE;
            }

            cleanup();
            return EXIT_SUCCESS;
#else
            return EXIT_FAILURE;
#endif
        }
    }
}