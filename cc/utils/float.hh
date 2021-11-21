#pragma once

#include <string>
#include <cmath>
#include <limits>
#include <type_traits>
#include <algorithm>
#include <stdexcept>
#include <concepts>

// ----------------------------------------------------------------------

// http://en.cppreference.com/w/cpp/types/numeric_limits/epsilon
template <std::floating_point T> constexpr inline bool float_equal(T x, T y, int ulp=1)
{
    // the machine epsilon has to be scaled to the magnitude of the values used
    // and multiplied by the desired precision in ULPs (units in the last place)
    return std::abs(x-y) < std::numeric_limits<T>::epsilon() * std::abs(x+y) * ulp
    // unless the result is subnormal
                           || std::abs(x-y) < std::numeric_limits<T>::min();
}

// ----------------------------------------------------------------------

template <std::floating_point T> constexpr inline bool float_equal_or_both_nan(T x, T y, int ulp=1)
{
    return float_equal(x, y, ulp) || (std::isnan(x) && std::isnan(y));
}

// ----------------------------------------------------------------------

template <std::floating_point T> constexpr inline bool float_zero(T x, int ulp=1)
{
    return float_equal(x, T(0), ulp);
}

// ----------------------------------------------------------------------

template <std::floating_point T> constexpr inline bool float_max(T x, int ulp=1)
{
    return float_equal(x, std::numeric_limits<T>::max(), ulp);
}

// ----------------------------------------------------------------------

template <std::floating_point T> constexpr inline T square(T v)
{
    return v * v;
}

// ======================================================================
