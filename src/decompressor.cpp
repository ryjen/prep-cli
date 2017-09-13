#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef HAVE_ARCHIVE_ENTRY_H
#include <archive_entry.h>
#endif
#include <cerrno>
#include <cstdlib>
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif
#include <limits.h>

#include "common.h"
#include "decompressor.h"
#include "log.h"
#include "util.h"

namespace micrantha
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

            for (;;) {
                r = archive_read_data_block(ar, &buff, &size, &offset);
                if (r == ARCHIVE_EOF) {
                    return (ARCHIVE_OK);
                }
                if (r != ARCHIVE_OK) {
                    return (r);
                }
                r = archive_write_data_block(aw, buff, size, offset);
                if (r != ARCHIVE_OK) {
                    printf("unable to write archive data block %d:%s", r, archive_error_string(aw));
                    return (r);
                }
            }
        }
#endif

        decompressor::decompressor(const std::string &path) : decompressor(path, path)
        {
        }

        decompressor::decompressor(const std::string &path, const std::string &topath)
            : path_(path)
#ifdef HAVE_LIBARCHIVE
              ,
              in_(NULL),
              out_(NULL)
#endif
        {
            char buf[PATH_MAX + 1] = { 0 };

            strncpy(buf, topath.c_str(), PATH_MAX);

            outPath_ = dirname(buf);
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
            if (in_ != NULL) {
#if ARCHIVE_VERSION_NUMBER < 3000000
                archive_read_close(in_);
#else
                archive_read_free(in_);
#endif
                in_ = NULL;
            }

            if (out_ != NULL) {
#if ARCHIVE_VERSION_NUMBER < 3000000
                archive_write_close(out_);
#else
                archive_write_free(out_);
#endif
                out_ = NULL;
            }
#endif
        }
        int decompressor::decompress(bool ignoreErrors)
        {
#ifdef HAVE_LIBARCHIVE
            int r;
            struct archive_entry *entry;
            char folderName[PATH_MAX + 1] = { 0 };
            char buf[PATH_MAX + 1]        = { 0 };

            if (in_ != NULL || out_ != NULL) {
                log_errno(EINVAL);
                return PREP_FAILURE;
            }

            in_ = archive_read_new();

            archive_read_support_format_all(in_);
#if ARCHIVE_VERSION_NUMBER >= 3000000
            archive_read_support_filter_all(in_);
#else
            archive_read_support_compression_all(in_);
#endif

            if ((r = archive_read_open_filename(in_, path_.c_str(), 10240))) {
                log_error("unable to open %s %d: %s\n", path_.c_str(), r, archive_error_string(in_));
                cleanup();
                return PREP_FAILURE;
            }

            out_ = archive_write_disk_new();

            archive_write_disk_set_options(out_, ARCHIVE_EXTRACT_TIME);

            // check the first entry for a directory
            r = archive_read_next_header(in_, &entry);

            if (!ignoreErrors && r < ARCHIVE_OK) {
                log_error("error extracting %s - %d:%s\n", path_.c_str(), r, archive_error_string(in_));
                cleanup();
                return PREP_SUCCESS;
            }

            strncpy(folderName, archive_entry_pathname(entry), PATH_MAX);

            strncpy(buf, build_sys_path(outPath_.c_str(), folderName, NULL), PATH_MAX);

            archive_entry_set_pathname(entry, buf);

            while (r == ARCHIVE_OK) {
                if (archive_write_header(out_, entry) != ARCHIVE_OK) {
                    log_error("unable to write header from decompression %d:%s", r, archive_error_string(in_));
                } else {
                    copy_data(in_, out_);
                    if (archive_write_finish_entry(out_) != ARCHIVE_OK) {
                        log_error("unable finish archive write %d:%s\n", r, archive_error_string(out_));
                        cleanup();
                        return PREP_FAILURE;
                    }
                }

                r = archive_read_next_header(in_, &entry);

                if (r == ARCHIVE_OK) {
                    const char *entryName = archive_entry_pathname(entry);

                    strncpy(buf, build_sys_path(outPath_.c_str(), entryName, NULL), PATH_MAX);

                    archive_entry_set_pathname(entry, buf);

                    log_debug("extracting %s", buf);
                }
            }

            outPath_ = build_sys_path(outPath_.c_str(), folderName, NULL);

            if (r < ARCHIVE_OK) {
                log_error("unable to extract %s - %d: %s\n", path_.c_str(), r, archive_error_string(in_));
                cleanup();
                return PREP_FAILURE;
            }

            cleanup();
            return PREP_SUCCESS;
#else
            log_error("libarchive not configured or installed");
            return PREP_FAILURE;
#endif
        }
    }
}
