#include "chart/v2/layout.hh"

// ----------------------------------------------------------------------

ae::chart::v2::Area ae::chart::v2::intersection(const Area& a1, const Area& a2)
{
    const PointCoordinates new_min{std::max(a1.min.x(), a2.min.x()), std::max(a1.min.y(), a2.min.y())};
    const PointCoordinates new_max{std::min(a1.max.x(), a2.max.x()), std::min(a1.max.y(), a2.max.y())};
    if (new_min.x() > new_max.x() || new_min.y() > new_max.y())
        return {{0.0, 0.0}}; // no intersection
    else
        return {new_min, new_max};

} // ae::chart::v2::intersection

// ----------------------------------------------------------------------

std::pair<std::vector<size_t>, std::vector<size_t>> ae::chart::v2::Layout::min_max_point_indexes() const
{
    const auto num_dim = number_of_dimensions();
    std::vector<size_t> min_points(*num_dim, 0), max_points(*num_dim, 0);
    size_t point_no = 0;
    for (; !operator[](point_no).exists(); ++point_no); // skip NaN points at the beginning
    PointCoordinates min_coordinates(operator[](point_no));
    PointCoordinates max_coordinates(min_coordinates);
    ++point_no;
    for (; point_no < number_of_points(); ++point_no) {
        const auto point = operator[](point_no);
        if (point.exists()) {
            for (number_of_dimensions_t dim{0}; dim < num_dim; ++dim) {
                if (point[dim] < min_coordinates[dim]) {
                    min_coordinates[dim] = point[dim];
                    min_points[*dim] = point_no;
                }
                if (point[dim] > max_coordinates[dim]) {
                    max_coordinates[dim] = point[dim];
                    max_points[*dim] = point_no;
                }
            }
        }
    }
    return {min_points, max_points};

} // ae::chart::v2::Layout::min_max_point_indexes

// ----------------------------------------------------------------------

std::vector<std::vector<double>> ae::chart::v2::Layout::as_vector_of_vectors_double() const
{
    const size_t num_dim = *number_of_dimensions();
    std::vector<std::vector<double>> result(number_of_points(), std::vector<double>(num_dim));
    auto coord = Vec::begin();
    for (auto& dest : result) {
        std::copy(coord, coord + static_cast<difference_type>(num_dim), dest.begin());
        coord += static_cast<difference_type>(num_dim);
    }
    return result;

} // ae::chart::v2::Layout::as_vector_of_vectors_double

// ----------------------------------------------------------------------

ae::chart::v2::Area ae::chart::v2::Layout::area() const
{
    size_t point_no = 0;
    for (; !operator[](point_no).exists(); ++point_no); // skip NaN points at the beginning
    Area result(operator[](point_no));
    ++point_no;
    for (; point_no < number_of_points(); ++point_no) {
        if (const auto point = operator[](point_no); point.exists())
            result.extend(point);
    }
    return result;

} // ae::chart::v2::Layout::boundaries

// ----------------------------------------------------------------------

ae::chart::v2::Area ae::chart::v2::Layout::area(const std::vector<size_t>& points) const // just for the specified point indexes
{
    Area result(operator[](points.front()));
    for (auto point_no : points) {
        if (const auto point = operator[](point_no); point.exists())
            result.extend(point);
    }
    return result;

} // ae::chart::v2::Layout::boundaries

// ----------------------------------------------------------------------

std::shared_ptr<ae::chart::v2::Layout> ae::chart::v2::Layout::transform(const ae::draw::v1::Transformation& aTransformation) const
{
    auto result = std::make_shared<ae::chart::v2::Layout>(number_of_points(), number_of_dimensions());
    for (size_t p_no = 0; p_no < number_of_points(); ++p_no)
        result->update(p_no, aTransformation.transform(at(p_no)));
    return result;

} // ae::chart::v2::Layout::transform

// ----------------------------------------------------------------------

ae::chart::v2::PointCoordinates ae::chart::v2::Layout::centroid() const
{
    PointCoordinates result(static_cast<double>(*number_of_dimensions()), 0.0);
    size_t num_non_nan = number_of_points();
    for (size_t p_no = 0; p_no < number_of_points(); ++p_no) {
        if (const auto coord = at(p_no); coord.exists())
            result += coord;
        else
            --num_non_nan;
    }
    result /= static_cast<double>(num_non_nan);
    return result;

} // ae::chart::v2::Layout::centroid

// ----------------------------------------------------------------------

ae::chart::v2::Layout::Layout(const Layout& source, const std::vector<size_t>& indexes)
    : Vec(indexes.size() * source.number_of_dimensions().get(), std::numeric_limits<double>::quiet_NaN()), number_of_dimensions_{source.number_of_dimensions()}
{
    auto target = Vec::begin();
    for (auto index : indexes) {
        const auto coord{source[index]};
        std::copy(coord.begin(), coord.end(), target);
        target += static_cast<decltype(target)::difference_type>(*number_of_dimensions_);
    }

} // ae::chart::v2::Layout::Layout

// ----------------------------------------------------------------------

std::vector<std::pair<double, double>> ae::chart::v2::Layout::minmax() const
{
    std::vector<std::pair<double, double>> result(*number_of_dimensions_);
    // using diff_t = decltype(result)::difference_type;
    // Layout may contain NaNs (disconnected points), avoid them when finding minmax
    auto it = Vec::begin();
    while (it != Vec::end()) {
        number_of_dimensions_t valid_dims{0};
        for (number_of_dimensions_t dim{0}; dim < number_of_dimensions_; ++dim) {
            if (!std::isnan(*it)) {
                result[*dim] = std::pair(*it, *it);
                ++valid_dims;
            }
            ++it;
        }
                if (valid_dims == number_of_dimensions_)
                    break;
    }
    while (it != Vec::end()) {
                for (number_of_dimensions_t dim{0}; dim < number_of_dimensions_; ++dim) {
                    if (!std::isnan(*it)) {
                        result[*dim].first = std::min(result[*dim].first, *it);
                        result[*dim].second = std::max(result[*dim].second, *it);
                    }
                    ++it;
                    }
    }
    return result;

} // ae::chart::v2::Layout::minmax

// ----------------------------------------------------------------------
