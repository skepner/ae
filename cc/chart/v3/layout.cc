#include "chart/v3/layout.hh"
#include "chart/v3/transformation.hh"

// ----------------------------------------------------------------------

std::vector<std::pair<double, double>> ae::chart::v3::Layout::minmax() const
{
    std::vector<std::pair<double, double>> result(*number_of_dimensions_);
    // using diff_t = decltype(result)::difference_type;
    // Layout may contain NaNs (disconnected points), avoid them when finding minmax
    auto it = data_.begin();
    while (it != data_.end()) {
        number_of_dimensions_t valid_dims{0};
        for (const auto dim : number_of_dimensions_) {
            if (!std::isnan(*it)) {
                result[*dim] = std::pair(*it, *it);
                ++valid_dims;
            }
            ++it;
        }
        if (valid_dims == number_of_dimensions_)
            break;
    }
    while (it != data_.end()) {
        for (const auto dim : number_of_dimensions_) {
            if (!std::isnan(*it)) {
                result[*dim].first = std::min(result[*dim].first, *it);
                result[*dim].second = std::max(result[*dim].second, *it);
            }
            ++it;
        }
    }
    return result;

} // ae::chart::v3::Layout::minmax

// ----------------------------------------------------------------------

// std::pair<std::vector<ae::point_index>, std::vector<ae::point_index>> ae::chart::v3::Layout::min_max_point_indexes() const
// {
//     const auto num_dim = number_of_dimensions();
//     std::vector<point_index> min_points(*num_dim, point_index{0}), max_points(*num_dim, point_index{0});
//     point_index point_no{0};
//     for (; !point_has_coordinates(point_no); ++point_no); // skip NaN points at the beginning
//     auto min_coordinates(operator[](point_no));
//     auto max_coordinates(min_coordinates);
//     ++point_no;
//     for (; point_no < number_of_points(); ++point_no) {
//         if (const auto point = operator[](point_no); point.exists()) {
//             for (const auto dim : num_dim) {
//                 if (point[dim] < min_coordinates[dim]) {
//                     min_coordinates[dim] = point[dim];
//                     min_points[*dim] = point_no;
//                 }
//                 if (point[dim] > max_coordinates[dim]) {
//                     max_coordinates[dim] = point[dim];
//                     max_points[*dim] = point_no;
//                 }
//             }
//         }
//     }
//     return {min_points, max_points};


// } // ae::chart::v3::Layout::min_max_point_indexes

// ----------------------------------------------------------------------

ae::chart::v3::Layout ae::chart::v3::Layout::transform(const Transformation& aTransformation) const
{
    Layout result{number_of_points(), number_of_dimensions()};
    for (const auto p_no : number_of_points())
        result.update(p_no, aTransformation.transform(operator[](p_no)));
    return result;

} // ae::chart::v3::Layout::transform

// ----------------------------------------------------------------------

void ae::chart::v3::Layout::remove_points(const std::vector<size_t>& points_sorted_descending)
{
    for (const auto ind : points_sorted_descending)
        data_.erase(std::next(data_.begin(), ind * number_of_dimensions_.get()), std::next(data_.begin(), (ind + 1) * number_of_dimensions_.get()));

} // ae::chart::v3::Layout::remove_points

// ----------------------------------------------------------------------
