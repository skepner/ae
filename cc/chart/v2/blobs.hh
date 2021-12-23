#pragma once

#include <cmath>
#include <vector>
#include <string>
#include <algorithm>

// ----------------------------------------------------------------------

#pragma GCC diagnostic push

#ifdef __GNUG__
#pragma GCC diagnostic ignored "-Wattributes" // g++-11 [[maybe_unused]]
#endif

namespace ae::chart::v2
{
    class Layout;
    class Stress;
    class PointIndexList;

    class Blobs
    {
      public:
        Blobs(double stress_diff, size_t number_of_drections = 36, double stress_diff_precision = 1e-5)
            : stress_diff_{stress_diff}, number_of_drections_{number_of_drections}, stress_diff_precision_{stress_diff_precision}, angle_step_{2.0 * M_PI / static_cast<double>(number_of_drections)} {}

        void calculate(const Layout& layout, const Stress& stress);
        void calculate(const Layout& layout, const PointIndexList& points, const Stress& stress);

        constexpr auto number_of_drections() const { return number_of_drections_; }
        constexpr auto angle_step() const { return angle_step_; }

        const std::vector<double>& data_for_point(size_t point_no) const
        {
            if (const auto found = std::find_if(std::begin(result_), std::end(result_), [point_no](const auto& entry) { return entry.first == point_no; }); found != std::end(result_))
                return found->second;
            throw std::runtime_error("Blobs::data_for_point: blob for point " + std::to_string(point_no) + " was not calculated");
        }

      private:
        [[maybe_unused]] const double stress_diff_{0.0};
        const size_t number_of_drections_{0};
        [[maybe_unused]] const double stress_diff_precision_{0.0};
        const double angle_step_{0.0};
        std::vector<std::pair<size_t, std::vector<double>>> result_{}; // point_no, blob_data

        void calculate_for_point(size_t point_no, Layout& layout, const Stress& stress, double initial_stress);

    }; // class Blobs

} // namespace ae::chart::v2

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------
