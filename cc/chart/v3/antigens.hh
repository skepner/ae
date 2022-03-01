#pragma once

#include <vector>

#include "chart/v3/index.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Chart;

    class Antigen
    {
      public:
        Antigen() = default;
        Antigen(const Antigen&) = default;
        Antigen(Antigen&&) = default;
        Antigen& operator=(const Antigen&) = default;
        Antigen& operator=(Antigen&&) = default;
    };

    class Serum
    {
      public:
        Serum() = default;
        Serum(const Serum&) = default;
        Serum(Serum&&) = default;
        Serum& operator=(const Serum&) = default;
        Serum& operator=(Serum&&) = default;
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

      private:
        std::vector<Element> data_;
    };

    class Antigens : public AntigensSera<antigen_index, Antigen>
    {
      public:
        using AntigensSera<antigen_index, Antigen>::AntigensSera;
    };

    class Sera : public AntigensSera<serum_index, Serum>
    {
      public:
        using AntigensSera<serum_index, Serum>::AntigensSera;
    };

}

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v3::Antigen> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::chart::v3::Antigen& antigen, FormatCtx& ctx) const
        {
            format_to(ctx.out(), "AG");
            return ctx.out();
        }
};

template <> struct fmt::formatter<ae::chart::v3::Serum> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::chart::v3::Serum& serum, FormatCtx& ctx) const
        {
            format_to(ctx.out(), "SR");
            return ctx.out();
        }
};

// ----------------------------------------------------------------------
