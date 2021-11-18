#include "tree/tree.hh"
#include "tree/newick.hh"

// ======================================================================

std::shared_ptr<ae::tree::Tree> ae::tree::load(const std::filesystem::path& filename)
{
    if (filename.filename().native().find(".newick") != std::string::npos)
        return load_newick(filename);
    else
        throw std::runtime_error{fmt::format("cannot load tree from \"{}\": unknown file format", filename)};

} // ae::tree::load

// ----------------------------------------------------------------------

void ae::tree::export_tree(const Tree& tree, const std::filesystem::path& filename)
{
    if (filename.filename().native().find(".newick") != std::string::npos)
        export_newick(tree, filename);
    else
        throw std::runtime_error{fmt::format("cannot export tree to \"{}\": unknown file format", filename)};

} // ae::tree::export_tree

// ----------------------------------------------------------------------
