#include "ext/simdjson.hh"
#include "ext/range-v3.hh"
#include "utils/timeit.hh"
#include "sequences/seqdb.hh"
#include "sequences/clades.hh"
#include "tree/tree.hh"
#include "tree/newick.hh"
#include "tree/export.hh"

// ======================================================================

std::pair<ae::tree::node_index_t, ae::tree::Inode&> ae::tree::Tree::add_inode(node_index_t parent)
{
    const node_index_t index{-static_cast<node_index_base_t>(inodes_.size())};
    inodes_[-*parent].children.push_back(index);
    return {index, inodes_.emplace_back()};

} // ae::tree::Tree::add_inode

// ----------------------------------------------------------------------

std::pair<ae::tree::node_index_t, ae::tree::Leaf&> ae::tree::Tree::add_leaf(node_index_t parent)
{
    const node_index_t index{static_cast<node_index_base_t>(leaves_.size())};
    inodes_[-*parent].children.push_back(index);
    return {index, leaves_.emplace_back()};

} // ae::tree::Tree::add_leaf

// ----------------------------------------------------------------------

std::pair<ae::tree::node_index_t, ae::tree::Leaf&> ae::tree::Tree::add_leaf(node_index_t parent, std::string_view name, EdgeLength edge)
{
    const node_index_t index{static_cast<node_index_base_t>(leaves_.size())};
    inodes_[-*parent].children.push_back(index);
    return {index, leaves_.emplace_back(name, edge)};

} // ae::tree::Tree::add_leaf

// ----------------------------------------------------------------------

size_t ae::tree::Tree::depth() const
{
    if (depth_ == 0) {
        // without using iterator! because iterators use depth_ value
        const auto descent = [this](const Inode& inod, size_t depth, auto&& self) -> void {
            for (const auto child_index : inod.children) {
                if (!ae::tree::is_leaf(child_index))
                    self(inode(child_index), depth + 1, self);
                else
                    depth_ = std::max(depth_, depth + 1);
            }
        };
        descent(root(), 0, descent);
        // fmt::print(">>>> tree depth {}\n", depth_);
    }
    return depth_;

} // ae::tree::Tree::depth

// ----------------------------------------------------------------------

ae::tree::EdgeLength ae::tree::Tree::calculate_cumulative(bool force)
{
    if (force || max_cumulative < EdgeLength{0}) {
        EdgeLength cumulative{0.0};
        max_cumulative = EdgeLength{0.0};
        for (auto ref : visit(tree_visiting::all_pre_post)) {
            if (ref.pre())
                ref.visit([&cumulative, this]<typename Node>(Node* node) {
                    node->cumulative_edge = cumulative + node->edge;
                    if constexpr (std::is_same_v<Node, Inode>)
                        cumulative = node->cumulative_edge;
                    else
                        max_cumulative = std::max(max_cumulative, node->cumulative_edge);
                });
            else
                ref.visit(
                    [&cumulative, this](Inode* inode) {
                        cumulative -= inode->edge;
                        // post -> update number_of_leaves
                        inode->number_of_leaves_ = 0;
                        for (const auto child_id : inode->children)
                            inode->number_of_leaves_ += node(child_id).visit([](const auto* sub_node) { return sub_node->number_of_leaves(); });
                    },
                    [](const Leaf*) {});
        }

        // for (const auto ref : visit(tree_visiting::all)) {
        //     fmt::print(">>>> all {}\n", ref.to_string());
        // }
        // fmt::print("\n\n");
        // for (const auto ref : visit(tree_visiting::all_pre_post)) {
        //     fmt::print(">>>> all-pre-post {}\n", ref.to_string());
        // }
        // fmt::print("\n\n");
        // for (const auto ref : visit(tree_visiting::all_post)) {
        //     fmt::print(">>>> all-post {}\n", ref.to_string());
        // }
        // fmt::print("\n\n");
        // for (const auto ref : visit(tree_visiting::inodes_post)) {
        //     fmt::print(">>>> inodes-post {}\n", ref.to_string());
        // }
        // fmt::print("\n\n");
        // for (const auto ref : visit(tree_visiting::leaves)) {
        //     fmt::print(">>>> leaves {}\n", ref.to_string());
        // }
        // fmt::print("\n\n");
        // for (const auto ref : visit(tree_visiting::inodes)) {
        //     fmt::print(">>>> inodes {}\n", ref.to_string());
        // }
    }
    // fmt::print(">>>> cumulative {}\n", max_cumulative);
    return max_cumulative;

} // ae::tree::Tree::calculate_cumulative

// ----------------------------------------------------------------------

void ae::tree::Tree::update_number_of_leaves_in_subtree()
{
    // Timeit ti{"update_number_of_leaves_in_subtree"};
    for (auto ref : visit(tree_visiting::inodes_post)) {
        ref.visit(
            [this](Inode* inode) {
                inode->number_of_leaves_ = 0;
                for (const auto child_id : inode->children)
                    inode->number_of_leaves_ += node(child_id).visit([](const auto* sub_node) { return sub_node->number_of_leaves(); });
            },
            [](const Leaf*) {});
    }

} // ae::tree::Tree::update_number_of_leaves_in_subtree

// ----------------------------------------------------------------------

static inline std::unordered_map<int, ae::tree::EdgeLength> calculate_max_edge_lengths(const ae::tree::Tree& tree)
{
    std::unordered_map<int, ae::tree::EdgeLength> max_edge_lengths;
    for (auto ref : tree.visit(ae::tree::tree_visiting::all_post)) {
        if (ref.pre()) {
            ref.visit(
                [&max_edge_lengths](const ae::tree::Leaf* leaf) { max_edge_lengths.emplace(*leaf->node_id_, leaf->edge); },
                [](const ae::tree::Inode*) {}
            );
        }
        else {
            ref.visit(
                [](const ae::tree::Leaf*) {},
                [&max_edge_lengths](const ae::tree::Inode* inode) {
                    ae::tree::EdgeLength max_edge{0.0};
                    for (ae::tree::node_index_t child : inode->children)
                        max_edge = std::max(max_edge, max_edge_lengths.find(*child)->second);
                    max_edge_lengths.emplace(*inode->node_id_, max_edge);
                }
            );
        }
    }
    // AD_DEBUG("max_edge_lengths {}", max_edge_lengths.size());

    return max_edge_lengths;

} // calculate_max_edge_lengths

// ----------------------------------------------------------------------

void ae::tree::Tree::ladderize(ladderize_method method)
{
    Timeit ti{"Tree::ladderize", std::chrono::milliseconds{100}};

    const auto max_edge_lengths = calculate_max_edge_lengths(*this);

    const auto compare_max_edge_length = [&max_edge_lengths](node_index_t ci1, node_index_t ci2) {
        const auto e1 = max_edge_lengths.find(*ci1)->second, e2 = max_edge_lengths.find(*ci2)->second;
        if (e1 == e2)
            return ci1 < ci2;
        else
            return e1 < e2;
    };

    const auto compare_number_of_leaves = [this, compare_max_edge_length](node_index_t ci1, node_index_t ci2) {
        const auto number_of_leaves = [](const auto& child) { return child.visit([](const auto* nod) {
            return nod->number_of_leaves(); });
        };
        const auto n1 = node(ci1), n2 = node(ci2);
        const auto nc1 = number_of_leaves(n1), nc2 = number_of_leaves(n2);
        if (nc1 == nc2)
            return compare_max_edge_length(ci1, ci2);
        else
            return nc1 < nc2;
    };

    const auto by_number_of_leaves = [this, compare_number_of_leaves]() {
        update_number_of_leaves_in_subtree();

        for (auto ref : visit(tree_visiting::inodes_post)) {
            auto& inode = *ref.inode();
            std::sort(inode.children.begin(), inode.children.end(), compare_number_of_leaves);
        }
    };

    switch (method) {
        case ladderize_method::max_edge_length:
            AD_WARNING("Ladderizing by max-edge-length Not implemented");
            break;
        case ladderize_method::number_of_leaves:
            by_number_of_leaves();
            break;
        case ladderize_method::none:
            AD_WARNING("no ladderizing");
            break;
    }

} // ae::tree::Tree::ladderize

// ----------------------------------------------------------------------

void ae::tree::Tree::set_node_id(reset_node_id reset)
{
    const auto set = [reset](Node& node, node_index_t node_id) {
        if (reset == reset_node_id::yes || node.node_id_ == node_index_t{0})
            node.node_id_ = node_id;
    };

    node_index_t inode_id{0};
    for (auto inode = inodes_.begin(); inode != inodes_.end(); ++inode, --inode_id)
        set(*inode, inode_id);
    node_index_t leaf_id{0};
    for (auto leaf = leaves_.begin(); leaf != leaves_.end(); ++leaf, ++leaf_id)
        set(*leaf, leaf_id);

} // ae::tree::Tree::set_node_id

// ----------------------------------------------------------------------

void ae::tree::Tree::populate_with_sequences(const virus::type_subtype_t& subtype)
{
    Timeit ti{"populate_with_sequences", std::chrono::milliseconds{100}};
    const auto& seqdb = sequences::seqdb_for_subtype(subtype);

    for (auto leaf_ref : visit(tree_visiting::leaves)) {
        Leaf* leaf = leaf_ref.leaf();
        if (const auto ref = seqdb.find_by_seq_id(sequences::seq_id_t{leaf->name}, sequences::Seqdb::set_master::yes); ref) {
            leaf->aa = ref.aa();
            leaf->nuc = ref.nuc();
            leaf->date = ref.entry->date();
            leaf->continent = ref.entry->continent;
            leaf->country = ref.entry->country;
            if (lineage_.empty() && !ref.entry->lineage.empty())
                lineage_ = ref.entry->lineage;
        }
        else
            fmt::print(">> [seqdb {}] seq_id not found in seqdb: \"{}\"\n", subtype, leaf->name);
    }

    subtype_ = subtype;

} // ae::tree::Tree::populate_with_sequences

// ----------------------------------------------------------------------

void ae::tree::Tree::populate_with_duplicates(const virus::type_subtype_t& subtype)
{
    Timeit ti{"populate_with_duplicates", std::chrono::milliseconds{100}};
    const auto& seqdb = sequences::seqdb_for_subtype(subtype);

    std::vector<Inode*> parents;
    std::vector<std::pair<Inode*, Leaf>> to_add;
    for (auto ref : visit(tree_visiting::all_pre_post)) {
        // cannot add leaves during iteration (breaks iterator)
        if (ref.pre())
            ref.visit([&parents](Inode* inode) { parents.push_back(inode); },
                      [&seqdb, &parents, &to_add](const Leaf* leaf) {
                          if (const auto present_ref = seqdb.find_by_seq_id(sequences::seq_id_t{leaf->name}, sequences::Seqdb::set_master::yes); present_ref) {
                              const auto refs_for_hash = seqdb.find_all_by_hash(present_ref.seq->hash);
                              for (const auto& ref_for_hash : refs_for_hash) {
                                  if (ref_for_hash != present_ref)
                                      to_add.emplace_back(parents.back(), Leaf{ref_for_hash.seq_id(), leaf->edge, leaf->cumulative_edge});
                              }
                          }
                          else
                              fmt::print(">> [seqdb {}] seq_id not found in seqdb: \"{}\"\n", seqdb.subtype(), leaf->name);
                      });
        else // post
            ref.visit([&parents](Inode*) { parents.pop_back(); }, [](Leaf*) {});
    }

    for (auto& [parent_for_new_leaf, new_leaf] : to_add) {
        // if (new_leaf.edge > 0) {
        //     // TODO: should replace original leaf with inode and add original leaf and new leaves to that new inode
        //     fmt::print(">> adding leaf with non-zero edge: {} \"{}\"\n", new_leaf.edge, new_leaf.name);
        // }
        const node_index_t index{static_cast<node_index_base_t>(leaves_.size())};
        parent_for_new_leaf->children.push_back(index);
        new_leaf.node_id_ = index;
        leaves_.push_back(std::move(new_leaf));
    }

    update_number_of_leaves_in_subtree();

} // ae::tree::Tree::populate_with_duplicates

// ----------------------------------------------------------------------

void ae::tree::Tree::set_clades(const std::filesystem::path& clades_json_file)
{
    Timeit ti{"set_clades", std::chrono::milliseconds{100}};
    const sequences::Clades clades{clades_json_file};
    for (auto leaf_ref : visit(tree_visiting::leaves)) {
        if (Leaf* leaf = leaf_ref.leaf(); !leaf->aa.empty() || !leaf->nuc.empty())
            leaf->clades = clades.clades(leaf->aa, leaf->nuc, subtype_, lineage_);
    }

} // ae::tree::Tree::set_clades

// ----------------------------------------------------------------------

ae::tree::Nodes ae::tree::Tree::select_all()
{
    // have to walk the tree, because there could be unlinked nodes (after remove())
    Nodes nodes{{}, *this};
    nodes.nodes.reserve(inodes_.size() + leaves_.size());
    for (auto ref : visit(tree_visiting::all))
        nodes.nodes.push_back(ref.node_id());
    return nodes;

} // ae::tree::Tree::select_all

// ----------------------------------------------------------------------

ae::tree::Nodes ae::tree::Tree::select_leaves()
{
    // have to walk the tree, because there could be unlinked nodes (after remove())
    Nodes nodes{{}, *this};
    nodes.nodes.reserve(leaves_.size());
    for (auto ref : visit(tree_visiting::leaves))
        nodes.nodes.push_back(ref.node_id());
    return nodes;

} // ae::tree::Tree::select_leaves

// ----------------------------------------------------------------------

ae::tree::Nodes ae::tree::Tree::select_inodes()
{
    // have to walk the tree, because there could be unlinked nodes (after remove())
    Nodes nodes{{}, *this};
    nodes.nodes.reserve(inodes_.size());
    for (auto ref : visit(tree_visiting::inodes))
        nodes.nodes.push_back(ref.node_id());
    return nodes;

} // ae::tree::Tree::select_inodes

// ----------------------------------------------------------------------

ae::tree::Nodes ae::tree::Tree::select_inodes_with_just_one_child()
{
    Nodes nodes{{}, *this};
    for (auto ref : visit(tree_visiting::inodes)) {
        const auto& inode = *ref.inode();
        if (inode.number_of_leaves() < 2)
            nodes.nodes.push_back(inode.node_id_);
    }
    return nodes;

} // ae::tree::Tree::select_inodes_with_just_one_child

// ----------------------------------------------------------------------

ae::tree::node_index_t ae::tree::Tree::parent(node_index_t child) const
{
    if (child == node_index_t{0})
        throw std::out_of_range{"no parent for the root node"};
    for (auto inode_it = inodes_.begin(); inode_it != inodes_.end(); ++inode_it) {
        if (const auto found = std::find(std::begin(inode_it->children), std::end(inode_it->children), child); found != std::end(inode_it->children))
            return node_index_t{static_cast<node_index_base_t>(inodes_.begin() - inode_it)}; // negative!
    }
    throw std::invalid_argument{"child not found in the tree"};

} // ae::tree::Tree::parent

// ----------------------------------------------------------------------

void ae::tree::Tree::remove(const std::vector<node_index_t>& nodes)
{
    Timeit ti{"Tree::remove", std::chrono::milliseconds{100}};

    const auto child_empty = [this](auto child_id) { return !is_leaf(child_id) && inode(child_id).children.empty(); };

    // normally just one entry in the collection, but there could be more in complcated cases: multiple single_child_inodes (direct and indirect children) for the same parent
    std::vector<Inode*> single_child_inodes;
    for (auto ref : visit(tree_visiting::inodes_post)) {
        ref.visit(
            [&nodes, &single_child_inodes, child_empty, this](Inode* inode) {
                inode->children.erase(
                    std::remove_if(std::begin(inode->children), std::end(inode->children),
                                   [&nodes, child_empty](auto child_id) { return std::find(std::begin(nodes), std::end(nodes), child_id) != std::end(nodes) || child_empty(child_id); }),
                    std::end(inode->children));
                if (!single_child_inodes.empty()) {
                    // fmt::print(">>>> single_child_inodes {}\n", single_child_inodes[0]->node_id_);
                    decltype(single_child_inodes)::iterator single_child_inode;
                    if (const auto child = std::find_if(std::begin(inode->children), std::end(inode->children),
                                                        [&single_child_inodes, &single_child_inode](const auto child_id) {
                                                            single_child_inode = std::find_if(std::begin(single_child_inodes), std::end(single_child_inodes),
                                                                                              [child_id](const Inode* sci) { return child_id == sci->node_id_; });
                                                            return single_child_inode != std::end(single_child_inodes);
                                                        });
                        child != std::end(inode->children)) {
                        // one of the single_child_inodes is emong inode->children
                        // fmt::print(">>> child of {} is a single child inode {}\n", inode->node_id_, single_child_inode->node_id_);
                        // add to edge of single_child_inode->children[0] edge of single_child_inode
                        node((*single_child_inode)->children[0]).visit([parent_edge = (*single_child_inode)->edge](auto* node) { node->edge += parent_edge; });
                        // replace single_child_inode in the children of this with single_child_inode->children[0]
                        *child = (*single_child_inode)->children[0];
                        // remove fixed single_child_inode from the list
                        single_child_inodes.erase(single_child_inode);
                    }
                }
                if (inode->children.size() == 1)
                    single_child_inodes.push_back(inode);
            },
            [](Leaf*) {});
    }
    calculate_cumulative(true);

} // ae::tree::Tree::remove

// ----------------------------------------------------------------------

void ae::tree::Tree::remove_leaves_isolated_before(std::string_view last_date, const std::vector<std::string>& important)
{
    // keep at least two children in each inode (attempt to keep tree topology)

    Timeit ti{"Tree::remove_leaves_isolated_before", std::chrono::milliseconds{100}};

    const auto is_important = [this, &important](node_index_t node_id) -> bool {
        if (important.empty())
            return false;
        const auto& name = leaf(node_id).name;
        const auto node_important = std::find_if(std::begin(important), std::end(important), [&name](const auto& imp) { return name.find(imp) != std::string::npos; }) != std::end(important);
        // AD_DEBUG(node_important, "important {}", name);
        return node_important;
    };

    for (auto ref : visit(tree_visiting::inodes_post)) {
        ref.visit(
            [this, last_date, is_important](Inode* inode) {
                if (inode->children.size() > 2) {
                    std::vector<node_index_t> to_remove;
                    std::copy_if(inode->children.begin(), inode->children.end(), std::back_inserter(to_remove),
                                 [this, last_date, is_important](node_index_t node_id) { return is_leaf(node_id) && leaf(node_id).date < last_date && !is_important(node_id); });
                    if (to_remove.size() > (inode->children.size() - 2))
                        to_remove.resize(inode->children.size() - 2); // keep at least two children in inode
                    inode->children.erase(std::remove_if(inode->children.begin(), inode->children.end(),
                                                         [&to_remove](node_index_t node_id) { return std::find(to_remove.begin(), to_remove.end(), node_id) != to_remove.end(); }),
                                          inode->children.end());
                }
            },
            [](Leaf*) {});
    }
    calculate_cumulative(true);

} // ae::tree::Tree::remove_leaves_isolated_before

// ----------------------------------------------------------------------

std::vector<std::string> ae::tree::Tree::fix_names_by_seqdb(const virus::type_subtype_t& subtype)
{
    const auto& seqdb = ae::sequences::seqdb_for_subtype(subtype);
    std::vector<std::string> messages;
    for (auto node_ref : visit(tree_visiting::leaves)) {
        auto& leaf = *node_ref.leaf();
        const ae::sequences::seq_id_t seq_id{leaf.name};
        auto ref = seqdb.find_by_seq_id(seq_id, ae::sequences::Seqdb::set_master::no);
        if (ref.empty() || !ref.is_master())
            ref = seqdb.find_by_hash(ae::hash_t{seq_id->substr(seq_id.size() - 8)});
        if (!ref.empty()) {
            if (ref.seq_id() != seq_id) {
                leaf.name = ref.seq_id();
                messages.push_back(fmt::format(">>> renamed \"{}\" <-- \"{}\"", ref.seq_id(), seq_id));
            }
        }
        else
            messages.push_back(fmt::format(">> not found: \"{}\"", seq_id));
    }
    return messages;

} // ae::tree::Tree::fix_names_by_seqdb

// ----------------------------------------------------------------------

ae::tree::node_index_t ae::tree::Tree::first_leaf(node_index_t index) const
{
    while (true) {
        if (is_leaf(index))
            return index;
        index = inode(index).children[0];
    }

} // ae::tree::Tree::first_leaf

// ----------------------------------------------------------------------

ae::tree::Nodes& ae::tree::Nodes::sort_by_cumulative()
{
    const auto cumulative_edge = [this](const auto& id) { return tree.node(id).visit([](const auto* node) { return node->cumulative_edge; }); };
    ranges::sort(nodes, [cumulative_edge](const auto& id1, const auto& id2) { return cumulative_edge(id1) > cumulative_edge(id2); });
    return *this;

} // ae::tree::Nodes::sort_by_cumulative

// ----------------------------------------------------------------------

ae::tree::Nodes& ae::tree::Nodes::filter_by_cumulative_more_than(double min_cumulative)
{
    ranges::actions::remove_if(nodes,
                               [this, min_cumulative](const auto& id) { return tree.node(id).visit([min_cumulative](const auto* node) { return node->cumulative_edge <= min_cumulative; }); });
    return *this;

} // ae::tree::Nodes::filter_by_cumulative_more_than

// ----------------------------------------------------------------------

ae::tree::Nodes& ae::tree::Nodes::filter_seq_id(const std::vector<std::string>& seq_ids)
{
    ranges::actions::remove_if(nodes, [this, &seq_ids](const auto& id) {
        return tree.node(id).visit([&seq_ids](const auto* node) { return std::find(std::begin(seq_ids), std::end(seq_ids), node->name) == std::end(seq_ids); });
    });
    return *this;

} // ae::tree::Nodes::filter_seq_id

// ----------------------------------------------------------------------

std::shared_ptr<ae::tree::Tree> ae::tree::load(const std::filesystem::path& filename)
{
    std::shared_ptr<ae::tree::Tree> tree;
    const auto data = file::read(filename, ::simdjson::SIMDJSON_PADDING);
    if (is_newick(data))
        tree = load_newick(data);
    else if (is_json(data))
        tree = load_json(data, filename);
    else
        throw std::runtime_error{AD_FORMAT("cannot load tree from \"{}\": unknown file format", filename)};
    tree->calculate_cumulative();
    tree->set_node_id(Tree::reset_node_id::no);
    return tree;

} // ae::tree::load

// ----------------------------------------------------------------------

void ae::tree::export_tree(const Tree& tree, const std::filesystem::path& filename)
{
    export_subtree(tree, tree.root(), filename);

} // ae::tree::export_tree

// ----------------------------------------------------------------------

void ae::tree::export_subtree(const Tree& tree, node_index_t root, const std::filesystem::path& filename)
{
    const auto parent = tree.parent(root);
    fmt::print(stderr, ">>>> export_subtree {} parent: {}\n", root, parent);
    export_subtree(tree, tree.inode(parent), filename);

} // ae::tree::export_subtree

// ----------------------------------------------------------------------

void ae::tree::export_subtree(const Tree& tree, const Inode& root, const std::filesystem::path& filename)
{
    Timeit ti{"export (sub)tree", std::chrono::milliseconds{100}};
    using namespace std::string_view_literals;
    const auto has_suffix = [filename = filename.filename().native()](std::initializer_list<std::string_view> suffixes) {
        return std::any_of(std::begin(suffixes), std::end(suffixes), [&filename](std::string_view suffix) { return filename.find(suffix) != std::string::npos; });
    };

    std::string data;
    if (has_suffix({".newick"sv}))
        data = export_newick(tree, root);
    else if (has_suffix({".json"sv, ".tjz"sv}) || filename.native() == "=")
        data = export_json(tree, root);
    else if (filename.native() == "-" || has_suffix({".txt"sv, ".text"sv}))
        data = export_text(tree, root);
    else
        throw std::runtime_error{AD_FORMAT("cannot export (sub)tree to \"{}\": unknown file format", filename)};
    file::write(filename, data);

} // ae::tree::export_subtree

// ----------------------------------------------------------------------
