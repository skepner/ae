#pragma once

#include "chart/v3/antigens.hh"
#include "chart/v3/titers.hh"
#include "chart/v3/projections.hh"
#include "chart/v3/styles.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Info
    {
      public:
        Info() = default;
        Info(const Info&) = delete;
        Info(Info&&) = default;
        Info& operator=(const Info&) = delete;
        Info& operator=(Info&&) = default;
    };

    // ----------------------------------------------------------------------

    class Chart
    {
      public:
        Chart() = default;
        Chart(const Chart&) = delete;
        Chart(Chart&&) = default;
        Chart& operator=(const Chart&) = delete;
        Chart& operator=(Chart&&) = default;

        const Info& info() const { return info_; }
        Info& info() { return info_; }

        Antigens& antigens() { return antigens_; }
        const Antigens& antigens() const { return antigens_; }
        Sera& sera() { return sera_; }
        const Sera& sera() const { return sera_; }
        Titers& titers() { return titers_; }
        const Titers& titers() const { return titers_; }
        Projections& projections() { return projections_; }
        const Projections& projections() const { return projections_; }
        Styles& styles() { return styles_; }
        const Styles& styles() const { return styles_; }

      private:
        Info info_;
        Antigens antigens_;
        Sera sera_;
        Titers titers_;
        Projections projections_;
        Styles styles_;
    };
}

// ----------------------------------------------------------------------
