#pragma once

#include "ext/fmt.hh"
#include "ext/hash.hh"

// ----------------------------------------------------------------------

namespace ae
{
    inline std::string hash(std::string_view source)
    {
        return fmt::format("{:08X}", ae::xxhash::xxhash32(source));
    }
}

// ----------------------------------------------------------------------
