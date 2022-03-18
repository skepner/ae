#pragma once

#include <unordered_map>

#include "chart/v3/common.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class merge_error : public std::runtime_error { public: using std::runtime_error::runtime_error; };

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

    class merge_data_t
    {
      public:
        merge_data_t(merge_data_t&&) = default;
        merge_data_t(const Chart& chart1, const Chart& chart2, const merge_settings_t& settings, common_antigens_sera_t&& a_common) : common_{std::move(a_common)} { build(chart1, chart2, settings); }

        // std::string titer_merge_report(const Chart& chart) const;
        // std::string titer_merge_report_common_only(const Chart& chart) const;
        // std::string titer_merge_diagnostics(const Chart& chart, const PointIndexList& antigens, const PointIndexList& sera, int max_field_size) const;

      private:
        template <typename Index> struct target_index_common_t
        {
            // target_index_common_t() = default;
            // target_index_common_t& operator=(size_t a_index) { index = a_index; return *this; }
            // target_index_common_t& operator=(const target_index_common_t& src) { index = src.index; common = true; return *this; }
            Index index{invalid_index};
            bool common{false};
        };

        template <typename Index> using index_mapping_t = std::unordered_map<Index, target_index_common_t<Index>, index_hash_for_unordered_map, std::equal_to<>>; // primary/secondary index -> (target index, if common)

        common_antigens_sera_t common_;
        index_mapping_t<antigen_index> antigens_primary_target_, antigens_secondary_target_;
        index_mapping_t<serum_index> sera_primary_target_, sera_secondary_target_;
        antigen_index target_antigens_{0};
        serum_index target_sera_{0};
        // std::unique_ptr<Titers::titer_merge_report> titer_report_;

        void build(const Chart& chart1, const Chart& chart2, const merge_settings_t& settings);
        antigen_indexes secondary_antigens_to_merge(const Chart& primary, const Chart& secondary, const merge_settings_t& settings) const;
    };

    // ----------------------------------------------------------------------

    std::pair<std::shared_ptr<Chart>, merge_data_t> merge(const Chart& chart1, const Chart& chart2, const merge_settings_t& settings);

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
