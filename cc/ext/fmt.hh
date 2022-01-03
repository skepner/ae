#pragma once

#pragma GCC diagnostic push

#if defined(__clang__)

// 8.1.0, clang 13
#pragma GCC diagnostic ignored "-Wc++20-compat"
#pragma GCC diagnostic ignored "-Wdeprecated-declarations" // fmt/chrono.h
#pragma GCC diagnostic ignored "-Wdocumentation-unknown-command"
#pragma GCC diagnostic ignored "-Wmissing-noreturn" // fmt/core.h
#pragma GCC diagnostic ignored "-Wreserved-identifier" // identifier '_a' is reserved because it starts with '_' at global scope (bug in clang13 ?)
#pragma GCC diagnostic ignored "-Wsigned-enum-bitfield" // fmt/format.h
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wundefined-func-template" // fmt/chrono.h:1182

#elif defined(__GNUG__)

#pragma GCC diagnostic ignored "-Wdeprecated" // fmt/format.h: implicit capture of ‘this’ via ‘[=]’ is deprecated in C++20
#pragma GCC diagnostic ignored "-Weffc++"

#endif

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/chrono.h>

#pragma GCC diagnostic pop

// ======================================================================

namespace ae::fmt_helper
{
    struct default_formatter
    {
    };

    struct float_formatter
    {
    };

} // namespace ae::fmt_helper

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename ParseContext> constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    }
};

template <> struct fmt::formatter<ae::fmt_helper::float_formatter>
{
    template <typename ParseContext> constexpr auto parse(ParseContext& ctx)
    {
        auto it = ctx.begin();
        if (it != ctx.end() && *it == ':')
            ++it;
        const auto end = std::find(it, ctx.end(), '}');
        format_ = fmt::format("{{:{}}}", std::string_view(it, static_cast<size_t>(end - it)));
        return end;
    }

    template <typename Val> std::string format_val(Val&& val) const
    {
        return fmt::format(fmt::runtime(format_), std::forward<Val>(val));
    }

    template <typename Val, typename FormatContext> auto format_val(Val&& val, FormatContext& ctx) const
    {
        return format_to(ctx.out(), fmt::runtime(format_), std::forward<Val>(val));
    }

  private:
    std::string format_{};
};

// ----------------------------------------------------------------------

// template <> struct fmt::formatter<###> : fmt::formatter<ae::fmt_helper::default_formatter> {
//     template <typename FormatCtx> auto format(const ###& value, FormatCtx& ctx)
//     {
//         format_to(ctx.out(), "{} {}", );
//         return format_to(ctx.out(), "{} {}", );
//         return ctx.out();
//     }
// };


// ----------------------------------------------------------------------
