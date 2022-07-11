#pragma once

#include "chart/v3/antigens.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    enum class antigens_sera_match_level_t { strict, relaxed, ignored, automatic };

    template <typename AgSrs> class common_data_t
    {
      public:
        using index_t = typename AgSrs::index_t;
        using indexes_t = typename AgSrs::indexes_t;
        using AgSr = typename AgSrs::element_t;
        using common_t = std::pair<index_t, index_t>;
        enum class score_t : size_t { no_match = 0, passage_serum_id_ignored = 1, egg = 2, without_date = 3, full_match = 4 };

        common_data_t(common_data_t&&) = default;
        common_data_t(const AgSrs& primary, const AgSrs& secondary, antigens_sera_match_level_t match_level);
        common_data_t(const AgSrs& primary);

        std::vector<common_t> common() const;
        indexes_t primary() const;
        std::string report(size_t indent) const;

        void clear()
            {
                match_.clear();
                number_of_common_ = 0;
            }

        bool empty() const { return number_of_common_ == 0; }
        size_t number_of_common() const { return number_of_common_; }

        std::optional<index_t> primary_by_secondary(index_t secondary_no) const;
        std::optional<index_t> secondary_by_primary(index_t primary_no) const;

        index_t size_primary() const { return primary_.size(); }
        index_t size_secondary() const { return secondary_.size(); }

      private:
        struct match_t
        {
            index_t primary;
            index_t secondary;
            score_t score;
            bool use{false};
        };

        const AgSrs& primary_;
        const AgSrs& secondary_;
        std::vector<match_t> match_{};
        size_t number_of_common_{0};
        const index_t min_number_;

        score_t match(const AgSr& prim, const AgSr& seco, antigens_sera_match_level_t match_level) const;
        score_t match_not_ignored(const AgSr& prim, const AgSr& seco) const;
        void build_match(antigens_sera_match_level_t match_level);
        void sort_match();
        void mark_match_use(antigens_sera_match_level_t match_level);
        size_t primary_name_max_size() const;

        std::vector<match_t> used() const
            {
                std::vector<match_t> result;
                for (const auto& en : match_) {
                    if (en.use)
                        result.push_back(en);
                }
                return result;
            }
    };

    extern template common_data_t<Antigens>::common_data_t(const Antigens& primary, const Antigens& secondary, antigens_sera_match_level_t match_level);
    extern template common_data_t<Sera>::common_data_t(const Sera& primary, const Sera& secondary, antigens_sera_match_level_t match_level);
    extern template common_data_t<Antigens>::common_data_t(const Antigens& primary);
    extern template common_data_t<Sera>::common_data_t(const Sera& primary);

    extern template std::vector<common_data_t<Antigens>::common_t> common_data_t<ae::chart::v3::Antigens>::common() const;
    extern template std::vector<common_data_t<Sera>::common_t> common_data_t<ae::chart::v3::Sera>::common() const;

    extern template std::optional<antigen_index> common_data_t<Antigens>::primary_by_secondary(ae::antigen_index secondary_no) const;
    extern template std::optional<serum_index> common_data_t<Sera>::primary_by_secondary(ae::serum_index secondary_no) const;

    extern template std::optional<antigen_index> common_data_t<Antigens>::secondary_by_primary(ae::antigen_index primary_no) const;
    extern template std::optional<serum_index> common_data_t<Sera>::secondary_by_primary(ae::serum_index primary_no) const;

    extern template std::string common_data_t<Antigens>::report(size_t indent) const;
    extern template std::string common_data_t<Sera>::report(size_t indent) const;

    extern template size_t ae::chart::v3::common_data_t<ae::chart::v3::Antigens>::primary_name_max_size() const;
    extern template size_t ae::chart::v3::common_data_t<ae::chart::v3::Sera>::primary_name_max_size() const;

    extern template ae::antigen_indexes ae::chart::v3::common_data_t<ae::chart::v3::Antigens>::primary() const;
    extern template ae::serum_indexes ae::chart::v3::common_data_t<ae::chart::v3::Sera>::primary() const;

    // ----------------------------------------------------------------------

    class Chart;

    class common_antigens_sera_t
    {
      public:
        using common_t = std::pair<point_index, point_index>;

        common_antigens_sera_t(common_antigens_sera_t&&) = default;
        common_antigens_sera_t(const Chart& primary, const Chart& secondary, antigens_sera_match_level_t match_level);
        common_antigens_sera_t(const Chart& primary); // procrustes between projections of the same chart

        bool empty() const { return antigens_.empty() && sera_.empty(); }
        size_t common_antigens() const { return antigens_.number_of_common(); }
        size_t common_sera() const { return sera_.number_of_common(); }

        //   void keep_only(const PointIndexList& antigens, const PointIndexList& sera);

        void antigens_only() { sera_.clear(); }
        void sera_only() { antigens_.clear(); }

        auto antigens() const { return antigens_.common(); }
        auto sera() const { return sera_.common(); }
        std::vector<common_t> points() const;

        auto primary_antigens() const { return antigens_.primary(); }
        auto primary_sera() const { return sera_.primary(); }

        //   // common antigen/serum mapping
        std::optional<antigen_index> antigen_primary_by_secondary(antigen_index secondary_no) const { return antigens_.primary_by_secondary(secondary_no); }
        std::optional<antigen_index> antigen_secondary_by_primary(antigen_index primary_no) const { return antigens_.secondary_by_primary(primary_no); }
        std::optional<serum_index> serum_primary_by_secondary(serum_index secondary_no) const { return sera_.primary_by_secondary(secondary_no); }
        std::optional<serum_index> serum_secondary_by_primary(serum_index primary_no) const { return sera_.secondary_by_primary(primary_no); }

        //   enum class subset { all, antigens, sera };
        //   std::vector<common_t> points(subset a_subset) const;
        //   std::vector<common_t> points_for_primary_antigens(const Indexes& antigen_indexes) const;
        //   std::vector<common_t> points_for_primary_sera(const Indexes& serum_indexes) const;

        [[nodiscard]] std::string report(size_t indent) const;
        //   [[nodiscard]] std::string report_unique(size_t indent = 0) const;

      private:
        common_data_t<Antigens> antigens_;
        common_data_t<Sera> sera_;

    }; // class CommonAntigensSera

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

// template <> struct fmt::formatter<ae::chart::v3::CommonAntigensSera::common_t> : public fmt::formatter<acmacs::fmt_helper::default_formatter>
// {
//     template <typename FormatContext> auto format(const ae::chart::v3::CommonAntigensSera::common_t& common, FormatContext& ctx) const
//     {
//         return format_to(ctx.out(), "{{{},{}}}", common.primary, common.secondary);
//     }
// };

/// ----------------------------------------------------------------------

    // namespace common
    // {
    //     struct CoreEntry
    //     {
    //         CoreEntry() = default;
    //         CoreEntry(const CoreEntry&) = default;
    //         CoreEntry(CoreEntry&&) = default;
    //         template <typename AgSr> CoreEntry(size_t a_index, const AgSr& ag_sr) : index(a_index), name(ag_sr.name()), reassortant(ag_sr.reassortant()), annotations(ag_sr.annotations()) {}
    //         // virtual ~CoreEntry() = default;
    //         CoreEntry& operator=(const CoreEntry&) = default;
    //         CoreEntry& operator=(CoreEntry&&) = default;

    //         auto operator<=>(const CoreEntry& rhs) const
    //         {
    //             if (const auto cmp = name <=> rhs.name; cmp != std::strong_ordering::equal)
    //                 return cmp;
    //             if (const auto cmp = annotations <=> rhs.annotations; cmp != std::strong_ordering::equal)
    //                 return cmp;
    //             if (const auto cmp = reassortant <=> rhs.reassortant; cmp != std::strong_ordering::equal)
    //                 return cmp;
    //             return std::strong_ordering::equal;
    //         }

    //         // static inline int compare(const CoreEntry& lhs, const CoreEntry& rhs)
    //         // {
    //         //     if (auto n_c = lhs.name.compare(rhs.name); n_c != 0)
    //         //         return n_c;
    //         //     if (auto r_c = lhs.reassortant.compare(rhs.reassortant); r_c != 0)
    //         //         return r_c;
    //         //     return ::string::compare(fmt::format("{: }", lhs.annotations), fmt::format("{: }", rhs.annotations));
    //         // }

    //         // static inline bool less(const CoreEntry& lhs, const CoreEntry& rhs) { return compare(lhs, rhs) < 0; }

    //         // virtual std::string full_name() const = 0; // for make_orig()

    //         virus::Name name;
    //         virus::Reassortant reassortant;
    //         Annotations annotations;
    //         std::string orig_full_name_;

    //         void make_orig() // to report if fields were updated by antigen_selector_t or serum_selector_t (in acmacs-py)
    //         {
    //             if (orig_full_name_.empty())
    //                 orig_full_name_ = full_name();
    //         }

    //     }; // struct CoreEntry

    //     struct AntigenEntry : public CoreEntry
    //     {
    //         AntigenEntry() = default;
    //         AntigenEntry(size_t a_index, const Antigen& antigen) : CoreEntry(a_index, antigen), passage(antigen.passage()) {}

    //         std::string_view ag_sr() const { return "AG"sv; }
    //         std::string full_name() const
    //         {
    //             auto fname = acmacs::string::join(acmacs::string::join_space, name, reassortant, acmacs::string::join(acmacs::string::join_space, annotations), passage);
    //             if (!orig_full_name_.empty())
    //                 fname += fmt::format(" (orig: {})", orig_full_name_);
    //             return fname;
    //         }
    //         size_t full_name_length() const
    //         {
    //             return name.size() + reassortant.size() + annotations.total_length() + passage.size() + 1 + (reassortant.empty() ? 0 : 1) + annotations->size() +
    //                    (orig_full_name_.empty() ? 0 : orig_full_name_.size() + 9);
    //         }
    //         bool operator<(const AntigenEntry& rhs) const { return compare(*this, rhs) < 0; }

    //         static inline int compare(const AntigenEntry& lhs, const AntigenEntry& rhs)
    //         {
    //             if (auto np_c = CoreEntry::compare(lhs, rhs); np_c != 0)
    //                 return np_c;
    //             return lhs.passage.compare(rhs.passage);
    //         }

    //         antigen_index index;
    //         virus::Passage passage;

    //     }; // class AntigenEntry

    //     struct SerumEntry : public CoreEntry
    //     {
    //         SerumEntry() = default;
    //         SerumEntry(size_t a_index, const Serum& serum) : CoreEntry(a_index, serum), serum_id(serum.serum_id()), passage(serum.passage()) {}

    //         std::string_view ag_sr() const { return "SR"; }

    //         std::string full_name() const override
    //         {
    //             auto fname = acmacs::string::join(acmacs::string::join_space, name, reassortant, acmacs::string::join(acmacs::string::join_space, annotations), serum_id, passage);
    //             if (!orig_full_name_.empty())
    //                 fname += fmt::format(" (orig: {})", orig_full_name_);
    //             return fname;
    //         }
    //         size_t full_name_length() const
    //         {
    //             return name.size() + reassortant.size() + annotations.total_length() + serum_id.size() + 1 + (reassortant.empty() ? 0 : 1) + annotations->size() + passage.size() +
    //                    (passage.empty() ? 0 : 1) + (orig_full_name_.empty() ? 0 : orig_full_name_.size() + 9);
    //         }
    //         bool operator<(const SerumEntry& rhs) const { return compare(*this, rhs) < 0; }

    //         static inline int compare(const SerumEntry& lhs, const SerumEntry& rhs)
    //         {
    //             if (auto np_c = CoreEntry::compare(lhs, rhs); np_c != 0)
    //                 return np_c;
    //             return lhs.serum_id.compare(rhs.serum_id);
    //         }

    //         serum_index index;
    //         SerumId serum_id;
    //         virus::Passage passage;

    //     }; // class SerumEntry

    // using antigen_selector_t = std::function<AntigenEntry(size_t, std::shared_ptr<Antigen>)>;
    // using serum_selector_t = std::function<SerumEntry(size_t, std::shared_ptr<Serum>)>;

    // } // namespace common

    // ----------------------------------------------------------------------
