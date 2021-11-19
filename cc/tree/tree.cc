#include "tree/tree.hh"
#include "ext/simdjson.hh"
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

ae::tree::EdgeLength ae::tree::Tree::calculate_cumulative() const
{
    if (max_cumulative < EdgeLength{0}) {
        for (const auto ref : *this) {
            fmt::print(">>>> calculate_cumulative ref\n");
        }
    }
    return max_cumulative;

} // ae::tree::Tree::calculate_cumulative

// ----------------------------------------------------------------------

std::shared_ptr<ae::tree::Tree> ae::tree::load(const std::filesystem::path& filename)
{
    const auto data = file::read(filename, ::simdjson::SIMDJSON_PADDING);
    if (is_newick(data))
        return load_newick(data);
    else
        throw std::runtime_error{fmt::format("cannot load tree from \"{}\": unknown file format", filename)};

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
    else if (filename.native() == "-" || has_suffix({".txt"sv, ".text"sv}))
        data = export_text(tree);
    else
        throw std::runtime_error{fmt::format("cannot export tree to \"{}\": unknown file format", filename)};
    file::write(filename, data);

} // ae::tree::export_tree

// ----------------------------------------------------------------------
