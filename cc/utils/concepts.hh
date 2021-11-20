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

} // namespace ae

// ======================================================================
