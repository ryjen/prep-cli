/**
 * @author: Ryan Jennings <ryan@micrantha.com>
 */
#ifndef MICRANTHA_PREP_DECOMPRESSOR_H
#define MICRANTHA_PREP_DECOMPRESSOR_H

#include <archive.h>
#include <string>

namespace micrantha
{
    namespace prep
    {
        /**
         * used to decompress a file or memory
         */
        class Decompressor
        {
           public:
            /* constructors */
            Decompressor(const void *buf, size_t size, const std::string &topath);
            explicit Decompressor(const std::string &path);
            Decompressor(const std::string &path, const std::string &topath);
            ~Decompressor();

            // disable copying
            Decompressor &operator=(const Decompressor &other) = delete;
            Decompressor(const Decompressor &other) = delete;

            /**
             * performs the decompression
             * @return PREP_SUCCESS if decompressed, otherwise PREP_FAILURE
             */
            int decompress();

           private:
            // types of decompression
            typedef enum { FILE, MEMORY } Type;

            // utility method
            void cleanup();

            // internal library structures
            struct archive *in_;
            struct archive *out_;

            const void *from_;
            std::string outPath_;
            // memory or file block size
            size_t size_;
            // type of input
            Type type_;
        };
    }
}

#endif
