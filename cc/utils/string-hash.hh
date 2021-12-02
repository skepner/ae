#pragma once

// https://www.cppstories.com/2021/heterogeneous-access-cpp20/

#include <functional>

#include "utils/named-type.hh"

// ----------------------------------------------------------------------

namespace ae
{
    struct string_hash_for_unordered_map
    {
        using is_transparent = void;
        [[nodiscard]] size_t operator()(const char *txt) const { return std::hash<std::string_view>{}(txt); }
        [[nodiscard]] size_t operator()(std::string_view txt) const { return std::hash<std::string_view>{}(txt); }
        [[nodiscard]] size_t operator()(const std::string& txt) const { return std::hash<std::string>{}(txt); }
        template <typename T, typename Tag> [[nodiscard]] size_t operator()(const named_string_t<T, Tag>& txt) const { return std::hash<T>{}(*txt); }
    };

} // namespace ae

// ======================================================================
