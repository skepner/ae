#include "tree/tree.hh"
#include "tree/text-export.hh"

// ======================================================================

std::string ae::tree::export_text(const Tree& tree)
{
    fmt::memory_buffer text;
    fmt::format_to(std::back_inserter(text), "-*- Tal-Text-Tree -*-\n");

    return fmt::to_string(text);

} // ae::tree::export_text

// ----------------------------------------------------------------------
