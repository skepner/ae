#pragma once

#include <string>

#include "ad/sfinae.hh"
#include "draw/v1/size.hh"
#include "ad/color-modifier.hh"
#include "utils/string.hh"

// ----------------------------------------------------------------------

namespace acmacs
{
    class FontSlant
    {
     public:
        enum Value { Normal, Italic };

        FontSlant(Value aFontSlant = Normal) noexcept : mFontSlant{aFontSlant} {}
        FontSlant(const FontSlant&) noexcept = default;
        FontSlant(std::string_view aFontSlant) { from(aFontSlant); }
        FontSlant(const char* aFontSlant) { from(aFontSlant); }
        FontSlant& operator=(const FontSlant&) noexcept = default;
        FontSlant& operator=(std::string_view aFontSlant) { from(aFontSlant); return *this; }

        constexpr operator Value() const noexcept { return mFontSlant; }
        constexpr Value get() const noexcept { return mFontSlant; }

     private:
        Value mFontSlant;

        void from(std::string_view aFontSlant)
        {
            using namespace std::string_view_literals;
            if (aFontSlant == "normal"sv)
                mFontSlant = Normal;
            else if (aFontSlant == "italic"sv)
                mFontSlant = Italic;
            else
                std::runtime_error{fmt::format("Unrecognized slant: {}", aFontSlant)};
        }

    }; // class FontSlant

// ----------------------------------------------------------------------

    class FontWeight
    {
     public:
        enum Value { Normal, Bold };

        FontWeight(Value aFontWeight = Normal) noexcept : mFontWeight{aFontWeight} {}
        FontWeight(const FontWeight&) noexcept = default;
        FontWeight(std::string_view aFontWeight) { from(aFontWeight); }
        FontWeight(const char* aFontWeight) { from(aFontWeight); }
        FontWeight& operator=(const FontWeight&) noexcept = default;
        FontWeight& operator=(std::string_view aFontWeight) { from(aFontWeight); return *this; }

        constexpr operator Value() const noexcept { return mFontWeight; }
        constexpr Value get() const noexcept { return mFontWeight; }

     private:
        Value mFontWeight;

        void from(std::string_view aFontWeight)
        {
            using namespace std::string_view_literals;
            if (aFontWeight == "normal"sv)
                mFontWeight = Normal;
            else if (aFontWeight == "bold"sv)
                mFontWeight = Bold;
            else
                std::runtime_error(fmt::format("Unrecognized font weight: \"{}\"", aFontWeight));
        }

    }; // class FontWeight

// ----------------------------------------------------------------------

    class TextStyle
    {
     public:
        TextStyle() = default;
        TextStyle(std::string_view font_name) : font_family{std::string{font_name}} {}

        [[nodiscard]] bool operator==(const TextStyle& ts) const
            {
                return slant == ts.slant && weight == ts.weight && font_family == ts.font_family;
            }

        FontSlant slant;
        FontWeight weight;
        std::string font_family;

    }; // class TextStyle

// ----------------------------------------------------------------------

    class LabelStyle
    {
     public:
        [[nodiscard]] bool operator==(const LabelStyle& ls) const
            {
                return shown == ls.shown && offset == ls.offset && size == ls.size && color == ls.color
                        && rotation == ls.rotation && float_equal(interline, ls.interline) && style == ls.style;
            }
        [[nodiscard]] bool operator!=(const LabelStyle& rhs) const { return !operator==(rhs); }

        bool shown{true};
        Offset offset{0, 1};
        ae::draw::v1::Pixels size{10.0};
        color::Modifier color{BLACK};
        ae::draw::v1::Rotation rotation{ae::draw::v1::NoRotation};
        double interline{0.2};
        TextStyle style;

    }; // class LabelStyle

} // namespace acmacs

// ----------------------------------------------------------------------

template <> struct fmt::formatter<acmacs::FontSlant> : fmt::formatter<ae::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const acmacs::FontSlant& slant, FormatCtx& ctx)
    {
        switch (slant.get()) {
            case acmacs::FontSlant::Normal:
                return format_to(ctx.out(), "normal");
            case acmacs::FontSlant::Italic:
                return format_to(ctx.out(), "italic");
        }
        return format_to(ctx.out(), "normal");
    }
};

template <> struct fmt::formatter<acmacs::FontWeight> : fmt::formatter<ae::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const acmacs::FontWeight& weight, FormatCtx& ctx)
    {
        switch (weight.get()) {
            case acmacs::FontWeight::Normal:
                return format_to(ctx.out(), "normal");
            case acmacs::FontWeight::Bold:
                return format_to(ctx.out(), "bold");
        }
        return format_to(ctx.out(), "normal");
    }
};

template <> struct fmt::formatter<acmacs::TextStyle> : fmt::formatter<ae::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const acmacs::TextStyle& style, FormatCtx& ctx) {
        return format_to(ctx.out(), R"({{"slant": {}, "weight": {}, "family": {}}})", style.slant, style.weight, style.font_family);
    }
};

template <> struct fmt::formatter<acmacs::LabelStyle> : fmt::formatter<ae::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const acmacs::LabelStyle& style, FormatCtx& ctx)
    {
        return format_to(ctx.out(), R"({{"shown": {}, "offset": {}, "size": {}, "color": {}, "rotation": {}, "interline": {:.2f}, "style": {}}})", style.shown, style.offset, style.size, style.color,
                         style.rotation, style.interline, style.style);
    }
};

// ----------------------------------------------------------------------
