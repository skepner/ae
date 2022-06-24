#pragma once

#include <cassert>
#include <span>

#include "chart/v3/point-coordinates.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Transformation;

    class Layout
    {
      public:
        using data_t = std::vector<double>;
        Layout() = default;

        Layout(const Layout&) = default;
        Layout(Layout&&) = default;
        Layout(point_index number_of_points, number_of_dimensions_t number_of_dimensions) : number_of_dimensions_{number_of_dimensions}, data_(number_of_points.get() * number_of_dimensions.get(), point_coordinates::nan) {}
        Layout(number_of_dimensions_t number_of_dimensions, const double* first, const double* last) : number_of_dimensions_{number_of_dimensions}, data_(first, last) {}
        Layout(const Layout& source, const point_indexes& indexes);

        Layout& operator=(const Layout&) = default;
        Layout& operator=(Layout&&) = default;

        point_index number_of_points() const noexcept { return point_index{data_.size() / number_of_dimensions_.get()}; }
        number_of_dimensions_t number_of_dimensions() const noexcept { return number_of_dimensions_; }

        void change_number_of_dimensions(number_of_dimensions_t num_dim, bool allow_dimensions_increase = false)
        {
            if (!allow_dimensions_increase && num_dim >= number_of_dimensions_)
                throw std::runtime_error{AD_FORMAT("Layout::change_number_of_dimensions: dimensions increase: {} --> {}", number_of_dimensions_, num_dim)};
            data_.resize(number_of_points().get() * num_dim.get());
            number_of_dimensions_ = num_dim;
        }

        auto iterator(point_index point_no) { return std::next(data_.begin(), static_cast<point_coordinates_ref::diff_t>(point_no.get() * number_of_dimensions_.get())); }
        auto iterator(point_index point_no) const { return std::next(data_.begin(), static_cast<point_coordinates_ref::diff_t>(point_no.get() * number_of_dimensions_.get())); }

        point_coordinates_ref operator[](point_index point_no) { return point_coordinates_ref(iterator(point_no), number_of_dimensions_); }
        point_coordinates_ref_const operator[](point_index point_no) const { return point_coordinates_ref_const(iterator(point_no), number_of_dimensions_); }
        point_coordinates_ref operator[](antigen_index antigen_no) { return operator[](to_point_index(antigen_no)); }
        point_coordinates_ref_const operator[](antigen_index antigen_no) const { return operator[](to_point_index(antigen_no)); }

        double operator()(point_index point_no, number_of_dimensions_t aDimensionNo) const { return data_[point_no.get() * number_of_dimensions_.get() + aDimensionNo.get()]; }
        double& operator()(point_index point_no, number_of_dimensions_t aDimensionNo) { return data_[point_no.get() * number_of_dimensions_.get() + aDimensionNo.get()]; }
        double operator()(antigen_index antigen_no, number_of_dimensions_t aDimensionNo) const { return operator()(to_point_index(antigen_no), aDimensionNo); }
        double& operator()(antigen_index antigen_no, number_of_dimensions_t aDimensionNo) { return operator()(to_point_index(antigen_no), aDimensionNo); }

        bool point_has_coordinates(point_index point_no) const { return operator[](point_no).exists(); }
        bool point_has_coordinates(antigen_index antigen_no) const { return point_has_coordinates(to_point_index(antigen_no)); }

        template <typename Storage> void update(point_index point_no, const point_coordinates_with_storage<Storage>& point)
        {
            assert(point_no < number_of_points());
            assert(point.number_of_dimensions() == number_of_dimensions());
            for (const auto dim : number_of_dimensions())
                operator()(point_no, dim) = point[dim];
        }

        void set_nan(point_index point_no)
        {
            std::for_each(iterator(point_no), iterator(point_no + point_index{1}), [](auto& target) { target = point_coordinates::nan; });
        }

        std::vector<std::pair<double, double>> minmax() const;

        double distance(point_index p1, point_index p2, double no_distance = point_coordinates::nan) const
        {
            if (const auto c1 = operator[](p1), c2 = operator[](p2); c1.exists() && c2.exists())
                return ae::chart::v3::distance(c1, c2);
            else
                return no_distance;
        }

        // returns indexes for min points for each dimension and max points for each dimension
        // std::pair<std::vector<ae::point_index>, std::vector<ae::point_index>> min_max_point_indexes() const;
        // returns boundary coordinates (min and max)

        Layout transform(const Transformation& aTransformation) const;
        // point_coordinates centroid() const;

        // import from ace
        void number_of_dimensions(number_of_dimensions_t num_dim) { number_of_dimensions_ = num_dim; }
        void add_value(double value) { data_.push_back(value); }

        std::span<double> span() { return std::span(data_.data(), data_.size()); }
        std::span<const double> span() const { return std::span(data_.data(), data_.size()); }

        void remove_points(const std::vector<size_t>& points_sorted_descending);

      private:
        number_of_dimensions_t number_of_dimensions_{2};
        data_t data_{};
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v3::Layout> : public fmt::formatter<ae::fmt_helper::float_formatter>
{
    template <typename FormatContext> auto format(const ae::chart::v3::Layout& layout, FormatContext& ctx)
    {
        format_to(ctx.out(), "[\n");
        for (const auto point_no : layout.number_of_points())
            format_to(ctx.out(), "    {}\n", layout[point_no]);
        return format_to(ctx.out(), "]");
    }
};

// ----------------------------------------------------------------------
