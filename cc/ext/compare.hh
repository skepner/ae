#pragma once

#include <compare>

#include <vector>
#include <string>
#include <string_view>

// ----------------------------------------------------------------------

#ifndef __cpp_lib_three_way_comparison

namespace std
{
    inline auto operator<=>(std::string_view lh, std::string_view rh)
    {
        if (const auto res = lh.compare(rh); res < 0)
            return std::strong_ordering::less;
        else if (res > 0)
            return std::strong_ordering::greater;
        else
            return std::strong_ordering::equal;
    }

    inline auto operator<=>(const std::string& lh, const std::string& rh)
    {
        if (const auto res = lh.compare(rh); res < 0)
            return std::strong_ordering::less;
        else if (res > 0)
            return std::strong_ordering::greater;
        else
            return std::strong_ordering::equal;
    }

    // ----------------------------------------------------------------------

    // from g++ 11
    template <typename InputIter1, typename InputIter2, typename Comp>
    auto lexicographical_compare_three_way(InputIter1 first1, InputIter1 last1, InputIter2 first2, InputIter2 last2, Comp comp) -> decltype(comp(*first1, *first2))
    {
        while (first1 != last1) {
            if (first2 == last2)
                return strong_ordering::greater;
            if (auto cmp = comp(*first1, *first2); cmp != 0)
                return cmp;
            ++first1;
            ++first2;
        }
        return (first2 == last2) <=> true; // See PR 94006
    }

    // from g++ 11
    template <typename InputIter1, typename InputIter2> auto lexicographical_compare_three_way(InputIter1 first1, InputIter1 last1, InputIter2 first2, InputIter2 last2)
    {
        return lexicographical_compare_three_way(first1, last1, first2, last2, compare_three_way{});
    }

    // ----------------------------------------------------------------------

    // from g++ 11
    // template <typename Tp, typename Alloc> inline auto operator<=>(const vector<Tp, Alloc>& x, const vector<Tp, Alloc>& y)
    // {
    //     return std::lexicographical_compare_three_way(x.begin(), x.end(), y.begin(), y.end());
    // }

} // namespace std

#endif

// ----------------------------------------------------------------------

namespace std
{
    inline strong_ordering operator|(strong_ordering lhs, strong_ordering rhs)
    {
        if (lhs == strong_ordering::equal)
            return rhs;
        else
            return lhs;
    }
}

// ----------------------------------------------------------------------
