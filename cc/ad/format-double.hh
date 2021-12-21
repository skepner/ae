#pragma once

#include "ext/fmt.hh"

// ----------------------------------------------------------------------

namespace acmacs
{
    inline std::string format_double(double value)
    {
        if (const auto abs = std::abs(value); abs > 1e16 || abs < 1e-16)
            return fmt::format("{:.32g}", value);

        using namespace std::string_view_literals;
        const auto zeros{"000000000000"sv};
        const auto nines{"999999999999"sv};

        auto res = fmt::format("{:.17f}", value);
        if (const auto many_zeros = res.find(zeros); many_zeros != std::string::npos) {
            if (many_zeros > 0 && res[many_zeros - 1] == '.')
                return res.substr(0, many_zeros - 1); // remove trailing dot
            else
                return res.substr(0, many_zeros);
        }
        else if (const auto many_nines = res.find(nines); many_nines != std::string::npos && many_nines > 1) {
            const auto len = res[many_nines - 1] == '.' ? many_nines - 1 : many_nines;
            ++res[len - 1];
            return res.substr(0, len);
        }
        else
            return res;
    }
} // namespace acmacs

// ----------------------------------------------------------------------
