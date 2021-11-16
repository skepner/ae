#pragma once

#include <string>
#include <string_view>
#include <compare>

// ======================================================================

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

} // namespace std

// ======================================================================
