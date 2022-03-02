#pragma once

#include "ext/fmt.hh"
#include "ext/hash.hh"
#include "utils/named-type.hh"

// ----------------------------------------------------------------------

namespace ae
{
    using hash_t = ae::named_string_t<std::string, struct sequences_hash_tag>;

    inline hash_t hash(std::string_view source)
    {
        return hash_t{fmt::format("{:08X}", ae::xxhash::xxhash32(source))};
    }
}

// ----------------------------------------------------------------------
