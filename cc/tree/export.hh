#pragma once

#include <string>

// ======================================================================

namespace ae::tree
{
    class Tree;

    std::string export_text(const Tree& tree);
    std::string export_json(const Tree& tree);

} // namespace ae::tree

// ======================================================================
