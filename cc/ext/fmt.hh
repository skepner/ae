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

// 9.0.0 clang 14
#pragma GCC diagnostic ignored "-Wfloat-equal" // fmt/format.h:2484

// 9.1.0 clang 14
#pragma GCC diagnostic ignored "-Wextra-semi-stmt" // fmt/ranges.h:510

#elif defined(__GNUG__)

#pragma GCC diagnostic ignored "-Wdeprecated" // fmt/format.h: implicit capture of ‘this’ via ‘[=]’ is deprecated in C++20
#pragma GCC diagnostic ignored "-Weffc++"

#endif

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/chrono.h>

#pragma GCC diagnostic pop

#include <optional>

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
//     template <typename FormatCtx> constexpr auto format(const ###& value, FormatCtx& ctx) const
//     {
//         format_to(ctx.out(), "{} {}", );
//         return format_to(ctx.out(), "{} {}", );
//         return ctx.out();
//     }
// };


// ----------------------------------------------------------------------

namespace ae
{
    inline std::string format_double(double value)
    {
        if (const auto abs = std::abs(value); abs > 1e16 || abs < 1e-16)
            return fmt::format("{:.32g}", value);

        using namespace std::string_view_literals;
        const auto zeros{"000000000000"sv};
        const auto nines{"999999999999"sv};

        auto res = fmt::format("{:.17f}", value);
        if (const auto many_zeros = res.find(zeros); many_zeros != std::string::npos) {
            if (many_zeros > 0 && res[many_zeros - 1] == '.')
                return res.substr(0, many_zeros - 1); // remove trailing dot
            else
                return res.substr(0, many_zeros);
        }
        else if (const auto many_nines = res.find(nines); many_nines != std::string::npos && many_nines > 1) {
            const auto len = res[many_nines - 1] == '.' ? many_nines - 1 : many_nines;
            ++res[len - 1];
            return res.substr(0, len);
        }
        else
            return res;
    }
} // namespace ae

// ----------------------------------------------------------------------

template <> struct fmt::formatter<std::optional<std::string>> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> constexpr auto format(const std::optional<std::string>& str, FormatCtx& ctx) const
    {
        if (str.has_value())
            return format_to(ctx.out(), "\"{}\"", *str);
        else
            return format_to(ctx.out(), "<none>");
    }
};

// ----------------------------------------------------------------------
