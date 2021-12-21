#include "utils/log.hh"
#include "ext/from_chars.hh"
#include "ad/color-hsv.hh"
#include "ad/color-gradient.hh"
#include "utils/regex.hh"

#include "ad/color-names.icc"

// ----------------------------------------------------------------------

#pragma GCC diagnostic push

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

const std::regex re_heatmap{"heatmap\\[ *([0-9]+) *, *([0-9]+) *\\]", ae::regex::icase};

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

void Color::from_string(std::string_view src)
{
    try {
        if (src.empty())
            throw std::exception{};
        if (src[0] == '#') {
            const auto v = static_cast<uint32_t>(std::strtoul(src.data() + 1, nullptr, 16));
            switch (src.size()) {
                case 4: // web color #abc -> #aabbcc
                    color_ = ((v & 0xF00) << 12) | ((v & 0xF00) << 8) | ((v & 0x0F0) << 8) | ((v & 0x0F0) << 4) | ((v & 0x00F) << 4) | (v & 0x00F);
                    break;
                case 7:
                case 9:
                    color_ = v;
                    break;
                default:
                    throw std::exception{};
            }
        }
        else if (std::cmatch m1; std::regex_search(std::begin(src), std::end(src), m1, re_heatmap)) {
            *this = acmacs::color::perceptually_uniform_heatmap(ae::from_chars<size_t>(m1.str(2)), ae::from_chars<size_t>(m1.str(1)));
        }
        else {
            color_ = find_color_by_name(src); // color-names.icc, throws if not found
        }
    }
    catch (std::exception&) {
        // throw acmacs::color::error{fmt::format("Color::from_string: cannot read Color from \"{}\"", src)};
        AD_WARNING("Color::from_string: cannot read Color from \"{}\", PINK is substituted", src);
        color_ = 0xFFC0CB;
    }

} // Color::from_string

// ----------------------------------------------------------------------
