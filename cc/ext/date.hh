#pragma once

#include <chrono>

// ----------------------------------------------------------------------

namespace ae::date
{
    inline auto today() { return floor<std::chrono::days>(std::chrono::system_clock::now()); }
    inline auto today_year() { return static_cast<size_t>(static_cast<int>(std::chrono::year_month_day{today()}.year())); }
}

// ----------------------------------------------------------------------
