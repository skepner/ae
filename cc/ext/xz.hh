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

// ----------------------------------------------------------------------

namespace ae::file
{
    namespace xz_internal
    {
        const unsigned char sXzSig[] = { 0xFD, '7', 'z', 'X', 'Z', 0x00 };
    }

      // ----------------------------------------------------------------------

    inline bool xz_compressed(const char* input)
    {
        return std::memcmp(input, xz_internal::sXzSig, sizeof(xz_internal::sXzSig)) == 0;
    }

      // ----------------------------------------------------------------------

    std::string xz_compress(std::string_view input);
    std::string xz_decompress(std::string_view input, size_t padding = 0);  // padding to support simdjson

    // ----------------------------------------------------------------------

    namespace detail
    {
        std::string process(lzma_stream* strm, std::string_view input, size_t padding = 0);
    }

} // namespace ae::file

// ----------------------------------------------------------------------

constexpr ssize_t sXzBufSize = 409600;

// ----------------------------------------------------------------------

inline std::string ae::file::detail::process(lzma_stream* strm, std::string_view input, size_t padding)
{
    strm->next_in = reinterpret_cast<const uint8_t*>(input.data());
    strm->avail_in = input.size();
    std::string output;
    output.reserve(sXzBufSize + padding);
    output.resize(sXzBufSize);
    ssize_t offset = 0;
    for (;;) {
        strm->next_out = reinterpret_cast<uint8_t*>(&*(output.begin() + offset));
        strm->avail_out = sXzBufSize;
        auto const r = lzma_code(strm, LZMA_FINISH);
        if (r == LZMA_STREAM_END) {
            output.resize(static_cast<size_t>(offset + sXzBufSize) - strm->avail_out);
            output.reserve(static_cast<size_t>(offset + sXzBufSize) - strm->avail_out + padding);
            break;
        }
        else if (r == LZMA_OK) {
            offset += sXzBufSize;
            output.reserve(static_cast<size_t>(offset + sXzBufSize) + padding);
            output.resize(static_cast<size_t>(offset + sXzBufSize));
        }
        else {
            throw std::runtime_error("lzma decompression failed 2");
        }
    }
    lzma_end(strm);
    return output;

} // ae::file::detail::process

// ----------------------------------------------------------------------

inline std::string ae::file::xz_compress(std::string_view input)
{
    lzma_stream strm = LZMA_STREAM_INIT; /* alloc and init lzma_stream struct */
    if (lzma_easy_encoder(&strm, 9 | LZMA_PRESET_EXTREME, LZMA_CHECK_CRC64) != LZMA_OK) {
        throw std::runtime_error("lzma compression failed 1");
    }
    return detail::process(&strm, input);

} // ae::file::xz_compress

// ----------------------------------------------------------------------

inline std::string ae::file::xz_decompress(std::string_view input, size_t padding)
{
    lzma_stream strm = LZMA_STREAM_INIT; /* alloc and init lzma_stream struct */
    if (lzma_stream_decoder(&strm, UINT64_MAX, LZMA_TELL_UNSUPPORTED_CHECK | LZMA_CONCATENATED) != LZMA_OK) {
        throw std::runtime_error("lzma decompression failed 1");
    }
    return detail::process(&strm, input, padding);

} // ae::file::xz_decompress

// ======================================================================
