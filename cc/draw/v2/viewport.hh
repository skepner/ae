#pragma once

#include "ext/fmt.hh"
#include "utils/float.hh"

// ----------------------------------------------------------------------

namespace ae::draw::v2
{
    struct Viewport
    {
        Float x{0.0}, y{0.0}, width{0.0}, height{0.0};

        Viewport() = default;
        Viewport(double ax, double ay, double awidth): x{ax}, y{ay}, width{awidth}, height{awidth} {}
        Viewport(double ax, double ay, double awidth, double aheight): x{ax}, y{ay}, width{awidth}, height{aheight} {}
        bool operator==(const Viewport&) const = default;
    };
}

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::draw::v2::Viewport> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> constexpr auto format(const ae::draw::v2::Viewport& viewport, FormatCtx& ctx) const
    {
        return format_to(ctx.out(), "Viewport{{x:{}, y:{}, w:{}, h:{}}}", ae::format_double(viewport.x), ae::format_double(viewport.y), ae::format_double(viewport.width),
                         ae::format_double(viewport.height));
    }
};
