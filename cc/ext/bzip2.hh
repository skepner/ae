#pragma once

#include <string>
#include <string_view>
#include <bzlib.h>

#include "ext/compressor.hh"

// ----------------------------------------------------------------------

namespace ae::file
{
    namespace bz2_internal
    {
        constexpr const unsigned char Signature[] = { 'B', 'Z', 'h' };
        constexpr ssize_t BufSize = 409600;

    } // namespace bz2_internal

      // ----------------------------------------------------------------------

    inline bool bz2_compressed(std::string_view input)
    {
        if (input.size() < sizeof(bz2_internal::Signature))
            return false;
        return std::memcmp(input.data(), bz2_internal::Signature, sizeof(bz2_internal::Signature)) == 0;
    }

      // ----------------------------------------------------------------------

    class BZ2_Compressor : public Compressor
    {
      public:
        BZ2_Compressor(size_t padding = 0) : Compressor(padding)
        {
            strm_.bzalloc = nullptr;
            strm_.bzfree = nullptr;
            strm_.opaque = nullptr;
        }

        std::string compress(std::string_view /*input*/) override { throw compressor_failed{"ae::file::bz2_compress not implemented"}; }

        std::string decompress(std::string_view input) override
        {
            if (BZ2_bzDecompressInit(&strm_, 0 /*verbosity*/, 0 /* not small */) != BZ_OK)
                throw compressor_failed("bz2 decompression failed during initialization");
            try {
                strm_.next_in = const_cast<decltype(strm_.next_in)>(input.data());
                strm_.avail_in = static_cast<decltype(strm_.avail_in)>(input.size());
                std::string output;
                output.reserve(bz2_internal::BufSize + padding());
                output.resize(bz2_internal::BufSize);
                ssize_t offset = 0;
                for (;;) {
                    strm_.next_out = const_cast<decltype(strm_.next_out)>(&*(output.begin() + offset));
                    strm_.avail_out = bz2_internal::BufSize;
                    auto const r = BZ2_bzDecompress(&strm_);
                    if (r == BZ_OK) {
                        if (strm_.avail_out > 0)
                            throw compressor_failed("bz2 decompression failed: unexpected end of input");
                        offset += bz2_internal::BufSize;
                        output.reserve(static_cast<size_t>(offset + bz2_internal::BufSize) + padding());
                        output.resize(static_cast<size_t>(offset + bz2_internal::BufSize));
                    }
                    else if (r == BZ_STREAM_END) {
                        output.resize(static_cast<size_t>(offset + bz2_internal::BufSize) - strm_.avail_out);
                        output.reserve(static_cast<size_t>(offset + bz2_internal::BufSize) - strm_.avail_out + padding());
                        break;
                    }
                    else
                        throw compressor_failed("bz2 decompression failed, code: " + std::to_string(r));
                }
                BZ2_bzDecompressEnd(&strm_);
                return output;
            }
            catch (std::exception&) {
                BZ2_bzDecompressEnd(&strm_);
                throw;
            }
        }

      private:
        bz_stream strm_{};
    };

} // namespace ae::file

// ----------------------------------------------------------------------
