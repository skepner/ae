#pragma once

#include <optional>

#include "draw/v2/label-style.hh"
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

        std::optional<bool> shown() const noexcept { return shown_; }
        void shown(bool a_shown) noexcept { shown_ = a_shown; }
        Color fill() const noexcept { return fill_; }
        void fill(Color a_fill) noexcept { fill_ = a_fill; }
        Color outline() const noexcept { return outline_; }
        void outline(Color a_outline) noexcept { outline_ = a_outline; }
        std::optional<double> outline_width() const noexcept { return outline_width_; }
        void outline_width(double a_outline_width) noexcept { outline_width_ = a_outline_width; }
        std::optional<double> size() const noexcept { return size_; }
        void size(double a_size) noexcept { size_ = a_size; }
        std::optional<Rotation> rotation() const noexcept { return rotation_; }
        void rotation(Rotation a_rotation) noexcept { rotation_ = a_rotation; }
        std::optional<Aspect> aspect() const noexcept { return aspect_; }
        void aspect(Aspect a_aspect) noexcept { aspect_ = a_aspect; }
        std::optional<point_shape> shape() const noexcept { return shape_; }
        void shape(point_shape a_shape) noexcept { shape_ = a_shape; }
        const ae::draw::v2::point_label& label() const noexcept { return label_; }
        ae::draw::v2::point_label& label() noexcept { return label_; }

      private:
        std::optional<bool> shown_{};
        Color fill_{};
        Color outline_{};
        std::optional<double> outline_width_{}; // pixels
        std::optional<double> size_{};          // pixels
        std::optional<Rotation> rotation_{};
        std::optional<Aspect> aspect_{};
        std::optional<point_shape> shape_{};
        ae::draw::v2::point_label label_{};

    }; // class PointStyle
} // namespace ae::chart::v3

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v3::PointStyle> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::chart::v3::PointStyle& style, FormatCtx& ctx)
    {
        using namespace std::string_view_literals;
        const auto out = [&ctx](std::string_view key, const auto& value, std::string_view format, bool comma) -> bool {
            if (value.has_value()) {
                format_to(ctx.out(), R"({}"{}": {})", comma ? ", " : "", key, fmt::format(format, *value));
                return true;
            }
            else
                return comma;
        };

        format_to(ctx.out(), "{{");
        auto comma = out("shape"sv, style.shape(), "{}", false);
        comma = out("shown"sv, style.shown(), "{}", comma);
        comma = out("fill"sv, style.fill(), "\"{}\"", comma);
        comma = out("outline"sv, style.outline(), "\"{}\"", comma);
        comma = out("outline_width"sv, style.outline_width(), "{}", comma);
        comma = out("size"sv, style.size(), "{}", comma);
        comma = out("aspect"sv, style.aspect(), "{}", comma);
        comma = out("rotation"sv, style.rotation(), "{}", comma);
        comma = out("label"sv, style.label(), "{}", comma);
        return format_to(ctx.out(), "}}");
    }
};

// ----------------------------------------------------------------------
