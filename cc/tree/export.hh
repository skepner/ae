#pragma once

#include <string>
#include <memory>
#include <filesystem>

// ======================================================================

namespace ae::tree
{
    class Tree;

    std::string export_text(const Tree& tree);
    std::string export_json(const Tree& tree);

    bool is_json(std::string_view data);
    std::shared_ptr<Tree> load_json(const std::string& data, const std::filesystem::path& filename);

} // namespace ae::tree

// ======================================================================
