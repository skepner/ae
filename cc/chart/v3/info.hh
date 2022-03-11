#pragma once

#include <vector>

#include "virus/virus.hh"
#include "virus/type-subtype.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Assay : public ae::named_string_t<std::string, struct assay_tag>
    {
      public:
        using ae::named_string_t<std::string, struct assay_tag>::named_string_t;
        enum class assay_name_t { full, brief, hi_or_neut, HI_or_Neut, no_hi, no_HI };
        enum class no_hi { no, yes };

        std::string name(assay_name_t an) const;
        std::string hi_or_neut(no_hi nh = no_hi::no) const;
        std::string HI_or_Neut(no_hi nh = no_hi::no) const;
        std::string short_name() const;
    };

    // ----------------------------------------------------------------------

    class RbcSpecies : public ae::named_string_t<std::string, struct rbc_species_tag>
    {
      public:
        using ae::named_string_t<std::string, struct rbc_species_tag>::named_string_t;
    };

    // ----------------------------------------------------------------------

    class TableDate : public ae::named_string_t<std::string, struct table_date_tag>
    {
      public:
        using ae::named_string_t<std::string, struct table_date_tag>::named_string_t;
    };

    // ----------------------------------------------------------------------

    class Lab : public ae::named_string_t<std::string, struct lab_tag>
    {
      public:
        using ae::named_string_t<std::string, struct lab_tag>::named_string_t;
    };

    // ----------------------------------------------------------------------

    class TableSource
    {
      public:
        TableSource() = default;
        TableSource(const TableSource&) = default;
        TableSource(TableSource&&) = default;
        TableSource& operator=(const TableSource&) = default;
        TableSource& operator=(TableSource&&) = default;

        std::string assay_rbc_short()
            {
                if (assay_.empty() || assay_ == Assay{"HI"} || !rbc_species_.empty())
                    return fmt::format("{} {}", assay_.short_name(), rbc_species_);
                else
                    return assay_.short_name();
            }

        const auto& virus() const { return virus_; }
        void virus(const ae::virus::virus_t& virus) { virus_ = virus; }
        const auto& type_subtype() const { return type_subtype_; }
        void type_subtype(const ae::virus::type_subtype_t& type_subtype) { type_subtype_ = type_subtype; }
        const auto& assay() const { return assay_; }
        void assay(const Assay& assay) { assay_ = assay; }
        const auto& rbc_species() const { return rbc_species_; }
        void rbc_species(const RbcSpecies& rbc_species) { rbc_species_ = rbc_species; }
        const auto& lab() const { return lab_; }
        void lab(const Lab& lab) { lab_ = lab; }
        const auto& date() const { return date_; }
        void date(const TableDate& date) { date_ = date; }
        const auto& name() const { return name_; }
        void name(const std::string& name) { name_ = name; }

      private:
        ae::virus::virus_t virus_{};
        ae::virus::type_subtype_t type_subtype_{};
        Assay assay_{};
        RbcSpecies rbc_species_{};
        Lab lab_{};
        TableDate date_{};
        std::string name_{}; // user supplied name
        // subset/lineage, e.g. "2009PDM"                                                                                                                                 |
    };

    // ----------------------------------------------------------------------

    class Info : public TableSource
    {
      public:
        using TableSource::TableSource;
        enum class include_number_of_tables { no, yes };

        auto& sources() { return sources_; }
        const auto& sources() const { return sources_; }

        // computed from sources if necessary
        std::string make_virus_not_influenza() const;
        std::string make_virus_type() const;
        std::string make_virus_subtype() const;
        std::string make_assay(Assay::assay_name_t tassay) const;
        std::string make_rbc_species() const;
        std::string make_lab() const;
        std::string make_date(include_number_of_tables inc = include_number_of_tables::yes) const;

      private:
        std::vector<TableSource> sources_{};
        // table type "A[NTIGENIC]" - default, "G[ENETIC]"                                                                                                                |
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
