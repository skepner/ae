#pragma once

#include <string>
#include <string_view>
#include <cstring>

#include <zlib.h>

#include "ext/compressor.hh"

// ----------------------------------------------------------------------

namespace ae::file
{
    namespace gzip_internal
    {
        constexpr const unsigned char Signature[] = {0x1F, 0x8B};
        constexpr ssize_t BufSize = 409600;

    } // namespace gzip_internal

    // ----------------------------------------------------------------------

    inline bool gzip_compressed(std::string_view input)
    {
        if (input.size() < sizeof(gzip_internal::Signature))
            return false;
        return std::memcmp(input.data(), gzip_internal::Signature, sizeof(gzip_internal::Signature)) == 0;
    }

    class GZIP_Compressor : public Compressor
    {
      public:
        GZIP_Compressor(size_t padding = 0) : Compressor(padding)
        {
            strm_.zalloc = Z_NULL;
            strm_.zfree = Z_NULL;
            strm_.opaque = Z_NULL;
        }

        ~GZIP_Compressor() override {}

        std::string compress(std::string_view input) override
        {
            ssize_t offset = 0;
            std::string output;

            auto advance = [this, &output, &offset]() {
                offset += gzip_internal::BufSize;
                output.resize(static_cast<size_t>(offset + gzip_internal::BufSize));
                strm_.next_out = reinterpret_cast<decltype(strm_.next_out)>(output.data() + offset);
                strm_.avail_out = gzip_internal::BufSize;
            };

            strm_.next_in = reinterpret_cast<decltype(strm_.next_in)>(const_cast<char*>(input.data()));
            strm_.total_in = strm_.avail_in = static_cast<decltype(strm_.avail_in)>(input.size());
            if (deflateInit2(&strm_, Z_BEST_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
                throw compressor_failed("gzip compression failed during initialization");

            try {
                output.resize(gzip_internal::BufSize, ' ');
                strm_.next_out = reinterpret_cast<decltype(strm_.next_out)>(output.data() + offset);
                strm_.avail_out = gzip_internal::BufSize;
                while (strm_.avail_in != 0) {
                    if (int res = deflate(&strm_, Z_NO_FLUSH); res != Z_OK)
                        throw compressor_failed("gzip compression failed, code: " + std::to_string(res));
                    if (strm_.avail_out == 0)
                        advance();
                }

                int deflate_res = Z_OK;
                while (deflate_res == Z_OK) {
                    if (strm_.avail_out == 0)
                        advance();
                    deflate_res = deflate(&strm_, Z_FINISH);
                }
                if (deflate_res != Z_STREAM_END)
                    throw compressor_failed("gzip compression failed, code: " + std::to_string(deflate_res));
                output.resize(static_cast<size_t>(offset + gzip_internal::BufSize) - strm_.avail_out);

                deflateEnd(&strm_);
                return output;
            }
            catch (std::exception&) {
                deflateEnd(&strm_);
                throw;
            }
        }

        std::string decompress(std::string_view input) override
        {
            strm_.next_in = reinterpret_cast<decltype(strm_.next_in)>(const_cast<char*>(input.data()));
            strm_.total_in = strm_.avail_in = static_cast<decltype(strm_.avail_in)>(input.size());

            if (inflateInit2(&strm_, 15 + 32) != Z_OK) // 15 window bits, and the +32 tells zlib to to detect if using gzip or zlib
                throw compressor_failed("gzip decompression failed during initialization");

            try {
                std::string output;
                output.reserve(gzip_internal::BufSize + padding());
                output.resize(gzip_internal::BufSize);
                ssize_t offset = 0;
                for (;;) {
                    strm_.next_out = reinterpret_cast<decltype(strm_.next_out)>(output.data() + offset);
                    strm_.avail_out = gzip_internal::BufSize;
                    auto const r = inflate(&strm_, Z_NO_FLUSH);
                    if (r == Z_OK) {
                        if (strm_.avail_out > 0)
                            throw compressor_failed("gzip decompression failed: unexpected end of input");
                        offset += gzip_internal::BufSize;
                        output.reserve(static_cast<size_t>(offset + gzip_internal::BufSize) + padding());
                        output.resize(static_cast<size_t>(offset + gzip_internal::BufSize));
                    }
                    else if (r == Z_STREAM_END) {
                        output.resize(static_cast<size_t>(offset + gzip_internal::BufSize) - strm_.avail_out);
                        output.reserve(static_cast<size_t>(offset + gzip_internal::BufSize) - strm_.avail_out + padding());
                        break;
                    }
                    else {
                        throw compressor_failed("gzip decompression failed, code: " + std::to_string(r));
                    }
                }
                inflateEnd(&strm_);
                return output;
            }
            catch (std::exception&) {
                inflateEnd(&strm_);
                throw;
            }
        }

      private:
        z_stream strm_{};
    };

} // namespace ae::file

// ----------------------------------------------------------------------
