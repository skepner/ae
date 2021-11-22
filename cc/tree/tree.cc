#include "ext/simdjson.hh"
#include "sequences/seqdb.hh"
#include "tree/tree.hh"
#include "tree/newick.hh"
#include "tree/text-export.hh"

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
        fmt::print(">>>> tree depth {}\n", depth_);
    }
    return depth_;

} // ae::tree::Tree::depth

// ----------------------------------------------------------------------

ae::tree::EdgeLength ae::tree::Tree::calculate_cumulative()
{
    if (max_cumulative < EdgeLength{0}) {
        EdgeLength cumulative{0.0};
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
    const auto& seqdb = sequences::seqdb_for_subtype(subtype);
    for (auto leaf = leaves_.begin() + 1; leaf != leaves_.end(); ++leaf) {
        if (const auto ref = seqdb.find_by_seq_id(sequences::seq_id_t{leaf->name}, sequences::Seqdb::set_master::yes); ref) {
            leaf->aa = ref.aa();
            leaf->nuc = ref.nuc();
            leaf->date = ref.entry->date();
            leaf->continent = ref.entry->continent;
            leaf->country = ref.entry->country;
        }
        else
            fmt::print(">> [seqdb {}] seq_id not found in seqdb: \"{}\"\n", subtype, leaf->name);
    }

} // ae::tree::Tree::populate_with_sequences

// ----------------------------------------------------------------------

void ae::tree::Tree::populate_with_duplicates(const virus::type_subtype_t& subtype)
{
    fmt::print(">> Tree::populate_with_duplicates no implemented\n");

    const auto& seqdb = sequences::seqdb_for_subtype(subtype);

    for (auto ref : visit(tree_visiting::all_post)) {
        ref.visit(
            [&seqdb, &subtype](const Leaf* leaf) {
                if (const auto present_ref = seqdb.find_by_seq_id(sequences::seq_id_t{leaf->name}, sequences::Seqdb::set_master::yes); present_ref) {
                    const auto refs = seqdb.find_all_by_hash(present_ref.seq->hash);
                    fmt::print(">>>> \"{}\" duplicates: {}\n", leaf->name, refs.size());
                }
                else
                    fmt::print(">> [seqdb {}] seq_id not found in seqdb: \"{}\"\n", subtype, leaf->name);
            },
            [this](Inode* inode) {
                // cumulative -= inode->edge;
                // // post -> update number_of_leaves
                // inode->number_of_leaves = 0;
                // for (const auto child_id : inode->children)
                //     node(child_id).visit([inode](const Inode* sub_inode) { inode->number_of_leaves += sub_inode->number_of_leaves; }, [inode](const Leaf*) { ++inode->number_of_leaves; });
            });
    }

} // ae::tree::Tree::populate_with_duplicates

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
