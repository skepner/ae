#pragma once

#include <regex>
#include <optional>

#include "ext/fmt.hh"

// ======================================================================
// Support for multiple regex and replacements, see acmacs-virus/cc/reassortant.cc
// ======================================================================

namespace ae::regex
{
    constexpr auto icase = std::regex::icase | std::regex::ECMAScript | std::regex::optimize;

    inline bool search(std::string_view search_in, std::cmatch& match, const std::regex& re) { return std::regex_search(std::begin(search_in), std::end(search_in), match, re); }

    inline bool search(std::string_view search_in, const std::regex& re) { return std::regex_search(std::begin(search_in), std::end(search_in), re); }

    inline bool search(std::string_view search_in, ssize_t offset, std::cmatch& match, const std::regex& re)
    {
        return std::regex_search(std::next(std::begin(search_in), offset), std::end(search_in), match, re);
    }

    inline bool search(std::string_view search_in, ssize_t offset, const std::regex& re) { return std::regex_search(std::next(std::begin(search_in), offset), std::end(search_in), re); }

    // ----------------------------------------------------------------------

    struct look_replace_t
    {
        const std::regex look_for;
        std::vector<const char*> fmt;
    };

    using scan_replace_result_t = std::optional<std::vector<std::string>>;

    template <typename Container> scan_replace_result_t scan_replace(std::string_view source, const Container& scan_data)
    {
        for (const auto& entry : scan_data) {
            if (std::cmatch match; std::regex_search(std::begin(source), std::end(source), match, entry.look_for)) {
                std::vector<std::string> result(entry.fmt.size());
                std::transform(std::begin(entry.fmt), std::end(entry.fmt), std::begin(result), [&match](const char* fmt) { return match.format(fmt); });
                return result;
            }
        }
        return {};
    }

} // namespace acmacs::regex

// ======================================================================
// fmt support for std::smatch and std::cmatch
// ======================================================================

// specialization below follows fmt lib description, but it does not work due to ambiguity with
// template <typename RangeT, typename Char> struct formatter<RangeT, Char, enable_if_t<fmt::is_range<RangeT, Char>::value>>
//
// template <typename Match>  struct fmt::formatter<Match, std::enable_if_t<std::is_same_v<Match, std::smatch> || std::is_same_v<Match, std::cmatch>, char>> : fmt::formatter<acmacs::fmt_helper::default_formatter>

// ----------------------------------------------------------------------

namespace ae
{
    template <typename Match> struct fmt_regex_match_formatter {};
}

template <typename Match> struct fmt::formatter<ae::fmt_regex_match_formatter<Match>> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const Match& mr, FormatCtx& ctx) {
        format_to(ctx.out(), "\"{}\" -> ({})[", mr.str(0), mr.size());
        for (size_t nr = 1; nr <= mr.size(); ++nr)
            format_to(ctx.out(), " {}:\"{}\"", nr, mr.str(nr));
        format_to(ctx.out(), " ]");
        return ctx.out();
    }
};

template <> struct fmt::formatter<std::smatch> : fmt::formatter<ae::fmt_regex_match_formatter<std::smatch>>
{
};

template <> struct fmt::formatter<std::cmatch> : fmt::formatter<ae::fmt_regex_match_formatter<std::cmatch>>
{
};

// ----------------------------------------------------------------------
