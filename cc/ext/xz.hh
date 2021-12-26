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
        constexpr size_t BufSize = 409600;
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

        std::string compress(std::string_view input) override
        {
            if (lzma_easy_encoder(&strm_, 9 | LZMA_PRESET_EXTREME, LZMA_CHECK_CRC64) != LZMA_OK) {
                throw compressor_failed("lzma compression failed 1");
            }
            return process(input, 0, xz_internal::BufSize);
        }

        std::string decompress(std::string_view input) override
        {
            if (lzma_stream_decoder(&strm_, UINT64_MAX, LZMA_TELL_UNSUPPORTED_CHECK | LZMA_CONCATENATED) != LZMA_OK) {
                throw compressor_failed("lzma decompression failed 1");
            }
            const size_t buf_size = input.size() < xz_internal::BufSize ? xz_internal::BufSize : input.size() * 100;
            return process(input, padding(), buf_size);
        }

      private:
        lzma_stream strm_{};

        std::string process(std::string_view input, size_t padding, size_t buf_size)
        {
            strm_.next_in = reinterpret_cast<const uint8_t*>(input.data());
            strm_.avail_in = input.size();
            std::string output;
            output.reserve(buf_size + padding);
            output.resize(buf_size);
            ssize_t offset = 0;
            for (;;) {
                strm_.next_out = reinterpret_cast<uint8_t*>(&*(output.begin() + offset));
                strm_.avail_out = buf_size;
                auto const r = lzma_code(&strm_, LZMA_FINISH);
                if (r == LZMA_STREAM_END) {
                    output.resize(static_cast<size_t>(offset) + buf_size - strm_.avail_out);
                    output.reserve(static_cast<size_t>(offset) + buf_size - strm_.avail_out + padding);
                    break;
                }
                else if (r == LZMA_OK) {
                    offset += buf_size;
                    output.reserve(static_cast<size_t>(offset) + buf_size + padding);
                    output.resize(static_cast<size_t>(offset) + buf_size);
                }
                else {
                    throw compressor_failed("lzma decompression failed 2");
                }
            }
            return output;
        }
    };

} // namespace ae::file

// ----------------------------------------------------------------------
