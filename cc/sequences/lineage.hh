#pragma once

#include "ext/fmt.hh"
#include "ext/compare.hh"
#include "fmt/core.h"

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
        const std::string& get() const { return lineage_; }

        bool empty() const { return lineage_.empty(); }
        explicit operator bool() const { return !empty(); }

      private:
        std::string lineage_{};
    };

    struct lineage_t_hash_for_unordered_map
    {
        using is_transparent = void;
        [[nodiscard]] size_t operator()(const sequences::lineage_t& txt) const { return std::hash<std::string_view>{}(txt); }
    };

} // namespace ae::sequences

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::sequences::lineage_t> : fmt::formatter<std::string>
{
    auto format(const ae::sequences::lineage_t& lineage, format_context& ctx) const { return fmt::format_to(ctx.out(), "{}", static_cast<std::string_view>(lineage)); }
};

// ======================================================================
