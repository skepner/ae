#pragma once

#include "utils/string.hh"
#include "chart/v2/point-index-list.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v2
{
    namespace detail
    {
        inline void extend(PointIndexList& target, std::vector<size_t>&& source, size_t aIncrementEach, size_t aMax)
        {
            for (const auto no : source) {
                if (no >= aMax)
                    throw std::runtime_error(fmt::format("invalid index {}, expected in range 0..{} inclusive", no, aMax - 1));
                target.insert(no + aIncrementEach);
            }
        }
    } // namespace detail

    inline DisconnectedPoints get_disconnected(std::string_view antigens, std::string_view sera, size_t number_of_antigens, size_t number_of_sera)
    {
        DisconnectedPoints points;
        if (!antigens.empty())
            detail::extend(points, ae::string::split_into_size_t(antigens), 0, number_of_antigens + number_of_sera);
        if (!sera.empty())
            detail::extend(points, ae::string::split_into_size_t(sera), number_of_antigens, number_of_sera);
        return points;
    }

} // namespace ae::chart::v2

// ----------------------------------------------------------------------
