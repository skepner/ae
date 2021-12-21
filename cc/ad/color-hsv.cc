#include "ad/color-hsv.hh"
#include "utils/log.hh"

// ----------------------------------------------------------------------

acmacs::color::HSV::HSV(Color src)
{
    const double min = std::min({src.red(), src.green(), src.blue()});
    const double max = std::max({src.red(), src.green(), src.blue()});
    const double delta = max - min;

    v = max;
    if (delta < 0.00001) {
        s = 0;
        h = 0; // undefined, maybe nan?
    }
    else if (max > 0.0) {                           // NOTE: if Max is == 0, this divide would cause a crash
        s = delta / max;                            // s
        if (src.red() >= max) {                     // > is bogus, just keeps compilor happy
            h = (src.green() - src.blue()) / delta; // between yellow & magenta
        }
        else {
            if (src.green() >= max)
                h = 2.0 + (src.blue() - src.red()) / delta; // between cyan & yellow
            else
                h = 4.0 + (src.red() - src.green()) / delta; // between magenta & cyan
        }
        h *= 60.0; // degrees
        if (h < 0.0)
            h += 360.0;
    }
    else {
        // if max is 0, then r = g = b = 0
        // s = 0, v is undefined
        s = 0.0;
        h = 0.0; // NAN;                            // its now undefined
    }

} // acmacs::color::HSV::HSV

// ----------------------------------------------------------------------

// https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
uint32_t acmacs::color::HSV::rgb() const noexcept
{
    const auto from_rgb = [](double r, double g, double b) -> uint32_t { return (uint32_t(r * 255) << 16) | (uint32_t(g * 255) << 8) | uint32_t(b * 255); };

    if (s <= 0.0) { // < is bogus, just shuts up warnings
        return from_rgb(v, v, v);
    }
    else {
        const auto hh = (h > 360.0 ? 0.0 : h) / 60.0;
        const auto hhi = static_cast<long>(hh);
        const auto ff = hh - static_cast<double>(hhi);
        const double p = v * (1.0 - s);
        const double q = v * (1.0 - (s * ff));
        const double t = v * (1.0 - (s * (1.0 - ff)));

        switch (hhi) {
            case 0:
                return from_rgb(v, t, p);
            case 1:
                return from_rgb(q, v, p);
            case 2:
                return from_rgb(p, v, t);
            case 3:
                return from_rgb(p, q, v);
            case 4:
                return from_rgb(t, p, v);
            default:
                return from_rgb(v, p, q);
        }
    }

} // acmacs::color::HSV::to_rgb

// ----------------------------------------------------------------------
