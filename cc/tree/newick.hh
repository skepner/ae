#pragma once

#include "tree/tree.hh"

// ======================================================================

namespace ae::tree
{
    inline bool is_newick(std::string_view data) { return data.size() > 5 && data[0] == '('; }
    std::shared_ptr<Tree> load_newick(const std::string& data);
    void export_newick(const Tree& tree, const std::filesystem::path& filename);

} // namespace ae::tree

// ======================================================================
