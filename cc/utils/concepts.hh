#pragma once

#include <concepts>
#include <type_traits>

// ======================================================================

namespace ae
{
    template <typename T>
    concept lvalue_reference = std::is_lvalue_reference_v<T>;

    template <typename T>
    concept pointer = std::is_pointer_v<T>;

    template <typename T2, typename T>
    concept constructible_from = std::constructible_from<T, T2>;

    template <typename T2, typename T>
    concept assignable_from = std::assignable_from<T, T2>;

} // namespace ae

// ======================================================================
