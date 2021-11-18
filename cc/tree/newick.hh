#pragma once

#include "tree/tree.hh"

// ======================================================================

namespace ae::tree
{
    std::shared_ptr<Tree> load_newick(const std::filesystem::path& filename);
    void export_newick(const Tree& tree, const std::filesystem::path& filename);

} // namespace ae::tree

// ======================================================================
