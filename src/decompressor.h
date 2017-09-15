/**
 * @author: Ryan Jennings <ryan@micrantha.com>
 */
#ifndef MICRANTHA_PREP_DECOMPRESSOR_H
#define MICRANTHA_PREP_DECOMPRESSOR_H

#ifdef HAVE_ARCHIVE_H
#include <archive.h>
#endif

#include <string>

namespace micrantha
{
    namespace prep
    {
        class decompressor
        {
          public:
            decompressor(const void *buf, size_t size, const std::string &topath);
            decompressor(const std::string &path);
            decompressor(const std::string &path, const std::string &topath);
            ~decompressor();

            int decompress(bool ignoreErrors = false);

          private:
            typedef enum { FILE, MEMORY } Type;
            void cleanup();
            decompressor &operator=(const decompressor &other) = delete;
            decompressor(const decompressor &other) = delete;

            struct archive *in_;
            struct archive *out_;

            const void *from_;
            size_t fromSize_;
            Type type_;
            std::string outPath_;
        };
    }
}

#endif
