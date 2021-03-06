#pragma once

#include <unordered_map>

#include "chart/v3/common.hh"
#include "chart/v3/titers.hh"

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
        template <typename Index> struct target_index_common_t
        {
            // target_index_common_t() = default;
            // target_index_common_t& operator=(size_t a_index) { index = a_index; return *this; }
            // target_index_common_t& operator=(const target_index_common_t& src) { index = src.index; common = true; return *this; }
            Index index{invalid_index};
            bool common{false};
        };

        // primary/secondary index -> (target index, if common)
        template <typename Index> using index_mapping_t = std::unordered_map<Index, target_index_common_t<Index>, index_hash_for_unordered_map, std::equal_to<>>;

        merge_data_t(merge_data_t&&) = default;
        merge_data_t(std::shared_ptr<Chart> chart1, std::shared_ptr<Chart> chart2, const merge_settings_t& settings, common_antigens_sera_t&& a_common)
            : chart1_{chart1}, chart2_{chart2}, common_{std::move(a_common)}
        {
            build(settings);
        }

        const auto& common() const { return common_; }

        std::string common_report(size_t indent) const { return common().report(indent); }

        auto number_of_antigens_in_merge() const { return target_antigens_; }
        auto number_of_sera_in_merge() const { return target_sera_; }
        auto number_of_points_in_merge() const { return target_antigens_ + target_sera_; }
        const auto& antigens_primary_target() const { return antigens_primary_target_; }
        const auto& antigens_secondary_target() const { return antigens_secondary_target_; }
        const auto& sera_primary_target() const { return sera_primary_target_; }
        const auto& sera_secondary_target() const { return sera_secondary_target_; }

        void titer_report(Titers::titer_merge_report&& tr) { titer_report_ = std::move(tr); }
        const auto& titer_report() const { return titer_report_; }

      private:
        std::shared_ptr<Chart> chart1_, chart2_; // keep charts because common_ uses Antigens& and Sera&
        common_antigens_sera_t common_;
        index_mapping_t<antigen_index> antigens_primary_target_{}, antigens_secondary_target_{};
        index_mapping_t<serum_index> sera_primary_target_{}, sera_secondary_target_{};
        antigen_index target_antigens_{0};
        serum_index target_sera_{0};
        Titers::titer_merge_report titer_report_{};

        void build(const merge_settings_t& settings);
        antigen_indexes secondary_antigens_to_merge(const merge_settings_t& settings) const;
    };

    // ----------------------------------------------------------------------

    std::pair<std::shared_ptr<Chart>, merge_data_t> merge(std::shared_ptr<Chart> chart1, std::shared_ptr<Chart> chart2, const merge_settings_t& settings);

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
