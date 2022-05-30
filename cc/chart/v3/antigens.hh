#pragma once

#include <optional>
#include <unordered_map>

#include "ext/compare.hh"
#include "utils/string.hh"
#include "virus/name.hh"
#include "virus/passage.hh"
#include "virus/reassortant.hh"
#include "sequences/lineage.hh"
#include "sequences/sequence.hh"
#include "sequences/clades.hh"
#include "chart/v3/index.hh"
#include "chart/v3/annotations.hh"
#include "chart/v3/semantic.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Chart;

    using Date = named_string_t<std::string, struct date_tag>;

    inline bool within_range(const Date& date, std::string_view first_date, std::string_view after_last_date)
    {
        return !date.empty() && (first_date.empty() || date >= first_date) && (after_last_date.empty() || date < after_last_date);
    }

    // ----------------------------------------------------------------------

    using LabIds = named_vector_t<std::string, struct LabIds_tag>;
    inline std::string join(const LabIds& lab_ids) { return fmt::format("{}", fmt::join(lab_ids, " ")); }

    // ----------------------------------------------------------------------

    using SerumId = named_string_t<std::string, struct SerumId_tag>;

    using SerumSpecies = named_string_t<std::string, struct SerumSpecies_tag>;

    // ----------------------------------------------------------------------

    class AntigenSerum
    {
      public:
        AntigenSerum() = default;
        AntigenSerum(const AntigenSerum&) = default;
        AntigenSerum(AntigenSerum&&) = default;
        AntigenSerum& operator=(const AntigenSerum&) = default;
        AntigenSerum& operator=(AntigenSerum&&) = default;
        bool operator==(const AntigenSerum&) const = default;

        const auto& name() const { return name_; }
        void name(const virus::Name& name) { name_ = name; }
        const auto& annotations() const { return annotations_; }
        Annotations& annotations() { return annotations_; }
        const auto& lineage() const { return lineage_; }
        void lineage(const sequences::lineage_t& lineage) { lineage_ = lineage; }
        const auto& passage() const { return passage_; }
        void passage(const virus::Passage& passage) { passage_ = passage; }
        const auto& reassortant() const { return reassortant_; }
        void reassortant(const virus::Reassortant& reassortant) { reassortant_ = reassortant; }
        const auto& aa() const { return aa_; }
        void aa(const sequences::sequence_aa_t& aa) { aa_ = aa; }
        const auto& nuc() const { return nuc_; }
        void nuc(const sequences::sequence_nuc_t& nuc) { nuc_ = nuc; }
        const auto& aa_insertions() const { return aa_insertions_; }
        void aa_insertions(const sequences::insertions_t& aa_insertions) { aa_insertions_ = aa_insertions; }
        const auto& nuc_insertions() const { return nuc_insertions_; }
        void nuc_insertions(const sequences::insertions_t& nuc_insertions) { nuc_insertions_ = nuc_insertions; }
        const auto& semantic() const { return semantic_; }
        SemanticAttributes& semantic() { return semantic_; }

        void update_with(const AntigenSerum& src);

      private:
        virus::Name name_{};
        Annotations annotations_{};
        sequences::lineage_t lineage_{};
        virus::Passage passage_{};
        virus::Reassortant reassortant_{};
        sequences::sequence_aa_t aa_{};
        sequences::sequence_nuc_t nuc_{};
        sequences::insertions_t aa_insertions_{};
        sequences::insertions_t nuc_insertions_{};
        SemanticAttributes semantic_{};
    };

    inline auto compare_basic_designations(const AntigenSerum& a1, const AntigenSerum& a2)
    {
        return (a1.name() <=> a2.name()) | (a1.annotations() <=> a2.annotations()) | (a1.reassortant() <=> a2.reassortant());
    }

    // ----------------------------------------------------------------------

    class Antigen : public AntigenSerum
    {
      public:
        using index_t = antigen_index;
        using indexes_t = antigen_indexes;

        using AntigenSerum::AntigenSerum;
        bool operator==(const Antigen&) const = default;

        const auto& date() const { return date_; }
        void date(const Date& date) { date_ = date; }
        const auto& lab_ids() const { return lab_ids_; }
        LabIds& lab_ids() { return lab_ids_; }

        std::string designation() const { return string::join(" ", name(), string::join(" ", annotations()), reassortant(), passage()); }
        size_t designation_size() const { return string::join_size(1, name().size(), annotations().join_size(1), reassortant().size(), passage().size()); }

        static inline const char* ag_sr = "AG";

        void update_with(const Antigen& src);

      private:
        Date date_{};
        LabIds lab_ids_{};
    };

    // ----------------------------------------------------------------------

    class Serum : public AntigenSerum
    {
      public:
        using index_t = serum_index;
        using indexes_t = serum_indexes;

        using AntigenSerum::AntigenSerum;
        bool operator==(const Serum&) const = default;

        const auto& serum_species() const { return serum_species_; }
        void serum_species(const SerumSpecies& serum_species) { serum_species_ = serum_species; }
        const auto& serum_id() const { return serum_id_; }
        void serum_id(const SerumId& serum_id) { serum_id_ = serum_id; }
        // const auto& homologous_antigens() const { return homologous_antigens_; }
        // antigen_indexes& homologous_antigens() { return homologous_antigens_; }

        std::optional<double> forced_column_basis() const { return forced_column_basis_; }
        void forced_column_basis(double forced) { forced_column_basis_ = forced; }
        void not_forced_column_basis() { forced_column_basis_ = std::nullopt; }

        std::string designation() const { return string::join(" ", name(), string::join(" ", annotations()), reassortant(), serum_id()); }
        size_t designation_size() const { return string::join_size(1, name().size(), annotations().join_size(1), reassortant().size(), serum_id().size()); }

        static inline const char* ag_sr = "SR";

        void update_with(const Serum& src);

      private:
        SerumSpecies serum_species_{};
        SerumId serum_id_{};
        // antigen_indexes homologous_antigens_{};
        std::optional<double> forced_column_basis_{};
    };

    // ----------------------------------------------------------------------

    template <typename Element> class AntigensSera
    {
      public:
        using element_t = Element;
        using index_t = typename Element::index_t;
        using indexes_t = typename Element::indexes_t;

        AntigensSera() = default;
        AntigensSera(const AntigensSera&) = default;
        AntigensSera(AntigensSera&&) = default;
        AntigensSera& operator=(const AntigensSera&) = default;
        AntigensSera& operator=(AntigensSera&&) = default;
        bool operator==(const AntigensSera&) const = default;

        index_t size() const { return index_t{data_.size()}; }
        Element& operator[](index_t index) { return data_[*index]; }
        const Element& operator[](index_t index) const { return data_[*index]; }
        void resize(index_t sz) { data_.resize(*sz); }

        auto begin() const { return data_.begin(); }
        auto begin() { return data_.begin(); }
        auto end() const { return data_.end(); }
        auto end() { return data_.end(); }

        Element& add() { return data_.emplace_back(); }

        size_t max_designation() const
        {
            return std::accumulate(begin(), end(), 0ul, [](size_t mx, const Element& elt) { return std::max(mx, elt.designation_size()); });
        }

        // ----------------------------------------------------------------------

        using duplicates_t = named_vector_t<std::vector<index_t>, struct duplicates_tag>;

        duplicates_t find_duplicates() const
        {
            using map_value_t = std::vector<index_t>;
            std::unordered_map<std::string, map_value_t> designations_to_indexes;
            for (const auto index : size()) {
                const auto& ag{operator[](index)};
                if (!ag.annotations().distinct()) {
                    auto [pos, inserted] = designations_to_indexes.try_emplace(ag.designation(), map_value_t{});
                    pos->second.push_back(index);
                }
            }

            duplicates_t result;
            for (auto [designation, indexes] : designations_to_indexes) {
                if (indexes.size() > 1)
                    result.push_back(std::move(indexes));
            }
            return result;
        }

        void duplicates_distinct(const duplicates_t& dups)
        {
            for (const auto& entry : dups) {
                for (auto ip = std::next(entry.begin()); ip != entry.end(); ++ip)
                    operator[](*ip).annotations().set_distinct();
            }
        }

        // ----------------------------------------------------------------------

      private:
        std::vector<Element> data_{};
    };

    // ----------------------------------------------------------------------

    class Antigens : public AntigensSera<Antigen>
    {
      public:
        using AntigensSera<Antigen>::AntigensSera;
        bool operator==(const Antigens&) const = default;

        antigen_indexes homologous(const Serum& serum) const;

        // returns INCLUSIVE date range of all or test antigens
        std::pair<std::string_view, std::string_view> date_range(bool test_only, const indexes_t& reference_indexes) const;
    };

    // ----------------------------------------------------------------------

    class Sera : public AntigensSera<Serum>
    {
      public:
        using AntigensSera<Serum>::AntigensSera;
        bool operator==(const Sera&) const = default;
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v3::Antigen> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::chart::v3::Antigen& /*antigen*/, FormatCtx& ctx) const
        {
            format_to(ctx.out(), "AG");
            return ctx.out();
        }
};

template <> struct fmt::formatter<ae::chart::v3::Serum> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::chart::v3::Serum& /*serum*/, FormatCtx& ctx) const
        {
            format_to(ctx.out(), "SR");
            return ctx.out();
        }
};

// ----------------------------------------------------------------------
