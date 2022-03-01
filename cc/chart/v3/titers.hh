#pragma once

#include <vector>

#include "chart/v3/index.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Chart;

    class Titer
    {
      public:
        Titer() = default;
        Titer(const Titer&) = default;
        Titer(Titer&&) = default;
        Titer& operator=(const Titer&) = default;
        Titer& operator=(Titer&&) = default;
    };

    // ----------------------------------------------------------------------

    class Titers
    {
      public:
        Titers() = default;
        Titers(const Titers&) = default;
        Titers(Titers&&) = default;
        Titers& operator=(const Titers&) = default;
        Titers& operator=(Titers&&) = default;
    };
}

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v3::Titer> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::chart::v3::Titer& titer, FormatCtx& ctx) const
        {
            format_to(ctx.out(), "TITER");
            return ctx.out();
        }
};

// ----------------------------------------------------------------------
