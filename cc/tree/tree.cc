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
        max_cumulative = 0.0;
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
                        inode->number_of_leaves = 0;
                        for (const auto child_id : inode->children)
                            node(child_id).visit([inode](const Inode* sub_inode) { inode->number_of_leaves += sub_inode->number_of_leaves; }, [inode](const Leaf*) { ++inode->number_of_leaves; });
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
                inode->number_of_leaves = 0;
                for (const auto child_id : inode->children)
                    node(child_id).visit([inode](const Inode* sub_inode) { inode->number_of_leaves += sub_inode->number_of_leaves; }, [inode](const Leaf*) { ++inode->number_of_leaves; });
            },
            [](const Leaf*) {});
    }

} // ae::tree::Tree::update_number_of_leaves_in_subtree

// ----------------------------------------------------------------------

void ae::tree::Tree::set_node_id()
{
    node_index_t inode_id{0};
    for (auto inode = inodes_.begin(); inode != inodes_.end(); ++inode, --inode_id)
        inode->node_id_ = inode_id;
    node_index_t leaf_id{0};
    for (auto leaf = leaves_.begin(); leaf != leaves_.end(); ++leaf, ++leaf_id)
        leaf->node_id_ = leaf_id;

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
                              for (const auto ref_for_hash : refs_for_hash) {
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
        if (Leaf* leaf = leaf_ref.leaf(); !leaf->aa.empty() || !leaf->nuc.empty()) {
            const auto clades_for_leaf = clades.clades(leaf->aa, leaf->nuc, subtype_, lineage_);
            fmt::print(">>>> \"{}\" {}\n", leaf->name, clades_for_leaf);
        }
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

    // normally just one entry in the collection, but there could be meore in complcated cases: multiple single_child_inodes (direct and indirect children) for the same parent
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

std::shared_ptr<ae::tree::Tree> ae::tree::load(const std::filesystem::path& filename)
{
    std::shared_ptr<ae::tree::Tree> tree;
    const auto data = file::read(filename, ::simdjson::SIMDJSON_PADDING);
    if (is_newick(data))
        tree = load_newick(data);
    else
        throw std::runtime_error{fmt::format("cannot load tree from \"{}\": unknown file format", filename)};
    tree->calculate_cumulative();
    tree->set_node_id();
    return tree;

} // ae::tree::load

// ----------------------------------------------------------------------

void ae::tree::export_tree(const Tree& tree, const std::filesystem::path& filename)
{
    Timeit ti{"export_tree", std::chrono::milliseconds{100}};
    using namespace std::string_view_literals;
    const auto has_suffix = [filename = filename.filename().native()](std::initializer_list<std::string_view> suffixes) {
        return std::any_of(std::begin(suffixes), std::end(suffixes), [&filename](std::string_view suffix) { return filename.find(suffix) != std::string::npos; });
    };

    std::string data;
    if (has_suffix({".newick"sv}))
        data = export_newick(tree);
    else if (has_suffix({".json"sv, ".tjz"sv}))
        data = export_json(tree);
    else if (filename.native() == "-" || has_suffix({".txt"sv, ".text"sv}))
        data = export_text(tree);
    else
        throw std::runtime_error{fmt::format("cannot export tree to \"{}\": unknown file format", filename)};
    file::write(filename, data);

} // ae::tree::export_tree

// ----------------------------------------------------------------------
