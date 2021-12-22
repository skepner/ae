#include <array>

#include "chart/v2/blobs.hh"
#include "chart/v2/layout.hh"
#include "chart/v2/stress.hh"
#include "chart/v2/point-index-list.hh"

// http://docs.rexamine.com/R-devel/contour-common_8h.html

// Suspended on 2019-02-25

// Possible implementation
// Move point in a grid and calculate stress or stress diff, the same
// as in hemi-grid-test.
// Infer contour(s) for the points within stress_diff and store them,
// perhaps as a vector of points in order of drawing the contour.

// ----------------------------------------------------------------------

void ae::chart::v2::Blobs::calculate(const Layout& layout, const Stress& stress)
{
    if (layout.number_of_dimensions() != number_of_dimensions_t{2})
        throw std::runtime_error("wrong number of dimensions for blobs");
    Layout layout_for_blobs{layout};
    const auto initial_stress = stress.value(layout_for_blobs);
    for (size_t point_no = 0; point_no < layout_for_blobs.number_of_points(); ++point_no) {
        calculate_for_point(point_no, layout_for_blobs, stress, initial_stress);
        layout_for_blobs.update(point_no, layout[point_no]);
    }

} // ae::chart::v2::Blobs::calculate

// ----------------------------------------------------------------------

void ae::chart::v2::Blobs::calculate(const Layout& layout, const PointIndexList& points, const Stress& stress)
{
    if (layout.number_of_dimensions() != number_of_dimensions_t{2})
        throw std::runtime_error("wrong number of dimensions for blobs");
    Layout layout_for_blobs{layout};
    const auto initial_stress = stress.value(layout_for_blobs);
    for (size_t point_no : points) {
        calculate_for_point(point_no, layout_for_blobs, stress, initial_stress);
        layout_for_blobs.update(point_no, layout[point_no]);
    }

} // ae::chart::v2::Blobs::calculate

// ----------------------------------------------------------------------

void ae::chart::v2::Blobs::calculate_for_point(size_t /*point_no*/, Layout& /*layout*/, const Stress& /*stress*/, double /*initial_stress*/)
{
} // ae::chart::v2::Blobs::calculate

// ----------------------------------------------------------------------

// Old (incorrect) algorithm and implementation

// void ae::chart::v2::Blobs::calculate_for_point(size_t point_no, acmacs::Layout& layout, const Stress& stress, double initial_stress)
// {
//     auto& result = result_.emplace_back(point_no, std::vector<double>(number_of_drections_, 0.0)).second;
//     const std::array saved{layout(point_no, 0), layout(point_no, 1)};
//     std::array radius{0.0, stress_diff_ / 10.0};
//     std::array stress_diff{0.0, 0.0};
//     for (size_t direction_no = 0; direction_no < number_of_drections_; ++direction_no) {
//         const auto angle_x = std::cos(angle_step_ * direction_no), angle_y = std::sin(angle_step_ * direction_no);
//         for (size_t step_no = 0; step_no < 100; ++step_no) {
//             layout.set(point_no, 0, saved[0] + angle_x * radius[1]);
//             layout.set(point_no, 1, saved[1] + angle_y * radius[1]);
//             stress_diff[1] = stress.value(layout) - initial_stress;
//             if (stress_diff[1] < stress_diff_) {
//                 const auto r0 = radius[0];
//                 radius[0] = radius[1];
//                 radius[1] = r0 + (radius[1] - r0) * (stress_diff_ - stress_diff[0]) / (stress_diff[1] - stress_diff[0]);
//                 stress_diff[0] = stress_diff[1];
//             }
//             else if ((stress_diff[1] - stress_diff_) < stress_diff_precision_) {
//                 break;
//             }
//             else {
//                 radius[1] = radius[0] + (radius[1] - radius[0]) * (stress_diff_ - stress_diff[0]) / (stress_diff[1] - stress_diff[0]);
//             }
//         }
//         result[direction_no] = radius[1];
//         // postprocess
//         radius[0] = 0.0;
//         stress_diff[0] = 0.0;
//     }

// } // ae::chart::v2::Blobs::calculate

// ----------------------------------------------------------------------
