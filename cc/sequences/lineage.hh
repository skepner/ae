#pragma once

#include "ext/fmt.hh"
#include "ext/string.hh"

// ======================================================================

namespace ae::sequences
{
    class lineage_t
    {
      public:
        lineage_t() = default;
        explicit lineage_t(std::string_view src)
        {
            using namespace std::string_view_literals;
            if (src == "VICTORIA"sv || src == "YAMAGATA"sv)
                lineage_ = src.substr(0, 1);
            else
                lineage_ = src;
        }

        lineage_t& operator=(std::string_view src) { return operator=(lineage_t{src}); }

        lineage_t(const lineage_t&) = default;
        lineage_t(lineage_t&&) = default;
        lineage_t& operator=(const lineage_t&) = default;
        lineage_t& operator=(lineage_t&&) = default;

        auto operator<=>(const lineage_t&) const = default;

        operator std::string_view() const { return lineage_; }
        operator const std::string&() const { return lineage_; }

        bool empty() const { return lineage_.empty(); }
        explicit operator bool() const { return !empty(); }

      private:
        std::string lineage_;
    };

} // namespace ae::sequences

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::sequences::lineage_t> : fmt::formatter<std::string>
{
    template <typename FormatCtx> constexpr auto format(const ae::sequences::lineage_t& lineage, FormatCtx& ctx) { return fmt::formatter<std::string>::format(lineage, ctx); }
};

// ======================================================================
