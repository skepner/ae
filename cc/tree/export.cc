#include <stack>

#include "ext/simdjson.hh"
#include "utils/timeit.hh"
#include "tree/tree.hh"
#include "tree/tree-iterator.hh"
#include "tree/export.hh"

// ======================================================================

using prefix_t = std::vector<std::string>;
static void export_leaf(fmt::memory_buffer& text, const ae::tree::Leaf& leaf, double edge_step, const prefix_t& prefix);
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

void export_leaf(fmt::memory_buffer& text, const ae::tree::Leaf& leaf, double edge_step, const prefix_t& prefix)
{
    using namespace fmt::literals;
    const std::string edge_symbol(static_cast<size_t>(*leaf.edge * edge_step), '-');
    fmt::format_to(std::back_inserter(text), fmt::runtime("{prefix}{edge_symbol} \"{name}\" <{node_id}>{aa_transitions}{accession_numbers} edge: {edge}  cumul: {cumul}  v:{vert}\n"),
                   "prefix"_a = fmt::join(prefix, std::string_view{}),                                      //
                   "edge_symbol"_a = edge_symbol, "edge"_a = leaf.edge, "cumul"_a = leaf.cumulative_edge, //
                   "name"_a = leaf.name,                                                                    //
                   "node_id"_a = leaf.node_id_,                                                            //
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
    const std::string edge_symbol(edge_size, '=');
    fmt::format_to(std::back_inserter(text), fmt::runtime("{prefix}{edge_symbol}\\ >>>> leaves: {leaves} <{node_id}>{aa_transitions} edge: {edge} cumul: {cumul}\n"), //
                   "prefix"_a = fmt::join(prefix, std::string_view{}),                                                                                  //
                   "edge_symbol"_a = edge_symbol, "edge"_a = inode.edge, "cumul"_a = inode.cumulative_edge,                           //
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
    Timeit ti{"tree::export_json", std::chrono::milliseconds{100}};

    using namespace fmt::literals;
    fmt::memory_buffer text;
    fmt::format_to(std::back_inserter(text), "{{\"_\": \"-*- js-indent-level: 1 -*-\",\n \"  version\": \"phylogenetic-tree-v3\",\n \"  date\": \"{:%Y-%m-%d %H:%M %Z}\",\n", fmt::localtime(std::time(nullptr)));
    if (!tree.subtype().empty()) {
        fmt::format_to(std::back_inserter(text), " \"v\": \"{}\",", tree.subtype());
        if (tree.subtype() == virus::type_subtype_t{"B"} && !tree.lineage().empty()) {
            if (tree.lineage() == sequences::lineage_t{"V"})
                fmt::format_to(std::back_inserter(text), " \"l\": \"VICTORIA\",");
            else if (tree.lineage() == sequences::lineage_t{"Y"})
                fmt::format_to(std::back_inserter(text), " \"l\": \"YAMAGATA\",");
            else
                fmt::format_to(std::back_inserter(text), " \"l\": \"{}\",", tree.lineage());
        }
        fmt::format_to(std::back_inserter(text), "\n");
    }

    fmt::format_to(std::back_inserter(text), " \"tree\"");
    std::string indent{" "};
    std::vector<bool> commas{false};
    commas.reserve(tree.depth());

    const auto format_node_begin = [&text, &indent, &commas](const Node* node) {
        if (commas.back()) {
            fmt::format_to(std::back_inserter(text), ",\n");
        }
        else {
            commas.back() = true;
            fmt::format_to(std::back_inserter(text), "\n");
        }

        fmt::format_to(std::back_inserter(text), "{}{{\n{} \"I\": {},", indent, indent, node->node_id_.get());
        if (node->edge != 0.0)
            fmt::format_to(std::back_inserter(text), " \"l\": {:.10g},", *node->edge);
        if (node->cumulative_edge != 0.0)
            fmt::format_to(std::back_inserter(text), " \"c\": {:.10g},", *node->cumulative_edge);
    };

    const auto format_node_end = [&text, &indent]() { fmt::format_to(std::back_inserter(text), "\n{}}}", indent); };

    const auto format_inode_pre = [&text, &indent, &tree, &commas, format_node_begin](const Inode* inode) {
        format_node_begin(inode);
        if (inode->node_id_ == node_index_t{0}) {
            if (tree.maximum_cumulative() > 0)
                fmt::format_to(std::back_inserter(text), " \"M\": {:.10g},", tree.maximum_cumulative());
            if (inode->number_of_leaves > 0)
                fmt::format_to(std::back_inserter(text), " \"L\": {},", inode->number_of_leaves);
        }
        // "A": ["aa subst", "N193K"],
        // "H": <true if hidden>,
        fmt::format_to(std::back_inserter(text), "\n");
        fmt::format_to(std::back_inserter(text), "{} \"t\": [", indent);
        indent.append(2, ' ');
        commas.push_back(false);
    };

    const auto format_inode_post = [&text, &indent, &commas, format_node_end](const Inode*) {
        indent.resize(indent.size() - 2);
        commas.pop_back();
        fmt::format_to(std::back_inserter(text), "\n{} ]", indent);
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
            fmt::format_to(std::back_inserter(text), ", \"D\": \"{}\"", leaf->country);
        if (!leaf->aa.empty())
            fmt::format_to(std::back_inserter(text), ",\n{} \"a\": \"{}\"", indent, leaf->aa);
        if (!leaf->nuc.empty())
            fmt::format_to(std::back_inserter(text), ",\n{} \"N\": \"{}\"", indent, leaf->nuc);
        if (!leaf->clades.empty())
            fmt::format_to(std::back_inserter(text), ",\n{} \"L\": [\"{}\"]", indent, fmt::join(leaf->clades, "\", \""));
        // "h": ["hi names"],
        // "A": ["aa subst", "N193K"],
        format_node_end();
    };

    const auto format_leaf_post = [](const Leaf* leaf) {
        fmt::print("> export_json format_leaf_post \"{}\"\n", leaf->name);
    };

    for (const auto ref : tree.visit(tree_visiting::all_pre_post)) {
        if (ref.pre())
            ref.visit(format_inode_pre, format_leaf);
        else
            ref.visit(format_inode_post, format_leaf_post);
    }
    fmt::format_to(std::back_inserter(text), "\n}}\n");
    return fmt::to_string(text);

} // ae::tree::export_json

// ======================================================================

bool ae::tree::is_json(std::string_view data)
{
    return data.size() > 100 && data[0] == '{' && data.find("\"phylogenetic-tree-v3\"") != std::string_view::npos;

} // ae::tree::is_json

// ----------------------------------------------------------------------

namespace ae::tree
{
    class Error : public std::runtime_error
    {
      public:
        template <typename... Args> Error(fmt::format_string<Args...> format, Args&&... args) : std::runtime_error{fmt::format("[tree-json] {}", fmt::format(format, args...))} {}
    };

    struct NodeData : public Leaf
    {
        size_t number_of_leaves{0}; // for inode
    };

    static inline void read_node_field(Node& node, std::string_view key, simdjson::simdjson_result<simdjson::fallback::ondemand::value>&& field)
    {
        if (key == "I") // node_id
            node.node_id_ = node_index_t{static_cast<int64_t>(field.value())};
        else if (key == "l")
            node.edge = EdgeLength{static_cast<double>(field.value())};
        else if (key == "c")
            node.cumulative_edge = EdgeLength{static_cast<double>(field.value())};
        else
            fmt::print(">> [tree-json] unhandled node key \"{}\"\n", key);
    }

    template <typename Arr> static inline void read_subtree(ae::tree::Tree& tree, std::stack<node_index_t>& parents, Arr&& source)
    {
        for (auto element : source) {
            NodeData source_node;
            Node* current = &source_node;
            for (auto field : element.get_object()) {
                if (const std::string_view key = field.unescaped_key(); key == "t") {
                     // this is inode
                }
                else if (key == "n") {
                    // this is leaf
                }
                else {
                    read_node_field(*current, key, field.value());
                }
            }
            if (current == &source_node) {
                throw Error{"unrecognized node type"};
            }
        }
    }

    template <typename Obj> static inline void read_tree(ae::tree::Tree& tree, Obj&& source)
    {
        std::stack<node_index_t> parents;
        parents.push(tree.root_index());
        auto& root_node = tree.root();
        for (auto field : source) {
            if (const std::string_view key = field.unescaped_key(); key == "I") { // node_id
                const node_index_t node_id{static_cast<int64_t>(field.value())};
                if (node_id != node_index_t{0})
                    fmt::print(">> [tree-json] node_id (\"I\") for root is {}\n", node_id);
            }
            else if (key == "L") { // number of leaves
                root_node.number_of_leaves = static_cast<uint64_t>(field.value());
            }
            else if (key == "t") { // subtree
                auto subtree = field.value().get_array();
                read_subtree(tree, parents, subtree);
            }
            else if (key == "M") { // ignored
            }
            else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
                fmt::print(">> [tree-json] unhandled \"{}\" in the tree root\n", key);
        }

    }

} // namespace ae::tree

std::shared_ptr<ae::tree::Tree> ae::tree::load_json(const std::string& data, const std::filesystem::path& filename)
{
    auto tree = std::make_shared<Tree>();
    try {
        simdjson::ondemand::parser parser;
        auto doc = parser.iterate(data, data.size() + simdjson::SIMDJSON_PADDING);
        const auto current_location_offset = [&doc, data] { return doc.current_location().value() - data.data(); };
        const auto current_location_snippet = [&doc](size_t size) { return std::string_view(doc.current_location().value(), size); };

        try {
            for (auto field : doc.get_object()) {
                const std::string_view key = field.unescaped_key();
                // fmt::print(">>>> key \"{}\"\n", key);
                if (key == "  version") {
                    if (const std::string_view ver{field.value()}; ver != "phylogenetic-tree-v3")
                        throw Error{"unsupported version: \"{}\"", ver};
                }
                else if (key == "v") {
                    tree->subtype(virus::type_subtype_t{field.value()});
                }
                else if (key == "l") {
                    tree->lineage(sequences::lineage_t{field.value()});
                }
                else if (key == "tree") {
                    auto source = field.value().get_object();
                    read_tree(*tree, source);
                }
                else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
                    fmt::print(">> [tree-json] unhandled \"{}\"\n", key);
            }
        }
        catch (simdjson::simdjson_error& err) {
            fmt::print("> {} parsing error: {} at {} \"{}\"\n", filename, err.what(), current_location_offset(), current_location_snippet(50));
        }
    }
    catch (simdjson::simdjson_error& err) {
        fmt::print("> {} json parser creation error: {} (UNESCAPED_CHARS means a char < 0x20)\n", filename, err.what());
    }
    return tree;

} // ae::tree::load_json

// ======================================================================
