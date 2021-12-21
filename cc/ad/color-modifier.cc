#include <cctype>

#include "ad/color-modifier.hh"
#include "ad/color-hsv.hh"
#include "utils/float.hh"
#include "ext/from_chars.hh"
#include "utils/string.hh"

// ----------------------------------------------------------------------

// see ~/AD/share/doc/color.org
acmacs::color::Modifier::Modifier(std::string_view source)
{
    try {
        for (const auto& field : ae::string::split(source, ":", ae::string::split_emtpy::remove)) {
            if (field.size() < 2 || field[0] == '#' || !(field[1] == '=' || field[1] == '+' || field[1] == '-' || field[1] == '.' || std::isdigit(field[1]))) {
                applicators_.push_back(Color{field});
            }
            else {
                const auto get_value = [&field](size_t pos) -> double {
                    if (const auto value = ae::from_chars<double>(field.substr(pos)); float_max(value))
                        throw std::exception{};
                    else
                        return value;
                };

                switch (field[0]) {
                    case 'h':
                        if (field[1] == '=') {
                            if (const auto value = get_value(2); value >= 1.0 && value <= 360.0)
                                applicators_.push_back(hue_set{value});
                            else if (value >= 0.0 && value < 1.0)
                                applicators_.push_back(hue_set{value * 360.0});
                            else
                                throw std::exception{};
                        }
                        else if (const auto value = get_value(1); value >= -1.0 && value <= 1.0)
                            applicators_.push_back(hue_adjust{value});
                        else
                            throw std::exception{};
                        break;
                    case 's':
                        if (field[1] == '=') {
                            if (const auto value = get_value(2); value >= 0.0 && value <= 1.0)
                                applicators_.push_back(saturation_set{value});
                            else
                                throw std::exception{};
                        }
                        else if (const auto value = get_value(1); value >= -1.0 && value <= 1.0)
                            applicators_.push_back(saturation_adjust{value});
                        else
                            throw std::exception{};
                        break;
                    case 'b':
                        if (field[1] == '=') {
                            if (const auto value = get_value(2); value >= 0.0 && value <= 1.0)
                                applicators_.push_back(brightness_set{value});
                            else
                                throw std::exception{};
                        }
                        else if (const auto value = get_value(1); value >= -1.0 && value <= 1.0)
                            applicators_.push_back(brightness_adjust{value});
                        else
                            throw std::exception{};
                        break;
                    case 't':
                        if (field[1] == '=') {
                            if (const auto value = get_value(2); value >= 0.0 && value <= 1.0)
                                applicators_.push_back(transparency_set{value});
                            else
                                throw std::exception{};
                        }
                        else if (const auto value = get_value(1); value >= -1.0 && value <= 1.0)
                            applicators_.push_back(transparency_adjust{value});
                        else
                            throw std::exception{};
                        break;
                    case 'p':
                        if (field[1] == '=')
                            throw std::exception{};
                        else if (const auto value = get_value(1); value >= -1.0 && value <= 1.0) {
                            applicators_.push_back(saturation_adjust{-value});
                            applicators_.push_back(brightness_adjust{value});
                        }
                        else
                            throw std::exception{};
                        break;
                    default:
                        throw std::exception{};
                }
            }
        }
        // AD_DEBUG("acmacs::color::Modifier::Modifier -> {}", *this);
    }
    catch (std::exception&) {
        throw acmacs::color::error{fmt::format("cannot read color modifier from \"{}\"", source)};
    }

} // acmacs::color::Modifier::Modifier

// ----------------------------------------------------------------------

// returns applicators_.end() if not found
acmacs::color::Modifier::applicators_t::iterator acmacs::color::Modifier::find_last_color() noexcept
{
    if (const auto found = std::find_if(applicators_.rbegin(), applicators_.rend(), [](const auto& en) -> bool { return std::holds_alternative<Color>(en); }); found != applicators_.rend())
        return found.base();
    else
        return applicators_.end();

} // acmacs::color::Modifier::find_last_color

// ----------------------------------------------------------------------

// returns applicators_.end() if not found
acmacs::color::Modifier::applicators_t::const_iterator acmacs::color::Modifier::find_last_color() const noexcept
{
    if (const auto found = std::find_if(applicators_.rbegin(), applicators_.rend(), [](const auto& en) -> bool { return std::holds_alternative<Color>(en); }); found != applicators_.rend())
        return std::prev(found.base()); // .base() returns next to what found points to
    else
        return applicators_.end();

} // acmacs::color::Modifier::find_last_color

// ----------------------------------------------------------------------

acmacs::color::Modifier& acmacs::color::Modifier::add(const Modifier& rhs)
{
    // if there is Color entry in rhs, remove everything before it and everything in this before adding
    if (const auto last_color_rhs = rhs.find_last_color(); last_color_rhs != rhs.applicators().end()) {
        applicators_.clear();
        std::copy(last_color_rhs, std::end(rhs.applicators()), std::back_inserter(applicators_));
    }
    else {
        if (const auto last_color = find_last_color(); last_color != applicators().end())
            applicators_.erase(applicators_.begin(), last_color);
        std::copy(std::begin(rhs.applicators()), std::end(rhs.applicators()), std::back_inserter(applicators_));
    }
    // AD_DEBUG("color::Modifier::add -> {}", *this);
    return *this;

} // acmacs::color::Modifier::add

// ----------------------------------------------------------------------

namespace acmacs::color
{
    // [-1..0) - to yellow-red, (0..1] - to magenta-red, -1 and 1 - red, 0 - no change
    inline static void modify_color(Color& target, Modifier::hue_set hue)
    {
        HSV target_hsv{target};
        target_hsv.h = *hue;
        target = Color{target_hsv.rgb()}.alphaI(target.alphaI());
    }

    inline static void modify_color(Color& target, Modifier::hue_adjust hue)
    {
        HSV target_hsv{target};
        if (*hue < 0.0)
            target_hsv.h += target_hsv.h * *hue;
        else
            target_hsv.s += (360.0 - target_hsv.h) * *hue;
        target = Color{target_hsv.rgb()}.alphaI(target.alphaI());
    }

    // [-1..0) - desaturate (pale), (0..1] saturate, -1 - white, 1 - full saturation, 0 - no change
    inline static void modify_color(Color& target, Modifier::saturation_set saturation)
    {
        HSV target_hsv{target};
        // AD_DEBUG("saturation_set {}: {} -> {}", *saturation, target, target_hsv);
        target_hsv.s = *saturation;
        target = Color{target_hsv.rgb()}.alphaI(target.alphaI());
    }

    inline static void modify_color(Color& target, Modifier::saturation_adjust saturation)
    {
        HSV target_hsv{target};
        if (*saturation < 0.0)
            target_hsv.s += target_hsv.s * *saturation;
        else
            target_hsv.s += (1.0 - target_hsv.s) * *saturation;
        target = Color{target_hsv.rgb()}.alphaI(target.alphaI());
    }

    inline static void modify_color(Color& target, Modifier::brightness_set brightness)
    {
        HSV target_hsv{target};
        target_hsv.v = *brightness;
        target = Color{target_hsv.rgb()}.alphaI(target.alphaI());
    }

    inline static void modify_color(Color& target, Modifier::brightness_adjust brightness)
    {
        HSV target_hsv{target};
        if (*brightness < 0.0)
            target_hsv.v += target_hsv.v * *brightness;
        else
            target_hsv.v += (1.0 - target_hsv.v) * *brightness;
        target = Color{target_hsv.rgb()}.alphaI(target.alphaI());
    }

    inline static void modify_color(Color& target, Modifier::transparency_set transparency)
    {
        target.transparency(*transparency);
    }

    inline static void modify_color(Color& target, Modifier::transparency_adjust transparency)
    {
        const auto old_transparency = target.transparency();
        if (*transparency < 0.0)
            target.transparency(old_transparency + old_transparency * *transparency);
        else
            target.transparency(old_transparency + (1.0 - old_transparency) * *transparency);
    }

    inline static void modify_color(Color& target, Color color)
    {
        target = color;
    }

}

Color& acmacs::color::modify(Color& target, const Modifier& modifier)
{
    for (const auto& app : modifier.applicators())
        std::visit([&target](const auto& app_value) { modify_color(target, app_value); }, app);
    // AD_DEBUG("acmacs::color::modify {} -> {}", modifier, target);
    return target;

} // acmacs::color::adjust

// ----------------------------------------------------------------------

acmacs::color::Modifier::operator Color() const
{
    if (auto app = find_last_color(); app != applicators().end()) {
        Color result{TRANSPARENT};
        for (; app != applicators().end(); ++app)
            std::visit([&result](const auto& app_value) { modify_color(result, app_value); }, *app);
        // AD_DEBUG("color::Modifier::operator Color -> {}", result);
        return result;
    }
    else
        throw error{fmt::format("cannot convert acmacs::color::Modifier to Color: \"{}\"", *this)};

} // acmacs::color::Modifier::operator Color

// ----------------------------------------------------------------------
