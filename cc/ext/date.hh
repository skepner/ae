#pragma once

#include <chrono>

// ----------------------------------------------------------------------

namespace ae::date
{
    inline auto today() { return floor<std::chrono::days>(std::chrono::system_clock::now()); }
}

// ----------------------------------------------------------------------
