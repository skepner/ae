#pragma once

#include <string>
#include <string_view>
#include <cstring>

#include <zlib.h>

// ----------------------------------------------------------------------

namespace ae::file
{
    namespace gzip_internal
    {
        constexpr const unsigned char sGzipSig[] = { 0x1F, 0x8B };

    } // namespace gzip_internal

      // ----------------------------------------------------------------------

    inline bool gzip_compressed(std::string_view input)
    {
        if (input.size() < sizeof(gzip_internal::sGzipSig))
            return false;
        return std::memcmp(input.data(), gzip_internal::sGzipSig, sizeof(gzip_internal::sGzipSig)) == 0;
    }
    std::string gzip_compress(std::string_view input);
    std::string gzip_decompress(std::string_view input, size_t padding = 0); // padding to support simdjson

} // namespace ae::file

// ----------------------------------------------------------------------

inline std::string ae::file::gzip_compress(std::string_view input)
{
    constexpr ssize_t BufSize = 409600;
    z_stream strm;
    std::string output(BufSize, ' ');
    ssize_t offset = 0;

    auto advance = [&strm, &output, &offset]() {
        offset += BufSize;
        output.resize(static_cast<size_t>(offset + BufSize));
        strm.next_out = reinterpret_cast<decltype(strm.next_out)>(output.data() + offset);
        strm.avail_out = BufSize;
    };

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.next_in = reinterpret_cast<decltype(strm.next_in)>(const_cast<char*>(input.data()));
    strm.total_in = strm.avail_in = static_cast<decltype(strm.avail_in)>(input.size());
    if (deflateInit2(&strm, Z_BEST_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        throw std::runtime_error("gzip compression failed during initialization");

    try {
        strm.next_out = reinterpret_cast<decltype(strm.next_out)>(output.data() + offset);
        strm.avail_out = BufSize;
        while (strm.avail_in != 0) {
            if (int res = deflate(&strm, Z_NO_FLUSH); res != Z_OK)
                throw std::runtime_error("gzip compression failed, code: " + std::to_string(res));
            if (strm.avail_out == 0)
                advance();
        }

        int deflate_res = Z_OK;
        while (deflate_res == Z_OK) {
            if (strm.avail_out == 0)
                advance();
            deflate_res = deflate(&strm, Z_FINISH);
        }
        if (deflate_res != Z_STREAM_END)
            throw std::runtime_error("gzip compression failed, code: " + std::to_string(deflate_res));
        output.resize(static_cast<size_t>(offset + BufSize) - strm.avail_out);

        deflateEnd(&strm);
        return output;
    }
    catch (std::exception&) {
        deflateEnd(&strm);
        throw;
    }

} // ae::file::gzip_compress

// ----------------------------------------------------------------------

inline std::string ae::file::gzip_decompress(std::string_view input, size_t padding)
{
    constexpr ssize_t BufSize = 409600;
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = reinterpret_cast<decltype(strm.next_in)>(const_cast<char*>(input.data()));
    strm.total_in = strm.avail_in = static_cast<decltype(strm.avail_in)>(input.size());

    if (inflateInit2(&strm, 15 + 32) != Z_OK) // 15 window bits, and the +32 tells zlib to to detect if using gzip or zlib
        throw std::runtime_error("gzip decompression failed during initialization");

    try {
        std::string output;
        output.reserve(BufSize + padding);
        output.resize(BufSize);
        ssize_t offset = 0;
        for (;;) {
            strm.next_out = reinterpret_cast<decltype(strm.next_out)>(output.data() + offset);
            strm.avail_out = BufSize;
            auto const r = inflate(&strm, Z_NO_FLUSH);
            if (r == Z_OK) {
                if (strm.avail_out > 0)
                    throw std::runtime_error("gzip decompression failed: unexpected end of input");
                offset += BufSize;
                output.reserve(static_cast<size_t>(offset + BufSize) + padding);
                output.resize(static_cast<size_t>(offset + BufSize));
            }
            else if (r == Z_STREAM_END) {
                output.resize(static_cast<size_t>(offset + BufSize) - strm.avail_out);
                output.reserve(static_cast<size_t>(offset + BufSize) - strm.avail_out + padding);
                break;
            }
            else {
                throw std::runtime_error("gzip decompression failed, code: " + std::to_string(r));
            }
        }
        inflateEnd(&strm);
        return output;
    }
    catch (std::exception&) {
        inflateEnd(&strm);
        throw;
    }

} // ae::file::gzip_decompress

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
