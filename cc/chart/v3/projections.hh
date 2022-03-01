#pragma once

#include <vector>

#include "chart/v3/index.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Chart;

    class Projection
    {
      public:
        Projection() = default;
        Projection(const Projection&) = delete;
        Projection(Projection&&) = default;
        Projection& operator=(const Projection&) = delete;
        Projection& operator=(Projection&&) = default;
    };

    // ----------------------------------------------------------------------

    class Projections
    {
      public:
        Projections() = default;
        Projections(const Projections&) = delete;
        Projections(Projections&&) = default;
        Projections& operator=(const Projections&) = delete;
        Projections& operator=(Projections&&) = default;
    };
}

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v3::Projection> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::chart::v3::Projection& projection, FormatCtx& ctx) const
        {
            format_to(ctx.out(), "PROJECTION");
            return ctx.out();
        }
};

// ----------------------------------------------------------------------
