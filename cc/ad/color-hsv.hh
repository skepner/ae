#pragma once

#include "ad/color.hh"

// ----------------------------------------------------------------------

namespace acmacs::color
{
    struct HSV
    {
        HSV(Color src);

        uint32_t rgb() const noexcept;

        constexpr HSV& light(double value) noexcept
        {
            s /= value;
            return *this;
        }

        HSV& adjust_saturation(double value) noexcept
        {
            s *= std::abs(value);
            if (s > 1.0)
                s = 1.0;
            return *this;
        }
        HSV& adjust_brightness(double value) noexcept
        {
            v *= std::abs(value);
            if (v > 1.0)
                v = 1.0;
            return *this;
        }

        double h{0.0}; // angle in degrees 0-360
        double s{0.0}; // percent
        double v{0.0}; // percent
    };

} // namespace acmacs::color

template <> struct fmt::formatter<acmacs::color::HSV> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatContext> auto format(const acmacs::color::HSV& hsv, FormatContext& ctx) const
    {
        return format_to(ctx.out(), "hsv({:.0f}, {:.2f}, {:.2f})", hsv.h, hsv.s, hsv.v);
    }
};

// ----------------------------------------------------------------------
