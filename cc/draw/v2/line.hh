#pragma once

#include <cmath>

#include "chart/v3/point-coordinates.hh"

// ----------------------------------------------------------------------

namespace ae::draw::v2
{
    using point_coordinates = ae::chart::v3::point_coordinates;

    inline double sqr(double value) { return value * value; }

    class LineDefinedByEquation
    {
      public:
        LineDefinedByEquation() = default;
        LineDefinedByEquation(const LineDefinedByEquation&) = default;
        LineDefinedByEquation(double slope, double intercept) : slope_{slope}, intercept_{intercept} {}
        LineDefinedByEquation(const point_coordinates& p1, const point_coordinates& p2) : slope_{(p1[DIMY] - p2[DIMY]) / (p1[DIMX] - p2[DIMX])}, intercept_{p1[DIMY] - slope_ * p1[DIMX]} {}
        LineDefinedByEquation& operator=(const LineDefinedByEquation&) = default;

        double slope() const { return slope_; }
        double intercept() const { return intercept_; }

        // https://en.wikipedia.org/wiki/Distance_from_a_point_to_a_line
        double distance_with_direction(const point_coordinates& point) const { return (slope() * point[DIMX] - point[DIMY] + intercept()) / std::sqrt(a2b2()); }

        double distance_to(const point_coordinates& point) const { return std::abs(distance_with_direction(point)); }

        point_coordinates project_on(const point_coordinates& source) const
        {
            return {(source[DIMX] + slope() * source[DIMY] - slope() * intercept()) / a2b2(), (slope() * (source[DIMX] + slope() * source[DIMY]) + intercept()) / a2b2()};
        }

        point_coordinates flip_over(const point_coordinates& source, double scale = 1.0) const { return source + (project_on(source) - source) * (1.0 + scale); }

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
        point_coordinates fix(const point_coordinates& source) const
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

} // namespace ae::draw::v2

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::draw::v2::LineDefinedByEquation> : public fmt::formatter<ae::fmt_helper::float_formatter>
{
    auto format(const ae::draw::v2::LineDefinedByEquation& line, format_context& ctx) const
    {
        return fmt::format_to(ctx.out(), "Line(slope:{}, intercept:{})", format_val(line.slope()), format_val(line.intercept()));
    }
};

template <> struct fmt::formatter<ae::draw::v2::LineSide> : public fmt::formatter<ae::draw::v2::LineDefinedByEquation>
{
};

// ----------------------------------------------------------------------
