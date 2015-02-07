#ifndef INCLUDE_DECOMPRESSOR
#define INCLUDE_DECOMPRESSOR

#include "config.h"

#ifdef HAVE_LIBARCHIVE
#include <archive.h>
#endif

#include <string>
#define windowBits 15
#define ENABLE_ZLIB_GZIP 32

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
