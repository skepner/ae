#pragma once

// https://www.cppstories.com/2021/heterogeneous-access-cpp20/

#include <string>
#include <string_view>
#include <functional>

// ----------------------------------------------------------------------

namespace ae
{
    struct string_hash_for_unordered_map
    {
        using is_transparent = void;
        [[nodiscard]] size_t operator()(const char *txt) const { return std::hash<std::string_view>{}(txt); }
        [[nodiscard]] size_t operator()(std::string_view txt) const { return std::hash<std::string_view>{}(txt); }
        [[nodiscard]] size_t operator()(const std::string &txt) const { return std::hash<std::string>{}(txt); }
    };

} // namespace ae

// ======================================================================
