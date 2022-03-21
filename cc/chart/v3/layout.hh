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

        auto iterator(point_index point_no) { return std::next(data_.begin(), static_cast<point_coordinates::ref_t::difference_type>(point_no.get() * number_of_dimensions_.get())); }
        auto iterator(point_index point_no) const { return std::next(data_.begin(), static_cast<point_coordinates::ref_t::difference_type>(point_no.get() * number_of_dimensions_.get())); }

        point_coordinates operator[](point_index point_no) { return point_coordinates::ref_t(&*iterator(point_no), number_of_dimensions_.get()); }
        point_coordinates at(point_index point_no) const { return const_cast<Layout*>(this)->operator[](point_no); }

        double operator()(point_index point_no, number_of_dimensions_t aDimensionNo) const { return data_[point_no.get() * number_of_dimensions_.get() + aDimensionNo.get()]; }
        double& operator()(point_index point_no, number_of_dimensions_t aDimensionNo) { return data_[point_no.get() * number_of_dimensions_.get() + aDimensionNo.get()]; }

        bool point_has_coordinates(point_index point_no) const { return at(point_no).exists(); }

        void update(point_index point_no, const point_coordinates& point)
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

        // void remove_points(const ReverseSortedIndexes& indexes, size_t base)
        // {
        //     for (const auto index : indexes) {
        //         const auto first = Vec::begin() + static_cast<difference_type>((index + base) * static_cast<size_t>(number_of_dimensions_));
        //         erase(first, first + static_cast<difference_type>(*number_of_dimensions_));
        //     }
        // }

        // void insert_point(size_t before, size_t base)
        // {
        //     insert(Vec::begin() + static_cast<difference_type>((before + base) * static_cast<size_t>(number_of_dimensions_)), *number_of_dimensions_, std::numeric_limits<double>::quiet_NaN());
        // }

        // size_t append_point()
        // {
        //     insert(Vec::end(), *number_of_dimensions_, std::numeric_limits<double>::quiet_NaN());
        //     return number_of_points() - 1;
        // }

        std::vector<std::pair<double, double>> minmax() const;

        double distance(point_index p1, point_index p2, double no_distance = point_coordinates::nan) const
        {
            if (const auto c1 = at(p1), c2 = at(p2); c1.exists() && c2.exists())
                return ae::chart::v3::distance(c1, c2);
            else
                return no_distance;
        }

        // returns indexes for min points for each dimension and max points for each dimension
        std::pair<std::vector<ae::point_index>, std::vector<ae::point_index>> min_max_point_indexes() const;
        // returns boundary coordinates (min and max)

        Layout transform(const Transformation& aTransformation) const;
        // point_coordinates centroid() const;

        // LayoutConstIterator begin() const { return {*this, 0}; }
        // LayoutConstIterator end() const { return {*this, number_of_points()}; }
        // LayoutConstIterator begin_antigens(size_t /*number_of_antigens*/) const { return {*this, 0}; }
        // LayoutConstIterator end_antigens(size_t number_of_antigens) const { return {*this, number_of_antigens}; }
        // LayoutConstIterator begin_sera(size_t number_of_antigens) const { return {*this, number_of_antigens}; }
        // LayoutConstIterator end_sera(size_t /*number_of_antigens*/) const { return {*this, number_of_points()}; }

        // LayoutDimensionConstIterator begin_dimension(number_of_dimensions_t dimension_no) const { return {*this, 0, dimension_no}; }
        // LayoutDimensionConstIterator end_dimension(number_of_dimensions_t dimension_no) const { return {*this, number_of_points(), dimension_no}; }
        // LayoutDimensionConstIterator begin_antigens_dimension(size_t /*number_of_antigens*/, number_of_dimensions_t dimension_no) const { return {*this, 0, dimension_no}; }
        // LayoutDimensionConstIterator end_antigens_dimension(size_t number_of_antigens, number_of_dimensions_t dimension_no) const { return {*this, number_of_antigens, dimension_no}; }
        // LayoutDimensionConstIterator begin_sera_dimension(size_t number_of_antigens, number_of_dimensions_t dimension_no) const { return {*this, number_of_antigens, dimension_no}; }
        // LayoutDimensionConstIterator end_sera_dimension(size_t /*number_of_antigens*/, number_of_dimensions_t dimension_no) const { return {*this, number_of_points(), dimension_no}; }

        // import from ace
        void number_of_dimensions(number_of_dimensions_t num_dim) { number_of_dimensions_ = num_dim; }
        void add_value(double value) { data_.push_back(value); }

        std::span<double> span() { return std::span(data_.data(), data_.size()); }
        std::span<const double> span() const { return std::span(data_.data(), data_.size()); }

      private:
        number_of_dimensions_t number_of_dimensions_{2};
        data_t data_{};
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
