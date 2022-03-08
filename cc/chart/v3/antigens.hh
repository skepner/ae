#pragma once

#include <optional>

#include "virus/name.hh"
#include "virus/name.hh"
#include "virus/passage.hh"
#include "virus/reassortant.hh"
#include "sequences/lineage.hh"
#include "sequences/sequence.hh"
#include "chart/v3/index.hh"
#include "chart/v3/annotations.hh"
#include "chart/v3/semantic.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Chart;

    class Date : public named_string_t<std::string, struct date_tag>
    {
      public:
        using named_string_t<std::string, struct date_tag>::named_string_t;

        bool within_range(std::string_view first_date, std::string_view after_last_date) const
        {
            return !empty() && (first_date.empty() || *this >= first_date) && (after_last_date.empty() || *this < after_last_date);
        }

        // void check() const;

    }; // class Date

    // ----------------------------------------------------------------------

    class LabIds : public named_vector_t<std::string, struct LabIds_tag>
    {
      public:
        using named_vector_t<std::string, struct LabIds_tag>::named_vector_t;

        std::string join() const { return fmt::format("{}", fmt::join(*this, " ")); }

    }; // class LabIds

    // ----------------------------------------------------------------------

    class SerumId : public named_string_t<std::string, struct SerumId_tag>
    {
      public:
        using named_string_t<std::string, struct SerumId_tag>::named_string_t;

    }; // class SerumId

    // ----------------------------------------------------------------------

    class SerumSpecies : public named_string_t<std::string, struct SerumSpecies_tag>
    {
      public:
        using named_string_t<std::string, struct SerumSpecies_tag>::named_string_t;

    }; // class SerumSpecies

    // ----------------------------------------------------------------------

    class AntigenSerum
    {
      public:
        AntigenSerum() = default;
        AntigenSerum(const AntigenSerum&) = default;
        AntigenSerum(AntigenSerum&&) = default;
        AntigenSerum& operator=(const AntigenSerum&) = default;
        AntigenSerum& operator=(AntigenSerum&&) = default;

        void name(const virus::Name& name) { name_ = name; }
        Annotations& annotations() { return annotations_; }
        void lineage(const sequences::lineage_t& lineage) { lineage_ = lineage; }
        void passage(const virus::Passage& passage) { passage_ = passage; }
        void reassortant(const virus::Reassortant& reassortant) { reassortant_ = reassortant; }
        void aa(const sequences::sequence_aa_t& aa) { aa_ = aa; }
        void nuc(const sequences::sequence_nuc_t& nuc) { nuc_ = nuc; }
        SemanticAttributes& semantic() { return semantic_; }

      private:
        virus::Name name_{};
        Annotations annotations_{};
        sequences::lineage_t lineage_{};
        virus::Passage passage_{};
        virus::Reassortant reassortant_{};
        sequences::sequence_aa_t aa_{};
        sequences::sequence_nuc_t nuc_{};
        SemanticAttributes semantic_{};
    };

    // ----------------------------------------------------------------------

    class Antigen : public AntigenSerum
    {
      public:
        using AntigenSerum::AntigenSerum;

        void date(const Date& date) { date_ = date; }
        LabIds& lab_ids() { return lab_ids_; }

      private:
        Date date_{};
        LabIds lab_ids_{};
    };

    // ----------------------------------------------------------------------

    class Serum : public AntigenSerum
    {
      public:
        using AntigenSerum::AntigenSerum;

        void serum_species(const SerumSpecies& serum_species) { serum_species_ = serum_species; }
        void serum_id(const SerumId& serum_id) { serum_id_ = serum_id; }
        antigen_indexes& homologous_antigens() { return homologous_antigens_; }

        std::optional<double> forced_column_basis() { return forced_column_basis_; }
        void forced_column_basis(double forced) { forced_column_basis_ = forced; }
        void not_forced_column_basis() { forced_column_basis_ = std::nullopt; }

      private:
        SerumSpecies serum_species_{};
        SerumId serum_id_{};
        antigen_indexes homologous_antigens_{};
        std::optional<double> forced_column_basis_{};
    };

    // ----------------------------------------------------------------------

    template <typename Index, typename Element> class AntigensSera
    {
      public:
        AntigensSera() = default;
        AntigensSera(const AntigensSera&) = default;
        AntigensSera(AntigensSera&&) = default;
        AntigensSera& operator=(const AntigensSera&) = default;
        AntigensSera& operator=(AntigensSera&&) = default;

        Index size() const { return Index{data_.size()}; }
        Element& operator[](Index index) { return data_[*index]; }
        const Element& operator[](Index index) const { return data_[*index]; }

        auto begin() const { return data_.begin(); }
        auto begin() { return data_.begin(); }
        auto end() const { return data_.end(); }
        auto end() { return data_.end(); }

        Element& add() { return data_.emplace_back(); }

      private:
        std::vector<Element> data_{};
    };

    // ----------------------------------------------------------------------

    class Antigens : public AntigensSera<antigen_index, Antigen>
    {
      public:
        using AntigensSera<antigen_index, Antigen>::AntigensSera;
    };

    // ----------------------------------------------------------------------

    class Sera : public AntigensSera<serum_index, Serum>
    {
      public:
        using AntigensSera<serum_index, Serum>::AntigensSera;
    };

}

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
