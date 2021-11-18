#pragma once

#include <memory>

#include "ext/filesystem.hh"

// ======================================================================

namespace ae::tree
{
    class Tree
    {
      public:
      private:
    };

    std::shared_ptr<Tree> load(const std::filesystem::path& filename);
    void export_tree(const Tree& tree, const std::filesystem::path& filename);

} // namespace ae::tree

// ======================================================================
