#pragma once

#include <string_view>

#include "ext/fmt.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class slant_t
    {
     public:
        enum Value { Normal, Italic };

        slant_t(Value aFontSlant = Normal) noexcept : slant_{aFontSlant} {}
        slant_t(const slant_t&) noexcept = default;
        slant_t(std::string_view aFontSlant) { from(aFontSlant); }
        slant_t& operator=(const slant_t&) noexcept = default;
        slant_t& operator=(std::string_view aFontSlant) { from(aFontSlant); return *this; }

        constexpr operator Value() const noexcept { return slant_; }
        constexpr Value get() const noexcept { return slant_; }

     private:
        Value slant_{Normal};

        void from(std::string_view aFontSlant)
        {
            using namespace std::string_view_literals;
            if (aFontSlant == "normal"sv)
                slant_ = Normal;
            else if (aFontSlant == "italic"sv)
                slant_ = Italic;
            else
                std::runtime_error{fmt::format("Unrecognized slant: {}", aFontSlant)};
        }

    }; // class slant_t

// ----------------------------------------------------------------------

    class weight_t
    {
     public:
        enum Value { Normal, Bold };

        weight_t(Value font_weight = Normal) noexcept : weight_{font_weight} {}
        weight_t(const weight_t&) noexcept = default;
        weight_t(std::string_view font_weight) { from(font_weight); }
        weight_t& operator=(const weight_t&) noexcept = default;
        weight_t& operator=(std::string_view font_weight) { from(font_weight); return *this; }

        constexpr operator Value() const noexcept { return weight_; }
        constexpr Value get() const noexcept { return weight_; }

     private:
        Value weight_{Normal};

        void from(std::string_view font_weight)
        {
            using namespace std::string_view_literals;
            if (font_weight == "normal"sv)
                weight_ = Normal;
            else if (font_weight == "bold"sv)
                weight_ = Bold;
            else
                std::runtime_error(fmt::format("Unrecognized font weight: \"{}\"", font_weight));
        }

    }; // class weight_t

    // ----------------------------------------------------------------------

    class text_style
    {
     public:
        text_style() = default;
        text_style(std::string_view font_name) : font_family{font_name} {}

        bool operator==(const text_style&) const = default;

        slant_t slant{};
        weight_t weight{};
        std::string font_family{};

    }; // class text_style

    // ----------------------------------------------------------------------

    class label_style
    {
      public:
        label_style() = default;

        bool operator==(const label_style&) const = default;

        bool shown{true};
        // ae::draw::v1::Offset offset{0, 1};
        // ae::draw::v1::Pixels size{10.0};
        // color::Modifier color{BLACK};
        // ae::draw::v1::Rotation rotation{ae::draw::v1::NoRotation};
        double interline{0.2};
        text_style style{};
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
