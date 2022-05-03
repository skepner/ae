#pragma once

#include <vector>
#include <string>

#include "ext/fmt.hh"

// ----------------------------------------------------------------------

namespace ae::draw::v2
{
    class Color
    {
      public:
        Color() = default;
        Color(const Color&) = default;
        Color(Color&&) = default;
        Color(std::string_view val) : blocks_{std::string{val}} {}
        Color& operator=(const Color&) = default;
        Color& operator=(Color&&) = default;
        Color& operator=(std::string_view val) { blocks_ = std::vector<std::string>{std::string{val}}; return *this; }

        bool operator==(const Color&) const noexcept = default;

        bool empty() const { return blocks_.empty() || blocks_[0].empty(); }
        const auto& blocks() const { return blocks_; }

      private:
        std::vector<std::string> blocks_;
    };

} // namespace ae::draw::v2

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::draw::v2::Color> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::draw::v2::Color& color, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "{}", fmt::join(color.blocks(), "/"));
    }
};

// ----------------------------------------------------------------------
