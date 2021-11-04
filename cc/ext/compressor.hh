#pragma once

#include <stdexcept>
#include <string_view>

// ----------------------------------------------------------------------

namespace ae::file
{
    class compressor_failed : public std::runtime_error { public: using std::runtime_error::runtime_error; };

    class Compressor
    {
      public:
        Compressor(size_t padding) : padding_{padding} {}
        virtual ~Compressor() = default;

        virtual std::string compress(std::string_view input) = 0;
        virtual std::string decompress(std::string_view input) = 0;

      protected:
        size_t padding() const { return padding_; }

      private:
        size_t padding_;
    };

    // ----------------------------------------------------------------------

    // class NotCompressed : public Compressor
    // {
    //   public:
    //     using Compressor::Compressor;
    //     ~NotCompressed() override = default;

    //     std::string compress(std::string_view input) override { return input; }

    //     std::string decompress(std::string_view input) override { return input; }
    // };

} // namespace ae::file

// ======================================================================
