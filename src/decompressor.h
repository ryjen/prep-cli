/**
 * @author: Ryan Jennings <c0der78@gmail.com>
 */
#ifndef ARG3_PREP_DECOMPRESSOR_H
#define ARG3_PREP_DECOMPRESSOR_H

#include "config.h"

#ifdef HAVE_ARCHIVE_H
#include <archive.h>
#endif

#include <string>

namespace arg3
{
    namespace prep
    {
        class decompressor
        {
           public:
            decompressor(const char *path);
            ~decompressor();

            std::string outputPath() const;
            std::string inputPath() const;

            int decompress();

           private:
            void cleanup();
            decompressor &operator=(const decompressor &other);
            decompressor(const decompressor &other);

#ifdef HAVE_LIBARCHIVE
            struct archive *in_;
            struct archive *out_;
#endif
            std::string path_;
            std::string outPath_;
        };
    }
}

#endif
