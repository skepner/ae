#pragma once

#include<utility>

// ======================================================================
// https://www.reddit.com/r/cpp_questions/comments/na7dke/is_there_any_standard_way_of_incrementing_an_enum/

template <typename E> requires(std::is_enum_v<E>) constexpr E& operator++(E& e) noexcept
{
    using underlying_t = std::underlying_type_t<E>;
    e = E(underlying_t(e)+underlying_t{1});
    return e;
}

// ======================================================================
