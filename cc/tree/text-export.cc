#include "tree/tree.hh"
#include "tree/text-export.hh"

// ======================================================================

using prefix_t = std::vector<std::string>;
static void export_leaf(fmt::memory_buffer& text, const ae::tree::Leaf& leaf, double edge_step, prefix_t& prefix);
static void export_inode(fmt::memory_buffer& text, const ae::tree::Inode& inode, const ae::tree::Tree& tree, double edge_step, prefix_t& prefix);

// ======================================================================

std::string ae::tree::export_text(const Tree& tree)
{
    const double edge_step = 200.0 / *const_cast<Tree&>(tree).calculate_cumulative();

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
    const std::string edge_symbol(static_cast<size_t>(*leaf.edge * edge_step), '-');
    fmt::format_to(std::back_inserter(text), "{prefix}{edge_symbol} \"{name}\" <{node_id}>{aa_transitions}{accession_numbers} edge: {edge}  cumul: {cumul}  v:{vert}\n",
                   "prefix"_a = fmt::join(prefix, std::string_view{}),                                      //
                   "edge_symbol"_a = edge_symbol, "edge"_a = *leaf.edge, "cumul"_a = *leaf.cumulative_edge, //
                   "name"_a = leaf.name,                                                                    //
                   "node_id"_a = leaf.node_id_,                                                             //
                   "accession_numbers"_a = "",                                                              // format_accession_numbers(node)),
                   "vert"_a = 0,                                                                            // node.node_id.vertical),
                   "aa_transitions"_a = ""                                                                  // , aa_transitions.empty() ? std::string{} : fmt::format("[{}] ", aa_transitions))
    );

} // export_leaf

// ----------------------------------------------------------------------

void export_inode(fmt::memory_buffer& text, const ae::tree::Inode& inode, const ae::tree::Tree& tree, double edge_step, prefix_t& prefix)
{
    using namespace fmt::literals;
    const auto edge_size = static_cast<size_t>(*inode.edge * edge_step);
    fmt::format_to(std::back_inserter(text), "{prefix}{edge_symbol}\\ >>>> leaves: {leaves} <{node_id}>{aa_transitions} edge: {edge} cumul: {cumul}\n", //
                   "prefix"_a = fmt::join(prefix, std::string_view{}),                                                                                  //
                   "edge_symbol"_a = std::string(edge_size, '='), "edge"_a = *inode.edge, "cumul"_a = *inode.cumulative_edge,                           //
                   "leaves"_a = inode.number_of_leaves,                                                                                                                     // node.number_leaves_in_subtree()),
                   "node_id"_a = inode.node_id_,                                                                                                        //
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

// ======================================================================

std::string ae::tree::export_json(const Tree& tree)
{
    using namespace fmt::literals;
    fmt::memory_buffer text;
    fmt::format_to(std::back_inserter(text),
                   "{{\"_\": \"-*- js-indent-level: 1 -*-\",\n \"  version\": \"phylogenetic-tree-v3\",\n \"  date\": \"{today:%Y-%m-%d %H:%M %Z}\",\n \"v\": \"{subtype}\",\n",
                   "today"_a = fmt::localtime(std::time(nullptr)), //
                   "subtype"_a = ""                                //
    );
    // if (subtype == "B")
    //     fmt::format_to(std::back_inserter(text), " \"l\": \"{lineage}\"\n", "lineage"_a = lineage);
    fmt::format_to(std::back_inserter(text), " \"tree\"");
    std::string indent{" "};

    const auto format_node_begin = [&text, &indent](const Node* node) {
        fmt::format_to(std::back_inserter(text), "{}{{\n{} \"I\": {},", indent, indent, node->node_id_);
        if (node->edge != 0.0)
            fmt::format_to(std::back_inserter(text), " \"l\": {:.10g},", *node->edge);
        if (node->cumulative_edge != 0.0)
            fmt::format_to(std::back_inserter(text), " \"c\": {:.10g},", *node->cumulative_edge);
    };

    const auto format_node_end = [&text, &indent]() { fmt::format_to(std::back_inserter(text), "\n{}}}\n", indent); };

    const auto format_inode_pre = [&text, &indent, &tree, format_node_begin](const Inode* inode) {
        format_node_begin(inode);
        if (inode->node_id_ == node_index_t{0} && tree.maximum_cumulative() > 0)
            fmt::format_to(std::back_inserter(text), " \"M\": {:.10g},", tree.maximum_cumulative());
        // "A": ["aa subst", "N193K"],
        // "H": <true if hidden>,
        fmt::format_to(std::back_inserter(text), "\n");
        fmt::format_to(std::back_inserter(text), "{} \"t\": [\n", indent);
        indent.append(2, ' ');
    };

    const auto format_inode_post = [&text, &indent, format_node_end](const Inode*) {
        indent.resize(indent.size() - 2);
        fmt::format_to(std::back_inserter(text), "{} ]", indent);
        format_node_end();
    };

    const auto format_leaf = [&text, &indent, format_node_begin, format_node_end](const Leaf* leaf) {
        format_node_begin(leaf);
        fmt::format_to(std::back_inserter(text), "\n{} \"n\": \"{}\"", indent, leaf->name);
        if (!leaf->date.empty())
            fmt::format_to(std::back_inserter(text), ", \"d\": \"{}\"", leaf->date);
        if (!leaf->continent.empty())
            fmt::format_to(std::back_inserter(text), ", \"C\": \"{}\"", leaf->continent);
        if (!leaf->country.empty())
            fmt::format_to(std::back_inserter(text), ", \"c\": \"{}\"", leaf->country);
        if (!leaf->aa.empty())
            fmt::format_to(std::back_inserter(text), ",\n{} \"a\": \"{}\"", indent, leaf->aa);
        if (!leaf->nuc.empty())
            fmt::format_to(std::back_inserter(text), ",\n{} \"N\": \"{}\"", indent, leaf->nuc);
        // "L": ["clade", "2A1B"]
        // "h": ["hi names"],
        // "A": ["aa subst", "N193K"],
        format_node_end();
    };

    for (const auto ref : tree.visit(tree_visiting::all_pre_post)) {
        if (ref.pre())
            ref.visit(format_inode_pre, format_leaf);
        else
            ref.visit(format_inode_post, format_leaf);
    }
    fmt::format_to(std::back_inserter(text), "}}\n");
    return fmt::to_string(text);

} // ae::tree::export_json

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
