#pragma once

#pragma GCC diagnostic push

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wdisabled-macro-expansion"
#pragma GCC diagnostic ignored "-Wused-but-marked-unused"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wused-but-marked-unused"
#pragma GCC diagnostic ignored "-Wextra-semi-stmt"
#pragma GCC diagnostic ignored "-Wold-style-cast"
// M1
#pragma GCC diagnostic ignored "-Wreserved-identifier"
// #pragma GCC diagnostic ignored ""

#pragma GCC diagnostic ignored "-Wswitch-default"

#endif

#define XXH_INLINE_ALL

#include <xxhash.h>

namespace ae::xxhash
{
    inline auto xxhash32(std::string_view source) { return XXH32(source.data(), source.size(), 0); }

} // namespace ae::xxhash


#pragma GCC diagnostic pop

// ----------------------------------------------------------------------
