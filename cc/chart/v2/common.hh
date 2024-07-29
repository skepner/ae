#pragma once

#include "chart/v2/chart.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v2
{
    namespace common
    {
        struct CoreEntry
        {
            CoreEntry() = default;
            CoreEntry(const CoreEntry&) = default;
            CoreEntry(CoreEntry&&) = default;
            template <typename AgSr> CoreEntry(size_t a_index, const AgSr& ag_sr) : index(a_index), name(ag_sr.name()), reassortant(ag_sr.reassortant()), annotations(ag_sr.annotations()) {}
            virtual ~CoreEntry() = default;
            CoreEntry& operator=(const CoreEntry&) = default;
            CoreEntry& operator=(CoreEntry&&) = default;

            static inline auto compare(const CoreEntry& lhs, const CoreEntry& rhs)
            {
                if (auto n_c = lhs.name <=> rhs.name; n_c != std::strong_ordering::equal)
                    return n_c;
                if (auto r_c = lhs.reassortant <=> rhs.reassortant; r_c != std::strong_ordering::equal)
                    return r_c;
                return fmt::format("{: }", lhs.annotations) <=> fmt::format("{: }", rhs.annotations);
            }

            static inline bool less(const CoreEntry& lhs, const CoreEntry& rhs) { return compare(lhs, rhs) == std::strong_ordering::less; }

            virtual std::string full_name() const = 0; // for make_orig()

            size_t index{0};
            ae::virus::Name name{};
            ae::virus::Reassortant reassortant{};
            Annotations annotations{};
            std::string orig_full_name_{};

            void make_orig()    // to report if fields were updated by antigen_selector_t or serum_selector_t (in acmacs-py)
            {
                if (orig_full_name_.empty())
                    orig_full_name_ = full_name();
            }

        }; // struct CoreEntry

        struct AntigenEntry : public CoreEntry
        {
            AntigenEntry() = default;
            AntigenEntry(size_t a_index, const Antigen& antigen) : CoreEntry(a_index, antigen), passage(antigen.passage()) {}

            std::string_view ag_sr() const
            {
                using namespace std::string_view_literals;
                return "AG"sv;
            }
            std::string full_name() const override
            {
                auto fname = ae::string::join(" ", name, *reassortant, ae::string::join(" ", annotations), passage);
                if (!orig_full_name_.empty())
                    fname += fmt::format(" (orig: {})", orig_full_name_);
                return fname;
            }
            size_t full_name_length() const
            {
                return name.size() + reassortant.size() + annotations.total_length() + passage.size() + 1 + (reassortant.empty() ? 0 : 1) + annotations->size() +
                       (orig_full_name_.empty() ? 0 : orig_full_name_.size() + 9);
            }

            static inline auto compare(const AntigenEntry& lhs, const AntigenEntry& rhs)
            {
                if (auto np_c = CoreEntry::compare(lhs, rhs); np_c != std::strong_ordering::equal)
                    return np_c;
                return lhs.passage <=> rhs.passage;
            }

            bool operator<(const AntigenEntry& rhs) const { return compare(*this, rhs) == std::strong_ordering::less; }

            ae::virus::Passage passage{};

        }; // class AntigenEntry

        struct SerumEntry : public CoreEntry
        {
            SerumEntry() = default;
            SerumEntry(size_t a_index, const Serum& serum) : CoreEntry(a_index, serum), serum_id(serum.serum_id()), passage(serum.passage()) {}

            std::string_view ag_sr() const
            {
                using namespace std::string_view_literals;
                return "SR"sv;
            }
            std::string full_name() const override
            {
                auto fname = ae::string::join(" ", name, *reassortant, ae::string::join(" ", annotations), *serum_id, passage);
                if (!orig_full_name_.empty())
                    fname += fmt::format(" (orig: {})", orig_full_name_);
                return fname;
            }
            size_t full_name_length() const
            {
                return name.size() + reassortant.size() + annotations.total_length() + serum_id.size() + 1 + (reassortant.empty() ? 0 : 1) + annotations->size() + passage.size() +
                    (passage.empty() ? 0 : 1) + (orig_full_name_.empty() ? 0 : orig_full_name_.size() + 9);
            }

            static inline auto compare(const SerumEntry& lhs, const SerumEntry& rhs)
            {
                if (auto np_c = CoreEntry::compare(lhs, rhs); np_c != std::strong_ordering::equal)
                    return np_c;
                return lhs.serum_id <=> rhs.serum_id;
            }

            bool operator<(const SerumEntry& rhs) const { return compare(*this, rhs) == std::strong_ordering::less; }

            SerumId serum_id{};
            ae::virus::Passage passage{};

        }; // class SerumEntry

        using antigen_selector_t = std::function<AntigenEntry(size_t, std::shared_ptr<Antigen>)>;
        using serum_selector_t = std::function<SerumEntry(size_t, std::shared_ptr<Serum>)>;

    } // namespace common

    // ----------------------------------------------------------------------

    class CommonAntigensSera
    {
      public:
        enum class match_level_t { strict, relaxed, ignored, automatic };

        CommonAntigensSera(const Chart& primary, const Chart& secondary, match_level_t match_level);
        CommonAntigensSera(const Chart& primary, const Chart& secondary, common::antigen_selector_t antigen_entry_extractor, common::serum_selector_t serum_entry_extractor, match_level_t match_level);
        CommonAntigensSera(const Chart& primary); // for procrustes between projections of the same chart
        CommonAntigensSera(const CommonAntigensSera&) = delete;
        CommonAntigensSera(CommonAntigensSera&&);
        ~CommonAntigensSera();
        CommonAntigensSera& operator=(const CommonAntigensSera&) = delete;
        CommonAntigensSera& operator=(CommonAntigensSera&&);

        [[nodiscard]] std::string report(size_t indent = 0) const;
        [[nodiscard]] std::string report_unique(size_t indent = 0) const;
        operator bool() const;
        size_t common_antigens() const;
        size_t common_sera() const;

        void keep_only(const PointIndexList& antigens, const PointIndexList& sera);
        void antigens_only(); // remove sera from common lists
        void sera_only();     // remove antigens from common lists

        struct common_t
        {
            common_t(size_t p, size_t s) : primary(p), secondary(s) {}
            size_t primary;
            size_t secondary;
        };

        std::vector<common_t> antigens() const;
        std::vector<common_t> sera() const; // returns serum indexes (NOT point indexes)!
        std::vector<common_t> sera_as_point_indexes() const;
        std::vector<common_t> points() const;

        Indexes common_primary_antigens() const;
        Indexes common_primary_sera() const; // returns serum indexes (NOT point indexes)!

        // common antigen/serum mapping
        std::optional<size_t> antigen_primary_by_secondary(size_t secondary_no) const;
        std::optional<size_t> antigen_secondary_by_primary(size_t primary_no) const;
        std::optional<size_t> serum_primary_by_secondary(size_t secondary_no) const;
        std::optional<size_t> serum_secondary_by_primary(size_t primary_no) const;

        enum class subset { all, antigens, sera };
        std::vector<common_t> points(subset a_subset) const;
        std::vector<common_t> points_for_primary_antigens(const Indexes& antigen_indexes) const;
        std::vector<common_t> points_for_primary_sera(const Indexes& serum_indexes) const;

        static match_level_t match_level(std::string_view source);

      private:
        class Impl;
        std::unique_ptr<Impl> impl_{};

    }; // class CommonAntigensSera

} // namespace ae::chart::v2

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v2::CommonAntigensSera::common_t> : public fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatContext> auto format(const ae::chart::v2::CommonAntigensSera::common_t& common, FormatContext& ctx) const
    {
        return format_to(ctx.out(), "{{{},{}}}", common.primary, common.secondary);
    }
};

/// ----------------------------------------------------------------------
