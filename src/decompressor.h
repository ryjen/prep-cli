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
            decompressor(const std::string &path);
            decompressor(const std::string &path, const std::string &topath);
            ~decompressor();

            std::string outputPath() const;
            std::string inputPath() const;

            int decompress(bool ignoreErrors = false);

          private:
            void cleanup();
            decompressor &operator=(const decompressor &other);
            decompressor(const decompressor &other);

            struct archive *in_;
            struct archive *out_;

            std::string path_;
            std::string outPath_;
        };
    }
}

#endif
