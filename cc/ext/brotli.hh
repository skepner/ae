#pragma once

#include <cstring>
#include <string>
#include <string_view>
#include <stdexcept>

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wdocumentation-pedantic"
#pragma GCC diagnostic ignored "-Wdocumentation-unknown-command"
#endif

#include <brotli/decode.h>
#include <brotli/encode.h>

#pragma GCC diagnostic pop

#include "ext/compressor.hh"

// ----------------------------------------------------------------------

namespace ae::file
{
    class Brotli_Compressor : public Compressor
    {
      public:
        Brotli_Compressor(size_t padding = 0) : Compressor(padding) {}

        ~Brotli_Compressor() override
        {
            if (encoder_)
                BrotliEncoderDestroyInstance(encoder_);
            if (decoder_)
                BrotliDecoderDestroyInstance(decoder_);
        }

        std::string compress(std::string_view input) override
        {
            std::string output(std::min(input.size() / 2 + 1, 1024ul * 1024), 0);
            if (encoder_)
                BrotliEncoderDestroyInstance(encoder_);
            encoder_ = BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
            size_t available_in = input.size();
            const auto* next_in = reinterpret_cast<const uint8_t*>(input.data());
            auto* next_out = reinterpret_cast<uint8_t*>(output.data());
            size_t available_out = output.size();
            while (!BrotliEncoderIsFinished(encoder_)) {
                if (BrotliEncoderCompressStream(encoder_, BROTLI_OPERATION_FINISH, &available_in, &next_in, &available_out, &next_out, nullptr) == BROTLI_FALSE)
                    throw compressor_failed{"brotli compression failed 1"};
                if (available_out == 0) {
                    const auto used = output.size();
                    output.resize(used * 2, 0);
                    next_out = reinterpret_cast<uint8_t*>(output.data() + used);
                    available_out = output.size() - used;
                }
            }
            BrotliEncoderDestroyInstance(encoder_);
            encoder_ = nullptr;
            output.resize(output.size() - available_out);
            return output;
        }

        enum class check_if_compressed { no, yes };

        std::string decompress(std::string_view input) override { return decompress_and_check(input, check_if_compressed::no); }

        std::string decompress_and_check(std::string_view input, check_if_compressed cic)
        {
            std::string output;
            if (decoder_)
                BrotliDecoderDestroyInstance(decoder_);
            decoder_ = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
            BrotliDecoderResult result = BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT;
            const uint8_t* next_in = reinterpret_cast<const uint8_t*>(input.data());
            // const auto* next_in = input.data();
            size_t available_in = input.size();
            while (result == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT) {
                size_t available_out = 0;
                result = BrotliDecoderDecompressStream(decoder_, &available_in, &next_in, &available_out, nullptr, nullptr);
                const uint8_t* next_out = BrotliDecoderTakeOutput(decoder_, &available_out);
                // fmt::print(">>>> result: {} available_out: {}\n", result, available_out);
                if (available_out != 0)
                    output.insert(output.end(), next_out, next_out + available_out);
            }
            BrotliDecoderDestroyInstance(decoder_);
            decoder_ = nullptr;
            if ((cic == check_if_compressed::yes && (result == BROTLI_DECODER_RESULT_SUCCESS || result == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT) && !output.empty()) ||
                (result == BROTLI_DECODER_RESULT_SUCCESS && !available_in)) {
                output.reserve(output.size() + padding());
                return output;
            }
            else
                throw compressor_failed{fmt::format("brotli decompression failed: {}", result)};
        }

      private:
        BrotliEncoderState* encoder_{nullptr};
        BrotliDecoderState* decoder_{nullptr};
    };

    // ----------------------------------------------------------------------

    inline bool brotli_compressed(std::string_view input)
    {
        try {
            Brotli_Compressor().decompress_and_check(input.substr(0, 100), Brotli_Compressor::check_if_compressed::yes);
            return true;
        }
        catch (compressor_failed&) {
            return false;
        }
    }

} // namespace ae::file

// ----------------------------------------------------------------------
