#pragma once

#include <memory>
#include <string>
#include <string_view>

// ======================================================================

namespace ae::tree
{
    class Tree;

    inline bool is_newick(std::string_view data) { return data.size() > 5 && data[0] == '('; }
    std::shared_ptr<Tree> load_newick(const std::string& data);
    std::string export_newick(const Tree& tree);

} // namespace ae::tree

// ======================================================================
