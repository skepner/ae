#pragma once

#include <stdexcept>
#include <string_view>

// ----------------------------------------------------------------------

namespace ae::file
{
    class compressor_failed : public std::runtime_error { public: using std::runtime_error::runtime_error; };

    class Compressed
    {
      public:
        enum class first_chunk { no, yes };

        Compressed(size_t padding) : padding_{padding} {}
        virtual ~Compressed() = default;

        virtual std::string_view compress(std::string_view input) = 0;
        virtual std::string_view decompress(std::string_view input, first_chunk fc) = 0; // returns decompressed data and its allocated capacity

      protected:
        size_t padding() const { return padding_; }

      private:
        size_t padding_;
    };

    // ----------------------------------------------------------------------

    class NotCompressed : public Compressed
    {
      public:
        using Compressed::Compressed;
        ~NotCompressed() override = default;

        std::string_view compress(std::string_view input) override
        {
            return input;
        }

        std::string_view decompress(std::string_view input, first_chunk) override { return input; }
    };

} // namespace ae::file

// ======================================================================
