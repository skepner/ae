#pragma once

#include "chart/v2/selected-antigens-sera.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v2
{
    enum class org_mode_separators_t { no, yes };
    enum class show_aa_t { no, yes };

    std::string export_text(const Chart& chart);
    std::string export_table_to_text(const Chart& chart, std::optional<size_t> just_layer = std::nullopt, bool sort = false, org_mode_separators_t org_mode_separators = org_mode_separators_t::no, show_aa_t show_aa = show_aa_t::yes);
    std::string export_info_to_text(const Chart& chart);

    template <typename SA, typename SS> std::string export_names_to_text(const Chart& chart, std::string_view format, const SA& antigens, const SS& sera)
    {
        fmt::memory_buffer out;
        for (const auto ag_no : antigens.indexes)
            fmt::format_to(std::back_inserter(out), "{}", ae::chart::v2::format_antigen(format, chart, ag_no, collapse_spaces_t::yes));
        fmt::format_to(std::back_inserter(out), "\n");
        for (const auto sr_no : sera.indexes)
            fmt::format_to(std::back_inserter(out), "{}", ae::chart::v2::format_serum(format, chart, sr_no, collapse_spaces_t::yes));
        return fmt::to_string(out);
    }

    inline std::string export_names_to_text(std::shared_ptr<Chart> chart, std::string_view format) { return export_names_to_text(*chart, format, SelectedAntigens{chart}, SelectedSera{chart}); }

} // namespace ae::chart::v2

// ----------------------------------------------------------------------
