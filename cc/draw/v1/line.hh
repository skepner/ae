#pragma once

#include <cmath>

#include "draw/v1/point-coordinates.hh"

// ----------------------------------------------------------------------

namespace ae::draw::v1
{
    inline double sqr(double value) { return value * value; }

    class LineDefinedByEquation
    {
      public:
        LineDefinedByEquation() = default;
        LineDefinedByEquation(const LineDefinedByEquation&) = default;
        LineDefinedByEquation(double slope, double intercept) : slope_{slope}, intercept_{intercept} {}
        LineDefinedByEquation(const PointCoordinates& p1, const PointCoordinates& p2) : slope_{(p1.y() - p2.y()) / (p1.x() - p2.x())}, intercept_{p1.y() - slope_ * p1.x()} {}
        LineDefinedByEquation& operator=(const LineDefinedByEquation&) = default;

        double slope() const { return slope_; }
        double intercept() const { return intercept_; }

        // https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
        double distance_with_direction(const PointCoordinates& point) const { return (slope() * point.x() - point.y() + intercept()) / std::sqrt(a2b2()); }

        double distance_to(const PointCoordinates& point) const { return std::abs(distance_with_direction(point)); }

        PointCoordinates project_on(const PointCoordinates& source) const
        {
            return {(source.x() + slope() * source.y() - slope() * intercept()) / a2b2(), (slope() * (source.x() + slope() * source.y()) + intercept()) / a2b2()};
        }

        PointCoordinates flip_over(const PointCoordinates& source, double scale = 1.0) const { return source + (project_on(source) - source) * (1.0 + scale); }

      private:
        double slope_{1.0};
        double intercept_{0.0};

        double a2b2() const { return sqr(slope()) + 1; }

    }; // class LineDefinedByEquation

    // ----------------------------------------------------------------------

    class LineSide : public LineDefinedByEquation
    {
      public:
        enum class side { negative, positive };

        LineSide() = default;
        LineSide(double slope, double intercept, side a_side) : LineDefinedByEquation(slope, intercept), side_{a_side} {}
        LineSide(const LineDefinedByEquation& line, side a_side) : LineDefinedByEquation(line), side_{a_side} {}

        // if passed point is one the correct side, leaves it as is,
        // otherwise flips it to the correct side
        PointCoordinates fix(const PointCoordinates& source) const
        {
            if (const auto dist = distance_with_direction(source); (dist * side_sign()) < 0)
                return flip_over(source);
            else
                return source;
        }

      private:
        side side_ = side::positive;

        double side_sign() const { return side_ == side::negative ? -1.0 : 1.0; }

    }; // class LineSide

} // namespace ae::draw::v1

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::draw::v1::LineDefinedByEquation> : public fmt::formatter<ae::fmt_helper::float_formatter>
{
    template <typename FormatContext> auto format(const ae::draw::v1::LineDefinedByEquation& line, FormatContext& ctx)
    {
        return format_to(ctx.out(), "Line(slope:{}, intercept:{})", format_val(line.slope()), format_val(line.intercept()));
    }
};

template <> struct fmt::formatter<ae::draw::v1::LineSide> : public fmt::formatter<ae::draw::v1::LineDefinedByEquation>
{
};

// ----------------------------------------------------------------------
