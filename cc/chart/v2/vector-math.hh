#pragma once

#include <numeric>

#include "utils/float.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v2::vector_math
{
    template <typename Iterator> inline double distance(Iterator first_1, Iterator last_1, Iterator first_2)
    {
        return std::sqrt(std::accumulate(first_1, last_1, 0.0, [first_2](double dist, double to_add_1) mutable { return dist + square(to_add_1 - *first_2++); }));

        // double dist = 0;
        // // #pragma clang loop vectorize(enable) interleave(enable)
        // for (; first_1 != last_1; ++first_1, ++first_2)
        //     dist += square(*first_1 - *first_2);
        // return std::sqrt(dist);
    }

    // template <typename Float, size_t NDim, typename Iterator = typename std::vector<Float>::const_iterator> struct distanceN
    // {
    //     static inline Float distance(Iterator first_1, Iterator first_2)
    //         {
    //             return std::sqrt(std::accumulate(first_1, first_1 + NDim, Float{0}, [first_2,square](Float dist, Float to_add_1) mutable { return dist + square(to_add_1 - *first_2++); }));
    //         }
    // };

    // template <typename Float, typename Iterator> struct distanceN<Float, 1, Iterator>
    // {
    //     static inline Float distance(Iterator first_1, Iterator first_2)
    //         {
    //             return std::abs(*first_1 - *first_2);
    //         }
    // };

    // template <typename Float, typename Iterator> struct distanceN<Float, 2, Iterator>
    // {
    //     static inline Float distance(Iterator first_1, Iterator first_2)
    //         {
    //             return std::hypot(*first_1 - *first_2, *(first_1 + 1) - *(first_2 + 1));
    //         }
    // };

    // template <typename Float, typename Iterator> struct distanceN<Float, 2, Iterator>
    // {
    //     static inline Float distance(Iterator first_1, Iterator first_2)
    //         {
    //             Float sum{0};
    //             for (size_t i = 0; i < 2; ++i, ++first_1, ++first_2)
    //                 sum += square(*first_1 - *first_2);
    //             return std::sqrt(sum);
    //         }
    // };

    // template <typename Float, typename Iterator> struct distanceN<Float, 3, Iterator>
    // {
    //     static inline Float distance(Iterator first_1, Iterator first_2)
    //         {
    //             return std::hypot(*first_1 - *first_2, *(first_1 + 1) - *(first_2 + 1), *(first_1 + 2) - *(first_2 + 2));
    //         }
    // };

// ----------------------------------------------------------------------

    // template <typename Float, typename Iterator> inline Float mean(Iterator first, Iterator last)
    // {
    //     return std::accumulate(first, last, Float{0}) / (last - first);
    // }

    // template <typename Float, typename Iterator> inline std::pair<Float, Float> mean_and_standard_deviation(Iterator first, Iterator last)
    // {
    //     const Float m = mean<Float>(first, last);
    //     const Float sum_of_squares = std::inner_product(first, last, first, Float{0}, std::plus<Float>(),
    //                                                     [m](Float xx, Float yy) { return (xx - m) * (yy - m); });
    //     return {m, std::sqrt(sum_of_squares / (last - first))};
    // }

    // template <typename Float, typename Iterator> inline Float inner_product(Iterator first_1, Iterator last_1, Iterator first_2, Float init = 0)
    // {
    //     return std::inner_product(first_1, last_1, first_2, init);
    // }

    // template <typename Float, typename Iterator> inline Float inner_product(Iterator first, Iterator last, Float init = 0)
    // {
    //     return inner_product(first, last, first, init);
    // }

    // template <typename Float, typename Iterator> inline Float eucledian_norm(Iterator first, Iterator last)
    // {
    //     return std::sqrt(inner_product(first, last, first, Float{0}));
    // }

} // namespace acmacs::vector_math

// ----------------------------------------------------------------------
