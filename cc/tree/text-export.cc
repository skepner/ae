#include "tree/tree.hh"
#include "tree/text-export.hh"

// ======================================================================

using prefix_t = std::vector<std::string>;
static void export_leaf(fmt::memory_buffer& text, const ae::tree::Leaf& leaf, double edge_step, prefix_t& prefix);
static void export_inode(fmt::memory_buffer& text, const ae::tree::Inode& inode, const ae::tree::Tree& tree, double edge_step, prefix_t& prefix);

// ======================================================================

std::string ae::tree::export_text(const Tree& tree)
{
    // tree.cumulative_calculate();
    // const double edge_step = 200.0 / tree.max_cumulative_shown().as_number();

    const double edge_step = 200.0 / 0.01;

    fmt::memory_buffer text;
    fmt::format_to(std::back_inserter(text), "-*- Tal-Text-Tree -*-\n");
    prefix_t prefix;
    export_inode(text, tree.root(), tree, edge_step, prefix);
    return fmt::to_string(text);

} // ae::tree::export_text

// ----------------------------------------------------------------------

void export_leaf(fmt::memory_buffer& text, const ae::tree::Leaf& leaf, double edge_step, prefix_t& prefix)
{
    using namespace fmt::literals;
    const auto edge_size = static_cast<size_t>(*leaf.edge * edge_step);
    fmt::format_to(std::back_inserter(text), "{prefix}{edge_symbol:{edge_size}s} \"{name}\" {aa_transitions}{accession_numbers} edge: {edge}  cumul: {cumul}  v:{vert}\n",
                   "prefix"_a = fmt::join(prefix, std::string_view{}),                                       //
                   "edge_symbol"_a = "-", "edge_size"_a = edge_size, "edge"_a = *leaf.edge, "cumul"_a = -1.0, //
                   "name"_a = leaf.name,                                                                     //
                   "accession_numbers"_a = "",                                                               // format_accession_numbers(node)),
                   "vert"_a = 0,                                                                             // node.node_id.vertical),
                   "aa_transitions"_a = ""                                                                   // , aa_transitions.empty() ? std::string{} : fmt::format("[{}] ", aa_transitions))
    );

} // export_leaf

// ----------------------------------------------------------------------

void export_inode(fmt::memory_buffer& text, const ae::tree::Inode& inode, const ae::tree::Tree& tree, double edge_step, prefix_t& prefix)
{
    using namespace fmt::literals;
    const auto edge_size = static_cast<size_t>(*inode.edge * edge_step);
    fmt::format_to(std::back_inserter(text), "{prefix}{edge_symbol:{edge_size}s}\\ >>>> leaves: {leaves}{aa_transitions} edge: {edge} cumul: {cumul}\n", //
                   "prefix"_a = fmt::join(prefix, std::string_view{}),                                                                                 //
                   "edge_symbol"_a = "=", "edge_size"_a = edge_size, "edge"_a = *inode.edge, "cumul"_a = -1.0,                                          //
                   "leaves"_a = -1,                                                                                                                    // node.number_leaves_in_subtree()),
                   // "node_id"_a = inode.node_id,
                   "aa_transitions"_a = "" // aa_transitions.empty() ? std::string{} : fmt::format(" [{}]", aa_transitions))
    );
    if (!prefix.empty()) {
        if (prefix.back().back() == '\\')
            prefix.back().back() = ' ';
        else if (prefix.back().back() == '+')
            prefix.back().back() = '|';
    }
    prefix.push_back(std::string(edge_size, ' ') + "+");
    for (auto child_it = inode.children.begin(); child_it != inode.children.end(); ++child_it) {
        const auto last_child = std::next(child_it) == inode.children.end();
        if (ae::tree::is_leaf(*child_it)) {
            if (const auto& leaf = tree.leaf(*child_it); leaf.shown) {
                if (last_child)
                    prefix.back().back() = '\\';
                export_leaf(text, leaf, edge_step, prefix);
            }
        }
        else {
            const auto& child_inode = tree.inode(*child_it);
            if (true /* child_inode.shown() */) {
                if (last_child)
                    prefix.back().back() = '\\';
                export_inode(text, child_inode, tree, edge_step, prefix);
            }
        }
    }
    prefix.pop_back();
    if (!prefix.empty() && prefix.back().back() == '|')
        prefix.back().back() = '+';

} // export_inode

// ----------------------------------------------------------------------
