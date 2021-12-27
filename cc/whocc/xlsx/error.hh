#pragma once

#include <stdexcept>

// ----------------------------------------------------------------------

namespace ae::xlsx::inline v1
{
    struct Error : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

} // namespace ae::xlsx::inline v1

// ----------------------------------------------------------------------
