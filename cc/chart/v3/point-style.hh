#pragma once

#include "chart/v3/point-shape.hh"
#include "chart/v3/label-style.hh"
#include "chart/v3/rotation.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class PointStyle
    {
      public:
        PointStyle() = default;
        PointStyle(const PointStyle&) = default;
        PointStyle(PointStyle&&) = default;
        PointStyle& operator=(const PointStyle&) = default;
        PointStyle& operator=(PointStyle&&) = default;

        bool operator==(const PointStyle& rhs) const noexcept = default;

        // PointStyle& scale(double aScale) noexcept { size(size() * aScale); return *this; }
        // PointStyle& scale_outline(double aScale) noexcept { outline_width(outline_width() * aScale); return *this; }

        // bool shown() const noexcept { return shown_; }
        // Color fill() const noexcept { return fill_; }
        // Color outline() const noexcept { return outline_; }
        // double outline_width() const noexcept { return outline_width_; }
        // double size() const noexcept { return size_; }
        // double diameter() const noexcept { return diameter_; } // drawi: use it if >0
        // Rotation rotation() const noexcept { return rotation_; }
        // Aspect aspect() const noexcept { return aspect_; }
        // point_shape shape() const noexcept { return shape_; }
        // const label_style& label() const noexcept { return label_; }
        // std::string_view label_text() const noexcept { return label_text_; }

        // void fill(Color a_fill) noexcept { fill_ = a_fill; }
        // void fill(const acmacs::color::Modifier& a_fill) noexcept { acmacs::color::modify(fill_, a_fill); }
        // void outline(Color a_outline) noexcept { outline_ = a_outline; }
        // void outline(const acmacs::color::Modifier& a_outline) noexcept { acmacs::color::modify(outline_, a_outline); }

        // void shown(bool a_shown) noexcept { shown_ = a_shown; }
        // void outline_width(double a_outline_width) noexcept { outline_width_ = a_outline_width; }
        // void size(double a_size) noexcept { size_ = a_size; }
        // void radius(double a_radius) noexcept { diameter_ = a_radius * 2.0; }
        // void rotation(Rotation a_rotation) noexcept { rotation_ = a_rotation; }
        // void aspect(Aspect a_aspect) noexcept { aspect_ = a_aspect; }
        // void shape(point_shape a_shape) noexcept { shape_ = a_shape; }
        // label_style& label() noexcept { return label_; }
        // void label_text(std::string_view a_label_text) noexcept { label_text_.assign(a_label_text); }

        // void fill(const acmacs::color::Modifier& a_fill) noexcept { fill_modifier_.add(a_fill); }
        // void outline(const acmacs::color::Modifier& a_outline) noexcept { outline_modifier_.add(a_outline); }

        // Color fill() const noexcept { auto fl = PointStyle::fill(); return acmacs::color::modify(fl, fill_modifier_); }
        // Color outline() const noexcept { auto outl = PointStyle::outline(); return acmacs::color::modify(outl, outline_modifier_); }

        // constexpr const auto& fill_modifier() const { return fill_modifier_; }
        // constexpr const auto& outline_modifier() const { return outline_modifier_; }

        // void shown(bool a_shown) noexcept { PointStyle::shown(a_shown); modified_shown_ = true; }
        // void outline_width(ae::draw::v1::Pixels a_outline_width) noexcept { PointStyle::outline_width(a_outline_width); modified_outline_width_ = true; }
        // void size(ae::draw::v1::Pixels a_size) noexcept { PointStyle::size(a_size); modified_size_ = true; }
        // void rotation(ae::draw::v1::Rotation a_rotation) noexcept { PointStyle::rotation(a_rotation); modified_rotation_ = true; }
        // void aspect(ae::draw::v1::Aspect a_aspect) noexcept { PointStyle::aspect(a_aspect); modified_aspect_ = true; }
        // void shape(point_shape a_shape) noexcept { PointStyle::shape(a_shape); modified_shape_ = true; }
        // label_style& label() noexcept { modified_label_ = true; return PointStyle::label(); }
        // void label_text(std::string_view a_label_text) noexcept { PointStyle::label_text(a_label_text); modified_label_text_ = true; }

      private:
        bool shown_{true};
        // Color fill_{TRANSPARENT};
        // Color outline_{BLACK};
        double outline_width_{1.0}; // pixels
        double size_{5.0};          // pixels
        Rotation rotation_{NoRotation};
        Aspect aspect_{AspectNormal};
        point_shape shape_{};
        label_style label_{};
        std::string label_text_{};

    }; // class PointStyle

}

// ----------------------------------------------------------------------

// template <> struct fmt::formatter<ae::chart::v3::PointStyle> : fmt::formatter<ae::fmt_helper::default_formatter>
// {
//     template <typename FormatCtx> auto format(const ae::chart::v3::PointStyle& style, FormatCtx& ctx)
//     {
//         return format_to(ctx.out(), R"({{"shape": {}, "shown": {}, "fill": "{}", "outline": "{}", "outline_width": {}, "size": {}, "aspect": {}, "rotation": {}, "label": {}, "label_text": "{}"}})",
//                          style.shape(), style.shown(), style.fill(), style.outline(), style.outline_width(), style.size(), style.aspect(), style.rotation(), style.label(), style.label_text());
//     }
// };

// ----------------------------------------------------------------------
