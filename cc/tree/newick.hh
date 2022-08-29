#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "tree/tree-iterator.hh"

// ======================================================================

namespace ae::tree
{
    inline bool is_newick(std::string_view data) { return data.size() > 5 && data[0] == '('; }
    std::shared_ptr<Tree> load_newick(const std::string& data);
    void load_join_newick(const std::string& data, Tree& tree, node_index_t join_at);
    std::string export_newick(const Tree& tree, const Inode& root);

} // namespace ae::tree

// ======================================================================
