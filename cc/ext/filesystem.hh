#pragma once

#include <filesystem>

#include "ext/fmt.hh"

// ======================================================================

template <> struct fmt::formatter<std::filesystem::path> : fmt::formatter<ae::fmt_helper::default_formatter> {
  auto format(const std::filesystem::path &path, format_context &ctx) const {
      return fmt::format_to(ctx.out(), "{}", path.native());
  }
};

// ======================================================================
