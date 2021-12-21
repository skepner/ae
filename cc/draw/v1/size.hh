#pragma once

#include <string>
#include <cassert>

#include "utils/log.hh"
#include "draw/v1/size-scale.hh"
#include "draw/v1/transformation.hh"

// ----------------------------------------------------------------------

namespace ae::draw::v1
{
    class Offset : public PointCoordinates
    {
      public:
        using PointCoordinates::PointCoordinates;

    }; // class Offset

// ----------------------------------------------------------------------

    class Size
    {
      public:
        double width = 0, height = 0;

        Size() = default;
        Size(double aWidth, double aHeight, const log::source_location& sl = {}) : width(aWidth), height(aHeight)
        {
            log::ad_assert(width >= 0 && height >= 0, sl, "Size{{{}, {}}} with negative width or/and height", width, height);
        }
        Size(const PointCoordinates& loc) : Size(loc.x(), loc.y()) {}
        Size(const PointCoordinates& a, const PointCoordinates& b) : Size(std::abs(a.x() - b.x()), std::abs(a.y() - b.y())) {}
        void set(double aWidth, double aHeight, const log::source_location& sl = {})
        {
            width = aWidth;
            height = aHeight;
            log::ad_assert(width >= 0 && height >= 0, sl, "Size{{{}, {}}} with negative width or/and height", width, height);
        }
        constexpr double aspect() const noexcept { return width / height; }
        constexpr bool empty() const noexcept { return float_zero(width) && float_zero(height); }

        [[nodiscard]] bool operator==(const Size& size) const { return float_equal(width, size.width) && float_equal(height, size.height); }
        [[nodiscard]] bool operator!=(const Size& size) const { return !operator==(size); }

        // std::string to_string() const { return "Size(" + std::to_string(width) + ", " + std::to_string(height) + ")"; }

        Size& operator+=(const Size& sz)
        {
            width += sz.width;
            height += sz.height;
            return *this;
        }
        Size& operator*=(double scale)
        {
            width *= scale;
            height *= scale;
            return *this;
        }

        PointCoordinates as_location() const { return PointCoordinates(width, height); }

    }; // class Size

    // inline std::string to_string(const Size& size) { return fmt::format("{{{}, {}}}", size.width, size.height); }

    inline Size operator-(const Size& a, const PointCoordinates& b) { return {a.width - b.x(), a.height - b.y()}; }
    inline Size operator-(const Size& a, const Size& b) { return {a.width - b.width, a.height - b.height}; }
    inline Size operator+(const Size& a, const Size& b) { return {a.width + b.width, a.height + b.height}; }
    inline Size operator*(const Size& a, double v) { return {a.width * v, a.height * v}; }
    inline Size operator/(const Size& a, double v) { return {a.width / v, a.height / v}; }

    inline PointCoordinates operator+(const PointCoordinates& a, const Size& b) { return PointCoordinates(a.x() + b.width, a.y() + b.height); }
    inline PointCoordinates operator-(const PointCoordinates& a, const Size& b) { return PointCoordinates(a.x() - b.width, a.y() - b.height); }

// ----------------------------------------------------------------------

    class Rectangle
    {
      public:
        Rectangle(double x1, double y1, double x2, double y2) : top_left{std::min(x1, x2), std::min(y1, y2)}, bottom_right{std::max(x1, x2), std::max(y1, y2)} {}
        Rectangle(const PointCoordinates& a, const PointCoordinates& b) : Rectangle(a.x(), a.y(), b.x(), b.y()) {}

        Rectangle transform(const Transformation& aTransformation) const
        {
            return {aTransformation.transform(top_left), aTransformation.transform(bottom_right)};
        }

        // returns if passed point is within the rectangle
        // constexpr bool within(double x, double y) const { return x >= top_left.x() && x <= bottom_right.x() && y >= top_left.y() && y <= bottom_right.y(); }
        bool within(const PointCoordinates& loc) const { return loc.x() >= top_left.x() && loc.x() <= bottom_right.x() && loc.y() >= top_left.y() && loc.y() <= bottom_right.y(); }
        PointCoordinates top_middle() const { return {(top_left.x() + bottom_right.x()) / 2.0, top_left.y()}; }
        PointCoordinates bottom_middle() const { return {(top_left.x() + bottom_right.x()) / 2.0, bottom_right.y()}; }
        PointCoordinates middle_left() const { return {top_left.x(), (top_left.y() + bottom_right.y()) / 2.0}; }
        PointCoordinates middle_right() const { return {bottom_right.x(), (top_left.y() + bottom_right.y()) / 2.0}; }

        PointCoordinates top_left;
        PointCoordinates bottom_right;

    }; // class Rectangle

// ----------------------------------------------------------------------

    class Circle
    {
      public:
          // Circle(double x, double y, double aRadius) : center{x, y}, radius{aRadius} {}
        Circle(const PointCoordinates& aCenter, double aRadius) : center{aCenter}, radius{aRadius} {}

        Circle transform(const Transformation& aTransformation) const
        {
            return {aTransformation.transform(center), radius};
        }

        // returns if passed point is within the circle
        // bool within(double x, double y) const { return distance(center, {x, y}) <= radius; }
        bool within(const PointCoordinates& loc) const { return distance(center, loc) <= radius; }

        PointCoordinates center;
        double radius;

    }; // class Circle

} // namespace ae::draw::v1

// ----------------------------------------------------------------------

// format for Size is format for double of each element, e.g. :.8f

template <> struct fmt::formatter<ae::draw::v1::Size> : public fmt::formatter<ae::fmt_helper::float_formatter>
{
    template <typename FormatContext> auto format(const ae::draw::v1::Size& size, FormatContext& ctx)
    {
        return format_to(ctx.out(), "[{}, {}]", format_val(size.width), format_val(size.height));
    }
};

// ----------------------------------------------------------------------
