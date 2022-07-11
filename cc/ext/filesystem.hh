#pragma once

#include <filesystem>

#include "ext/fmt.hh"

// ======================================================================

template <> struct fmt::formatter<std::filesystem::path> : fmt::formatter<ae::fmt_helper::default_formatter> {
    template <typename FormatCtx> constexpr auto format(const std::filesystem::path& path, FormatCtx& ctx) const
    {
        return format_to(ctx.out(), "{}", path.native());
    }
};

// ======================================================================
