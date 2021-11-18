#include "tree/newick.hh"

// ======================================================================

std::shared_ptr<ae::tree::Tree> ae::tree::load_newick(const std::string& data)
{
    fmt::print(">>>> load_newick\n");
    return std::make_shared<Tree>();

} // ae::tree::load_newick

// ----------------------------------------------------------------------

void ae::tree::export_newick(const Tree& tree, const std::filesystem::path& filename)
{

} // ae::tree::export_newick

// ----------------------------------------------------------------------
