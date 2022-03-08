#pragma once

#include "draw/v2/label-style.hh"
#include "draw/v2/rotation.hh"
#include "chart/v3/point-shape.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    using Color = ae::draw::v2::Color;
    using Rotation = ae::draw::v2::Rotation;
    using Aspect = ae::draw::v2::Aspect;

    class PointStyle
    {
      public:
        PointStyle() = default;
        PointStyle(const PointStyle&) = default;
        PointStyle(PointStyle&&) = default;
        PointStyle& operator=(const PointStyle&) = default;
        PointStyle& operator=(PointStyle&&) = default;

        bool operator==(const PointStyle&) const noexcept = default;

        bool shown() const noexcept { return shown_; }
        void shown(bool a_shown) noexcept { shown_ = a_shown; }
        Color fill() const noexcept { return fill_; }
        void fill(Color a_fill) noexcept { fill_ = a_fill; }
        Color outline() const noexcept { return outline_; }
        void outline(Color a_outline) noexcept { outline_ = a_outline; }
        double outline_width() const noexcept { return outline_width_; }
        void outline_width(double a_outline_width) noexcept { outline_width_ = a_outline_width; }
        double size() const noexcept { return size_; }
        void size(double a_size) noexcept { size_ = a_size; }
        Rotation rotation() const noexcept { return rotation_; }
        void rotation(Rotation a_rotation) noexcept { rotation_ = a_rotation; }
        Aspect aspect() const noexcept { return aspect_; }
        void aspect(Aspect a_aspect) noexcept { aspect_ = a_aspect; }
        point_shape shape() const noexcept { return shape_; }
        void shape(point_shape a_shape) noexcept { shape_ = a_shape; }
        const ae::draw::v2::label_style& label() const noexcept { return label_; }
        ae::draw::v2::label_style& label() noexcept { return label_; }
        std::string_view label_text() const noexcept { return label_text_; }
        void label_text(std::string_view a_label_text) noexcept { label_text_ = a_label_text; }

      private:
        bool shown_{true};
        Color fill_{"transparent"};
        Color outline_{"black"};
        double outline_width_{1.0}; // pixels
        double size_{5.0};          // pixels
        Rotation rotation_{ae::draw::v2::NoRotation};
        Aspect aspect_{ae::draw::v2::AspectNormal};
        point_shape shape_{};
        ae::draw::v2::label_style label_{};
        std::string label_text_{};

    }; // class PointStyle

}

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v3::PointStyle> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::chart::v3::PointStyle& style, FormatCtx& ctx)
    {
        return format_to(ctx.out(), R"({{"shape": {}, "shown": {}, "fill": "{}", "outline": "{}", "outline_width": {}, "size": {}, "aspect": {}, "rotation": {}, "label": {}, "label_text": "{}"}})",
                         style.shape(), style.shown(), style.fill(), style.outline(), style.outline_width(), style.size(), style.aspect(), style.rotation(), style.label(), style.label_text());
    }
};

// ----------------------------------------------------------------------
