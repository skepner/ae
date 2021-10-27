#pragma once

#include <cstring>
#include <string>
#include <string_view>

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wdocumentation"
// #pragma GCC diagnostic ignored "-Wdocumentation-unknown-command"
// #pragma GCC diagnostic ignored "-Wreserved-id-macro"
// #pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif
#define lzma_nothrow
#include <lzma.h>
#pragma GCC diagnostic pop

#ifdef __clang__
// #pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

#include "ext/compressor.hh"

// ----------------------------------------------------------------------

namespace ae::file
{
    namespace xz_internal
    {
        const unsigned char Signature[] = {0xFD, '7', 'z', 'X', 'Z', 0x00};
        constexpr ssize_t BufSize = 409600;
    }

    // ----------------------------------------------------------------------

    inline bool xz_compressed(std::string_view input)
    {
        if (input.size() < sizeof(xz_internal::Signature))
            return false;
        return std::memcmp(input.data(), xz_internal::Signature, sizeof(xz_internal::Signature)) == 0;
    }

    // ----------------------------------------------------------------------

    class XZ_Compressor : public Compressor
    {
      public:
        XZ_Compressor(size_t padding = 0) : Compressor(padding) { strm_ = LZMA_STREAM_INIT; }

        ~XZ_Compressor() override { lzma_end(&strm_); }

        std::string_view compress(std::string_view input) override
        {
            if (lzma_easy_encoder(&strm_, 9 | LZMA_PRESET_EXTREME, LZMA_CHECK_CRC64) != LZMA_OK) {
                throw compressor_failed("lzma compression failed 1");
            }
            return process(input);
        }

        std::string_view decompress(std::string_view input, first_chunk /*fc*/) override
        {
            if (lzma_stream_decoder(&strm_, UINT64_MAX, LZMA_TELL_UNSUPPORTED_CHECK | LZMA_CONCATENATED) != LZMA_OK) {
                throw compressor_failed("lzma decompression failed 1");
            }
            return process(input);
        }

      private:
        lzma_stream strm_;
        std::string data_;

        std::string_view process(std::string_view input)
        {
            strm_.next_in = reinterpret_cast<const uint8_t*>(input.data());
            strm_.avail_in = input.size();
            data_.reserve(xz_internal::BufSize + padding());
            data_.resize(xz_internal::BufSize);
            ssize_t offset = 0;
            for (;;) {
                strm_.next_out = reinterpret_cast<uint8_t*>(&*(data_.begin() + offset));
                strm_.avail_out = xz_internal::BufSize;
                auto const r = lzma_code(&strm_, LZMA_FINISH);
                if (r == LZMA_STREAM_END) {
                    data_.resize(static_cast<size_t>(offset + xz_internal::BufSize) - strm_.avail_out);
                    data_.reserve(static_cast<size_t>(offset + xz_internal::BufSize) - strm_.avail_out + padding());
                    break;
                }
                else if (r == LZMA_OK) {
                    offset += xz_internal::BufSize;
                    data_.reserve(static_cast<size_t>(offset + xz_internal::BufSize) + padding());
                    data_.resize(static_cast<size_t>(offset + xz_internal::BufSize));
                }
                else {
                    throw compressor_failed("lzma decompression failed 2");
                }
            }
            return data_;
        }
    };

} // namespace ae::file

// ----------------------------------------------------------------------
