#pragma once

#include <string>

#include "ext/fmt.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v2
{
    class Chart;
    class Antigens;
    class Sera;

    enum class collapse_spaces_t { no, yes };

    std::string collapse_spaces(std::string src, collapse_spaces_t cs);
    std::string format_antigen(std::string_view pattern, const Chart& chart, size_t antigen_no, collapse_spaces_t cs);
    std::string format_serum(std::string_view pattern, const Chart& chart, size_t serum_no, collapse_spaces_t cs);

    template <typename AgSr> std::string format_antigen_serum(std::string_view pattern, const Chart& chart, size_t no, collapse_spaces_t cs)
    {
        if constexpr (std::is_base_of_v<Antigens, AgSr>)
            return format_antigen(pattern, chart, no, cs);
        else if constexpr (std::is_base_of_v<Sera, AgSr>)
            return format_serum(pattern, chart, no, cs);
        else
            static_assert(std::is_same_v<AgSr, Sera>);
    }

    template <typename AgSr> void format_antigen_serum(fmt::memory_buffer& out, std::string_view pattern, const Chart& chart, size_t no, collapse_spaces_t cs)
    {
        if constexpr (std::is_base_of_v<Antigens, AgSr>)
            fmt::format_to(std::back_inserter(out), "{}", format_antigen(pattern, chart, no, cs));
        else if constexpr (std::is_base_of_v<Sera, AgSr>)
            fmt::format_to(std::back_inserter(out), "{}", format_serum(pattern, chart, no, cs));
        else
            static_assert(std::is_same_v<AgSr, Sera>);
    }

    std::string format_point(std::string_view pattern, const Chart& chart, size_t point_no, collapse_spaces_t cs);

    std::string format_help();

} // namespace ae::chart::v2

// ----------------------------------------------------------------------
