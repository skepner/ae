#pragma once

#include "chart/v3/common.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    enum class projection_merge_t { type1, type2, type3, type4, type5 }; // see ../doc/merge-types.org
    enum class remove_distinct { no, yes };
    enum class combine_cheating_assays { no, yes };

    struct merge_settings_t
    {
        // merge_settings_t() = default;
        // merge_settings_t(CommonAntigensSera::match_level_t a_match_level, projection_merge_t a_projection_merge = projection_merge_t::type1,
        //               combine_cheating_assays a_combine_cheating_assays = combine_cheating_assays::no, remove_distinct a_remove_distinct = remove_distinct::no)
        //     : match_level{a_match_level}, projection_merge{a_projection_merge}, combine_cheating_assays_{a_combine_cheating_assays}, remove_distinct_{a_remove_distinct}
        // {
        // }
        // merge_settings_t(projection_merge_t a_projection_merge) : projection_merge{a_projection_merge} {}

        common_antigens_sera_t::match_level_t match_level{common_antigens_sera_t::match_level_t::automatic};
        projection_merge_t projection_merge{projection_merge_t::type1};
        combine_cheating_assays combine_cheating_assays_{combine_cheating_assays::no};
        remove_distinct remove_distinct_{remove_distinct::no};
    };

    // ----------------------------------------------------------------------

    struct merge_report_t
    {
    };

    // ----------------------------------------------------------------------

    std::pair<std::shared_ptr<Chart>, merge_report_t> merge(const Chart& chart1, const Chart& chart2, const merge_settings_t& settings);

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
