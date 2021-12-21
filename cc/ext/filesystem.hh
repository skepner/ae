#pragma once

#include <filesystem>

#include "ext/fmt.hh"

// ======================================================================

template <> struct fmt::formatter<std::filesystem::path> : fmt::formatter<ae::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const std::filesystem::path& path, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "{}", path.native());
    }
};

// ======================================================================
