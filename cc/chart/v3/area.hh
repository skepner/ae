#pragma once

#include "chart/v3/point-coordinates.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    struct Area
    {
        point_coordinates min, max;

        template <typename Storage> Area(const point_coordinates_with_storage<Storage>& a_min, const point_coordinates_with_storage<Storage>& a_max) : min{a_min}, max{a_max} {}
        template <typename Storage> Area(const point_coordinates_with_storage<Storage>& a_min) : min{a_min}, max{a_min} {}

        number_of_dimensions_t num_dim() const { return min.number_of_dimensions(); }

        void extend(const point_coordinates& point)
        {
            for (const auto dim : num_dim()) {
                min[dim] = std::min(point[dim], min[dim]);
                max[dim] = std::max(point[dim], max[dim]);
            }
        }

        double area() const
        {
            double result{1.0};
            for (number_of_dimensions_t dim{0}; dim < num_dim(); ++dim)
                result *= max[dim] - min[dim];
            return result;
        }

        class Iterator
        {
          public:
            const point_coordinates& operator*() const { return current_; }
            const point_coordinates* operator->() const { return &current_; }

            bool operator==(const Iterator& rhs) const
            {
                if (std::isnan(rhs.step_))
                    return std::isnan(current_[DIMX]);
                else if (std::isnan(step_))
                    return std::isnan(rhs.current_[DIMX]);
                else
                    throw std::runtime_error("cannot compare Area::Iterators");
            }

            const Iterator& operator++()
            {
                if (!std::isnan(current_[DIMX])) {
                    for (const auto dim : current_.number_of_dimensions()) {
                        current_[dim] += step_;
                        if (current_[dim] > max_[dim] && (dim + number_of_dimensions_t{1}) < current_.number_of_dimensions())
                            current_[dim] = min_[dim];
                        else
                            break;
                    }
                    if (current_.back() > max_.back())
                        current_.set_nan(); // end
                }
                return *this;
            }

          private:
            double step_;
            point_coordinates min_, max_, current_;

            friend struct Area;
            Iterator(double step, const point_coordinates& a_min, const point_coordinates& a_max) : step_{step}, min_{a_min}, max_{a_max}, current_{a_min} {}
        };

        Iterator begin(double step) const { return Iterator{step, min, max}; }
        Iterator end() const { return Iterator{point_coordinates::nan, point_coordinates{number_of_dimensions_t{2}}, point_coordinates{number_of_dimensions_t{2}}}; }

    }; // struct Area

    // in case of no intersection returned Area has min and max points at {0, 0} and therefore its area is 0
    // Area intersection(const Area& a1, const Area& a2);

    class Layout;

    Area area(const Layout& layout);                                    // for all points
    Area area(const Layout& layout, const std::vector<size_t>& points); // just for the specified point indexes

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v3::Area> : public fmt::formatter<ae::fmt_helper::float_formatter>
{
    template <typename FormatContext> auto format(const ae::chart::v3::Area& area, FormatContext& ctx) const
    {
        return format_to(ctx.out(), "Area{{area: {}, min: {}, max: {}}}", area.area(), area.min, area.max);
    }
};

// ----------------------------------------------------------------------
