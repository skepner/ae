#pragma once

#include <string>
#include <memory>
#include <filesystem>

// ======================================================================

namespace ae::tree
{
    class Tree;
    struct Inode;

    std::string export_text(const Tree& tree, const Inode& root);
    std::string export_json(const Tree& tree, const Inode& root);

    bool is_json(std::string_view data);
    std::shared_ptr<Tree> load_json(const std::string& data, const std::filesystem::path& filename);
    void load_join_json(const std::string& data, Tree& tree, Inode& join_at, const std::filesystem::path& filename);

} // namespace ae::tree

// ======================================================================
