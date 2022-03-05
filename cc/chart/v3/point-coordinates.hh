#pragma once

#include <limits>
#include <variant>
#include <vector>
#include <span>

#include "utils/float.hh"
#include "chart/v3/number-of-dimensions.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class point_coordinates
    {
      public:
        constexpr static const double nan = std::numeric_limits<double>::quiet_NaN();
        using store_t = std::vector<double>;
        using ref_t = std::span<double>;

        point_coordinates() = default;
        explicit point_coordinates(number_of_dimensions_t number_of_dimensions, double init = nan) : data_{store_t(*number_of_dimensions, init)} {}
        explicit point_coordinates(const std::span<double>& ref) : data_{ref} {}

      private:
        std::variant<store_t, ref_t> data_;
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
