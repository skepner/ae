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
        antigens_sera_match_level_t match_level{antigens_sera_match_level_t::automatic};
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
