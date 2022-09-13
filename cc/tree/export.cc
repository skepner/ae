#include <stack>

#include "ext/simdjson.hh"
#include "utils/timeit.hh"
#include "utils/log.hh"
#include "tree/tree.hh"
#include "tree/tree-iterator.hh"
#include "tree/export.hh"

// ======================================================================

using prefix_t = std::vector<std::string>;
static void export_leaf(fmt::memory_buffer& text, const ae::tree::Leaf& leaf, double edge_step, const prefix_t& prefix);
static void export_inode(fmt::memory_buffer& text, const ae::tree::Inode& inode, const ae::tree::Tree& tree, double edge_step, prefix_t& prefix);

// ======================================================================

std::string ae::tree::export_text(const Tree& tree, const Inode& root)
{
    const double edge_step = 200.0 / *const_cast<Tree&>(tree).calculate_cumulative();

    fmt::memory_buffer text;
    fmt::format_to(std::back_inserter(text), "-*- Tal-Text-Tree -*-\n");
    prefix_t prefix;
    export_inode(text, root, tree, edge_step, prefix);
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
    fmt::format_to(std::back_inserter(text), fmt::runtime("{prefix}{edge_symbol}\\ >>>> leaves: {leaves} <{node_id}>{aa_transitions}{nuc_transitions} edge: {edge} cumul: {cumul}\n"), //
                   "prefix"_a = fmt::join(prefix, std::string_view{}),                                                                                                                 //
                   "edge_symbol"_a = edge_symbol, "edge"_a = inode.edge, "cumul"_a = inode.cumulative_edge,                                                                            //
                   "leaves"_a = inode.number_of_leaves(), // node.number_leaves_in_subtree()),
                   "node_id"_a = inode.node_id_,          //
                   "aa_transitions"_a = inode.aa_transitions.empty() ? std::string{} : fmt::format(" [{}]", inode.aa_transitions),
                   "nuc_transitions"_a = inode.nuc_transitions.empty() ? std::string{} : fmt::format(" [{}]", inode.nuc_transitions));
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
            if (const auto& child_inode = tree.inode(*child_it); child_inode.shown) {
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

std::string ae::tree::export_json(const Tree& tree, const Inode& root)
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

    fmt::format_to(std::back_inserter(text), " \"tree\":");
    std::string indent{" "};
    std::vector<bool> commas{false};
    commas.reserve(tree.depth());

    const auto format_node_begin = [&text, &indent, &commas](const Node* node) {
        if (commas.back()) {
            fmt::format_to(std::back_inserter(text), ",\n");
        }
        else {
            commas.back() = true;
            if (node->node_id_ != node_index_t{0}) // no newline right after "tree":
                fmt::format_to(std::back_inserter(text), "\n");
        }

        fmt::format_to(std::back_inserter(text), "{}{{\n{} \"I\": {}", indent, indent, node->node_id_.get());
        if (node->edge != 0.0)
            fmt::format_to(std::back_inserter(text), ", \"l\": {:.10g}", *node->edge);
        if (node->cumulative_edge != 0.0)
            fmt::format_to(std::back_inserter(text), ", \"c\": {:.10g}", *node->cumulative_edge);
    };

    const auto format_node_end = [&text, &indent]() {
        fmt::format_to(std::back_inserter(text), "\n{}}}", indent); };

    const auto format_node_sequences = [&text, &indent](const Node* node) {
        if (!node->aa.empty())
            fmt::format_to(std::back_inserter(text), ",\n{} \"a\": \"{}\"", indent, node->aa);
        if (!node->nuc.empty())
            fmt::format_to(std::back_inserter(text), ",\n{} \"N\": \"{}\"", indent, node->nuc);
    };

    const auto format_inode_pre = [&text, &indent, &tree, &commas, format_node_begin, format_node_sequences](const Inode* inode) {
        format_node_begin(inode);
        if (inode->node_id_ == node_index_t{0}) {
            if (tree.maximum_cumulative() > 0)
                fmt::format_to(std::back_inserter(text), ", \"M\": {:.10g}", tree.maximum_cumulative());
            if (inode->number_of_leaves() > 0)
                fmt::format_to(std::back_inserter(text), ", \"L\": {}", inode->number_of_leaves());
        }
        if (!inode->aa_transitions.empty())
            fmt::format_to(std::back_inserter(text), ", \"A\": [\"{}\"]", fmt::join(inode->aa_transitions.transitions, "\", \""));
        if (!inode->nuc_transitions.empty())
            fmt::format_to(std::back_inserter(text), ", \"B\": [\"{}\"]", fmt::join(inode->nuc_transitions.transitions, "\", \""));
        format_node_sequences(inode);
        // "H": <true if hidden>,

        // debugging set_raxml_ancestral_state_reconstruction_data
        // if (!inode->name.empty())
        //     fmt::format_to(std::back_inserter(text), " \"n\": \"{}\",", inode->name);
        // if (!inode->raxml_inode_names.empty())
        //     fmt::format_to(std::back_inserter(text), "\n{} \"rx\": [\"{}\"],", indent, fmt::join(inode->raxml_inode_names, "\", \""));

        fmt::format_to(std::back_inserter(text), ",\n{} \"t\": [", indent);
        indent.append(2, ' ');
        commas.push_back(false);
    };

    const auto format_inode_post = [&text, &indent, &commas, format_node_end](const Inode*) {
        indent.resize(indent.size() - 2);
        commas.pop_back();
        fmt::format_to(std::back_inserter(text), "\n{} ]", indent);
        format_node_end();
    };

    const auto format_leaf = [&text, &indent, format_node_begin, format_node_end, format_node_sequences](const Leaf* leaf) {
        format_node_begin(leaf);
        fmt::format_to(std::back_inserter(text), ",\n{} \"n\": \"{}\"", indent, leaf->name);
        if (!leaf->date.empty())
            fmt::format_to(std::back_inserter(text), ", \"d\": \"{}\"", leaf->date);
        if (!leaf->continent.empty())
            fmt::format_to(std::back_inserter(text), ", \"C\": \"{}\"", leaf->continent);
        if (!leaf->country.empty())
            fmt::format_to(std::back_inserter(text), ", \"D\": \"{}\"", leaf->country);
        format_node_sequences(leaf);
        if (!leaf->clades.empty())
            fmt::format_to(std::back_inserter(text), ",\n{} \"L\": [\"{}\"]", indent, fmt::join(leaf->clades, "\", \""));
        // "A": ["aa subst", "N193K"],
        format_node_end();
    };

    const auto format_leaf_post = [](const Leaf* leaf) {
        fmt::print("> export_json format_leaf_post \"{}\"\n", leaf->name);
    };

    bool within_subtree = false;
    for (const auto ref : tree.visit(tree_visiting::all_pre_post)) {
        if (ref.pre()) {
            if (!within_subtree && root.node_id_ == ref.node_id())
                within_subtree = true;
            if (within_subtree)
                ref.visit(format_inode_pre, format_leaf);
        }
        else if (within_subtree) {
            ref.visit(format_inode_post, format_leaf_post);
            if (root.node_id_ == ref.node_id())
                within_subtree = false;
        }
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
        // size_t number_of_leaves_{0}; // for inode

        void reset() { *this = NodeData{}; }
    };

    class TreeReader
    {
      public:
        enum class keep_json_node_id { no, yes };

        TreeReader(ae::tree::Tree& tree, keep_json_node_id keep_node_id) : tree_{tree}, keep_node_id_{keep_node_id}, current_node_{&tree_.root()}
            {
                parents_.push(tree.root_index());
            }
        TreeReader(const TreeReader&) = delete;
        TreeReader& operator=(const TreeReader&) = delete;

        void read(simdjson::simdjson_result<simdjson::ondemand::object> source)
        {
            source_objects_.emplace(source.begin(), source.end());
            while (!source_objects_.empty()) { // source_objects_.top().first != source_objects_.top().second) {
                advance_object_afterwards_ = true;
                auto field = *source_objects_.top().first;
                const std::string_view key = field.unescaped_key();

                if (key.size() != 1)
                    unhandled_key(key);
                else if (current_node_ == &tree_.root()) // root
                    root_node_field(key, field);
                else
                    node_field(key, field);

                if (advance_object_afterwards_)
                    ++source_objects_.top().first;
                while (check_object_end())
                    ++source_objects_.top().first; // advance if subtree ends
            }
        }

      private:
        using object_iterator = decltype(std::declval<simdjson::simdjson_result<simdjson::ondemand::object>>().begin());
        using array_iterator = decltype(std::declval<simdjson::simdjson_result<simdjson::ondemand::array>>().begin());
        using field_t = decltype(*std::declval<object_iterator>());

        ae::tree::Tree& tree_;
        keep_json_node_id keep_node_id_;
        NodeData temporary_target_node_{};
        Node* current_node_;
        Leaf* current_leaf_{nullptr};
        Inode* current_inode_{nullptr};
        std::stack<node_index_t> parents_{};
        std::stack<std::pair<object_iterator, object_iterator>> source_objects_{}; // pair(current, end) iterators of the current node object
        std::stack<std::pair<array_iterator, array_iterator>> subtree_current_elements_{}; // pair(current, end) iterators of the current subtree "t" array
        bool advance_object_afterwards_{true}; // reset at the beginning of each main loop in read()
        node_index_t current_node_id_{0};         // node.node_id_ is read from json and can be different

        void unhandled_key(std::string_view key) const { AD_WARNING("[tree-json] unhandled key \"{}\" for node {}", key, current_node_->node_id_); }

        void push_subtree_element()
        {
            auto new_source_object = (*subtree_current_elements_.top().first).get_object();
            source_objects_.emplace(new_source_object.begin(), new_source_object.end());
            temporary_target_node_.reset();
            current_node_ = &temporary_target_node_;
        }

        void push_subtree(field_t& field)
        {
            parents_.push(current_node_id_); // current_node_->node_id_ is perhaps read from json and can be different
            auto arr = field.value().get_array();
            subtree_current_elements_.emplace(arr.begin(), arr.end());
            push_subtree_element();
            advance_object_afterwards_ = false;
        }

        // returns if subtree ends
        bool check_object_end()
        {
            bool end_of_subtree{false};
            // fmt::print(stderr, ">>>> check_object_end\n");
            if (source_objects_.top().first == source_objects_.top().second) {
                // fmt::print(stderr, ">>>> end of object node:{} source_objects_:{} parents_:{} subtree_current_elements_:{}\n", current_node_->node_id_, source_objects_.size(), parents_.size(), subtree_current_elements_.size());
                // end of object
                current_leaf_ = nullptr;
                current_inode_ = nullptr;
                source_objects_.pop();
                if (!subtree_current_elements_.empty()) {
                    ++subtree_current_elements_.top().first;
                    if (subtree_current_elements_.top().first == subtree_current_elements_.top().second) {
                        // end of subtree
                        subtree_current_elements_.pop();
                        current_inode_ = &tree_.inode(parents_.top());
                        current_node_ = current_inode_;
                        parents_.pop();
                        // AD_DEBUG(source_objects_.size() < 5, "end of subtree {} parents_:{} source_objects_:{}", current_node_->node_id_, parents_.size(), source_objects_.size());
                        end_of_subtree = true;
                    }
                    else { // next element in the subtree
                        push_subtree_element();
                    }
                }
                else if (!source_objects_.empty()) {
                    throw std::runtime_error{AD_FORMAT("internal error: source_objects.size():{}", source_objects_.size())};
                }
                else {
                    // AD_DEBUG("end of tree? source_objects_:{}", source_objects_.size());
                    current_node_ = nullptr;
                }
            }
            return end_of_subtree;
        }

        void node_field(std::string_view key, field_t& field)
        {
            // fmt::print(stderr, ">>>> node field {}\n", key);
            switch (key[0]) {
                case 'I': // <node-id: int (ae only)>,
                    if (keep_node_id_ == keep_json_node_id::yes) {
                        if (const node_index_t node_id{static_cast<int64_t>(field.value())}; node_id != node_index_t{0})
                            current_node_->node_id_ = node_id;
                        // else
                        //     fmt::print(stderr, ">>>> invalid stored node_id for node {}\n", current_node_->node_id_);
                    }
                    break;
                case 'H': // <true if hidden>,
                    //!!! TODO
                    break;
                case 'l': // <edge-length: double>,
                    current_node_->edge = EdgeLength{static_cast<double>(field.value())};
                    break;
                case 'c': // <cumulative-edge-length: double>,
                    current_node_->cumulative_edge = EdgeLength{static_cast<double>(field.value())};
                    break;
                case 'a': // "aligned aa sequence",
                    current_node_->aa = sequences::sequence_aa_t{static_cast<std::string_view>(field.value())};
                    break;
                case 'N': // "aligned nuc sequence",
                    current_node_->nuc = sequences::sequence_nuc_t{static_cast<std::string_view>(field.value())};
                    break;
                case 'n':
                case 'd':
                case 'C':
                case 'D':
                case 'h':
                case 'L':
                    leaf_field(key, field);
                    break;
                case 't':
                case 'A':
                case 'B':
                    inode_field(key, field);
                    break;
                default:
                    unhandled_key(key);
                    break;
            }
        }

        template <typename NODE> void assign_current(node_index_t node_id, NODE& node)
        {
            current_node_id_ = node_id;
            static_cast<Node&>(node) = *current_node_;
            if (node.node_id_ == node_index_t{0})
                node.node_id_ = node_id;
            current_node_ = &node;
        }

        void leaf_field(std::string_view key, field_t& field)
        {
            if (current_node_ == &temporary_target_node_) {
                auto [index, leaf] = tree_.add_leaf(parents_.top());
                assign_current(index, leaf);
                current_leaf_ = &leaf;
            }
            else if (current_leaf_ == nullptr)
                throw std::runtime_error{AD_FORMAT("internal: leaf_field() current_leaf_==nullptr")};

            switch (key[0]) {
                case 'n': // "seq_id",
                    current_node_->name = static_cast<std::string_view>(field.value());
                    break;
                case 'd': // "2019-01-01: isolation date",
                    current_leaf_->date = static_cast<std::string_view>(field.value());
                    break;
                case 'C': // "continent",
                    current_leaf_->continent = static_cast<std::string_view>(field.value());
                    break;
                case 'D': // "country",
                    current_leaf_->country = static_cast<std::string_view>(field.value());
                    break;
                case 'h': // ["hi names"],
                    AD_WARNING("[tree-json] hi names are not handled");
                    for (auto hi_name [[maybe_unused]] : field.value().get_array())
                        ;
                    break;
                case 'L': // ["clade", "2A1B"]
                    for (auto clade : field.value().get_array())
                        current_leaf_->clades.insert(static_cast<std::string_view>(clade));
                    break;
                default:
                    unhandled_key(key);
                    break;
            }
        }

        void inode_field(std::string_view key, field_t& field)
        {
            if (current_node_ == &temporary_target_node_) {
                auto [index, inode] = tree_.add_inode(parents_.top());
                assign_current(index, inode);
                current_inode_ = &inode;
            }
            else if (current_inode_ == nullptr)
                throw std::runtime_error{AD_FORMAT("internal: inode_field() current_inode_==nullptr")};

            switch (key[0]) {
                case 'A': // ["aa subst", "N193K"],
                    for (auto transition : field.value().get_array())
                        current_inode_->aa_transitions.add(static_cast<std::string_view>(transition));
                    break;
                case 'B': // ["nuc subst", "A193T"],
                    for (auto transition : field.value().get_array())
                        current_inode_->nuc_transitions.add(static_cast<std::string_view>(transition));
                    break;
                case 't': // subtree
                    push_subtree(field);
                    break;
                default:
                    unhandled_key(key);
                    break;
            }
        }

        void root_node_field(std::string_view key, field_t& field)
        {
            // fmt::print(stderr, ">>>> root node field \"{}\"\n", key);
            switch (key[0]) {
                case 'I':
                    if (static_cast<int64_t>(field.value()) != 0)
                        AD_WARNING("[tree-json] node_id (\"I\") for root is {}", static_cast<int64_t>(field.value()));
                    break;
                case 'M': // max cumulative
                    break;
                case 'L': // number-of-leaves-in-tree
                    break;
                case 'a': // "aa sequence inferred by e.g. raxml --ancestral",
                    current_node_->aa = sequences::sequence_aa_t{static_cast<std::string_view>(field.value())};
                    break;
                case 'N': // "nuc sequence inferred by e.g. raxml --ancestral",
                    current_node_->nuc = sequences::sequence_nuc_t{static_cast<std::string_view>(field.value())};
                    break;
                case 't': // subtree
                    push_subtree(field);
                    break;
                default:
                    unhandled_key(key);
                    break;
            }
        }
    };

} // namespace ae::tree

// ----------------------------------------------------------------------

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
                    TreeReader(*tree, TreeReader::keep_json_node_id::yes).read(field.value().get_object());
                    tree->update_number_of_leaves_in_subtree();
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

// ----------------------------------------------------------------------

void ae::tree::load_join_json(const std::string& data, Tree& tree, Inode& join_at, const std::filesystem::path& filename)
{
    fmt::print(stderr, "> load_join_json not implemented\n");
    throw std::runtime_error{AD_FORMAT("load_join_json not implemented")};

} // ae::tree::load_join_json


// ======================================================================
