#pragma once

#include <string>
#include <string_view>
#include <bzlib.h>

// ----------------------------------------------------------------------

namespace ae::file
{
    namespace bz2_internal
    {
        constexpr const unsigned char sBz2Sig[] = { 'B', 'Z', 'h' };

    } // namespace bz2_internal

      // ----------------------------------------------------------------------

    inline bool bz2_compressed(std::string_view input)
    {
        if (input.size() < sizeof(bz2_internal::sBz2Sig))
            return false;
        return std::memcmp(input.data(), bz2_internal::sBz2Sig, sizeof(bz2_internal::sBz2Sig)) == 0;
    }

      // ----------------------------------------------------------------------

    // inline std::string bz2_compress(std::string_view /*input*/)
    // {
    //     throw std::runtime_error("ae::file::bz2_compress not implemented");
    //     // lzma_stream strm = LZMA_STREAM_INIT; /* alloc and init lzma_stream struct */
    //     // if (lzma_easy_encoder(&strm, 9 | LZMA_PRESET_EXTREME, LZMA_CHECK_CRC64) != LZMA_OK) {
    //     //     throw std::runtime_error("lzma compression failed 1");
    //     // }
    //     // return xz_internal::process(&strm, input);
    // }

      // ----------------------------------------------------------------------

    inline std::string bz2_decompress(std::string_view input, size_t padding = 0)
    {
        constexpr ssize_t BufSize = 409600;
        bz_stream strm;
        strm.bzalloc = nullptr;
        strm.bzfree = nullptr;
        strm.opaque = nullptr;
        if (BZ2_bzDecompressInit(&strm, 0 /*verbosity*/, 0 /* not small */) != BZ_OK)
            throw std::runtime_error("bz2 decompression failed during initialization");
        try {
            strm.next_in = const_cast<decltype(strm.next_in)>(input.data());
            strm.avail_in = static_cast<decltype(strm.avail_in)>(input.size());
            std::string output;
            output.reserve(BufSize + padding);
            output.resize(BufSize);
            ssize_t offset = 0;
            for (;;) {
                strm.next_out = const_cast<decltype(strm.next_out)>(&*(output.begin() + offset));
                strm.avail_out = BufSize;
                auto const r = BZ2_bzDecompress(&strm);
                if (r == BZ_OK) {
                    if (strm.avail_out > 0)
                        throw std::runtime_error("bz2 decompression failed: unexpected end of input");
                    offset += BufSize;
                    output.reserve(static_cast<size_t>(offset + BufSize) + padding);
                    output.resize(static_cast<size_t>(offset + BufSize));
                }
                else if (r == BZ_STREAM_END) {
                    output.resize(static_cast<size_t>(offset + BufSize) - strm.avail_out);
                    output.reserve(static_cast<size_t>(offset + BufSize) - strm.avail_out + padding);
                    break;
                }
                else
                    throw std::runtime_error("bz2 decompression failed, code: " + std::to_string(r));
            }
            BZ2_bzDecompressEnd(&strm);
            return output;
        }
        catch (std::exception&) {
            BZ2_bzDecompressEnd(&strm);
            throw;
        }
    }

} // namespace ae::file

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
