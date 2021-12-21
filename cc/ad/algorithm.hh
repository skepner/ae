#pragma once

#include <algorithm>

// ----------------------------------------------------------------------

namespace acmacs
{
    template <typename Input, typename Output, typename Pred, typename Operation> inline Output transform_if(Input first, Input last, Output target, Pred predicate, Operation operation)
    {
        for (size_t index{0}; first != last; ++first, ++index) {
            if (predicate(*first)) {
                if constexpr (std::is_invocable_v<Operation, decltype(*first)>)
                    *target++ = operation(*first);
                else if constexpr (std::is_invocable_v<Operation, size_t, decltype(*first)>)
                    *target++ = operation(index, *first);
                else
                    static_assert(std::is_invocable_v<Operation, const decltype(*first)>, "acmacs::transform_if: unsupported Operation signature");
            }
        }
        return target;
    }

} // namespace acmacs


// ----------------------------------------------------------------------
