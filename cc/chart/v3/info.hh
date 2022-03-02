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
        enum class no_hi { no, yes };

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

        void virus(const ae::virus::virus_t& virus) { virus_ = virus; }
        void type_subtype(const ae::virus::type_subtype_t& type_subtype) { type_subtype_ = type_subtype; }
        void assay(const Assay& assay) { assay_ = assay; }
        void rbc_species(const RbcSpecies& rbc_species) { rbc_species_ = rbc_species; }
        void lab(const Lab& lab) { lab_ = lab; }
        void date(const TableDate& date) { date_ = date; }
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

        auto& sources() { return sources_; }
        const auto& sources() const { return sources_; }

      private:
        std::vector<TableSource> sources_{};
        // table type "A[NTIGENIC]" - default, "G[ENETIC]"                                                                                                                |
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
