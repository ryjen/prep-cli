
#include <archive_entry.h>
#include <cerrno>
#include <climits>
#include <cstdlib>

#include "common.h"
#include "decompressor.h"
#include "log.h"
#include "util.h"

namespace micrantha
{
    namespace prep
    {
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
                r = static_cast<int>(archive_write_data_block(aw, buff, size, offset));
                if (r != ARCHIVE_OK) {
                    log::error("unable to write archive data block ", r, ":", archive_error_string(aw));
                    return (r);
                }
            }
        }

        Decompressor::Decompressor(const std::string &path) : Decompressor(path, path)
        {
        }

        Decompressor::Decompressor(const void *from, size_t size, const std::string &topath)
            : from_(from), size_(size), type_(MEMORY), outPath_(topath), in_(nullptr), out_(nullptr)
        {
        }

        Decompressor::Decompressor(const std::string &path, const std::string &topath)
            : from_(path.c_str()), size_(10240), type_(FILE), outPath_(topath), in_(nullptr), out_(nullptr)
        {
        }

        Decompressor::~Decompressor()
        {
            cleanup();
        }

        void Decompressor::cleanup()
        {
            if (in_ != nullptr) {
#if ARCHIVE_VERSION_NUMBER < 3000000
                archive_read_close(in_);
#else
                archive_read_free(in_);
#endif
                in_ = nullptr;
            }

            if (out_ != nullptr) {
#if ARCHIVE_VERSION_NUMBER < 3000000
                archive_write_close(out_);
#else
                archive_write_free(out_);
#endif
                out_ = nullptr;
            }
        }
        int Decompressor::decompress()
        {
            int r;
            struct archive_entry *entry;
            char folderName[PATH_MAX + 1] = {0};
            char buf[PATH_MAX + 1] = {0};

            if (in_ != nullptr || out_ != nullptr) {
                log::perror(EINVAL);
                return PREP_FAILURE;
            }

            in_ = archive_read_new();

            archive_read_support_format_all(in_);
#if ARCHIVE_VERSION_NUMBER >= 3000000
            archive_read_support_filter_all(in_);
#else
            archive_read_support_compression_all(in_);
#endif
            switch (type_) {
                case FILE:
                    if ((r = archive_read_open_filename(in_, static_cast<const char *>(from_), size_))) {
                        log::error("unable to open file ", r, ": ", archive_error_string(in_));
                        cleanup();
                        return PREP_FAILURE;
                    }
                    break;
                case MEMORY:
                    if ((r = archive_read_open_memory(in_, from_, size_))) {
                        log::error("unable to open memory archive ", r, ": ", archive_error_string(in_));
                        cleanup();
                        return PREP_FAILURE;
                    }
            }
            out_ = archive_write_disk_new();

            archive_write_disk_set_options(out_, ARCHIVE_EXTRACT_TIME);

            // check the first entry for a directory
            r = archive_read_next_header(in_, &entry);

            if (r < ARCHIVE_OK) {
                log::error("error extracting ", r, ": ", archive_error_string(in_));
                cleanup();
                return PREP_SUCCESS;
            }

            strncpy(folderName, archive_entry_pathname(entry), PATH_MAX);

            strncpy(buf, filesystem::build_path(outPath_, folderName).c_str(), PATH_MAX);

            archive_entry_set_pathname(entry, buf);

            while (r == ARCHIVE_OK) {
                if (archive_write_header(out_, entry) != ARCHIVE_OK) {
                    log::error("unable to write header from decompression ", r, ": ", archive_error_string(in_));
                } else {
                    copy_data(in_, out_);
                    if (archive_write_finish_entry(out_) != ARCHIVE_OK) {
                        log::error("unable finish archive write ", r, ": ", archive_error_string(out_));
                        cleanup();
                        return PREP_FAILURE;
                    }
                }

                r = archive_read_next_header(in_, &entry);

                if (r == ARCHIVE_OK) {
                    const char *entryName = archive_entry_pathname(entry);

                    strncpy(buf, filesystem::build_path(outPath_, entryName).c_str(), PATH_MAX);

                    archive_entry_set_pathname(entry, buf);

                    log::debug("extracting ", buf);
                }
            }

            if (r < ARCHIVE_OK) {
                log::error("unable to extracting ", r, ": ", archive_error_string(in_));
                cleanup();
                return PREP_FAILURE;
            }

            cleanup();
            return PREP_SUCCESS;
        }
    }
}
