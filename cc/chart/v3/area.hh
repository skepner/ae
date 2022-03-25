#pragma once

#include "chart/v3/point-coordinates.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    struct Area
    {
        point_coordinates min, max;

        Area(const point_coordinates& a_min, const point_coordinates& a_max) : min(a_min), max(a_max) {}
        Area(const point_coordinates& a_min) : min(a_min), max(a_min) {}

        number_of_dimensions_t num_dim() const { return min.number_of_dimensions(); }

        void extend(const point_coordinates& point)
        {
            for (number_of_dimensions_t dim{0}; dim < num_dim(); ++dim) {
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
          private:
            double step_;
            point_coordinates min_, max_, current_;

            friend struct Area;
            Iterator(double step, const point_coordinates& a_min, const point_coordinates& a_max) : step_(step), min_(a_min), max_(a_min), current_(a_min) { set_max(a_max); }
            Iterator() : step_(std::numeric_limits<double>::quiet_NaN()), min_{number_of_dimensions_t{2}}, max_{number_of_dimensions_t{2}}, current_{number_of_dimensions_t{2}} {}

            void set_max(const point_coordinates& a_max)
            {
                for (number_of_dimensions_t dim{0}; dim < a_max.number_of_dimensions(); ++dim)
                    max_[dim] = min_[dim] + std::ceil((a_max[dim] - min_[dim]) / step_) * step_;
            }

          public:
            bool operator==(const Iterator& rhs) const
            {
                if (std::isnan(rhs.step_))
                    return current_.x() > max_.x();
                else if (std::isnan(step_))
                    return rhs.current_.x() > rhs.max_.x();
                else
                    throw std::runtime_error("cannot compare Area::Iterators");
            }

            bool operator!=(const Iterator& rhs) const { return !operator==(rhs); }
            const point_coordinates& operator*() const { return current_; }
            const point_coordinates* operator->() const { return &current_; }

            const Iterator& operator++()
            {
                if (current_.x() <= max_.x()) {
                    for (number_of_dimensions_t dim{0}; dim < current_.number_of_dimensions(); ++dim) {
                        current_[dim] += step_;
                        if (current_[dim] <= max_[dim]) {
                            std::copy(min_.begin(), min_.begin() + static_cast<size_t>(dim), current_.begin());
                            break;
                        }
                    }
                }
                return *this;
            }
        };

        Iterator begin(double step) const { return Iterator(step, min, max); }
        Iterator end() const { return Iterator{}; }

    }; // struct Area

    // in case of no intersection returned Area has min and max points at {0, 0} and therefore its area is 0
    // Area intersection(const Area& a1, const Area& a2);

    class Layout;

    Area area(const Layout& layout);                                  // for all points
    Area area(const Layout& layout, const std::vector<size_t>& points); // just for the specified point indexes

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
