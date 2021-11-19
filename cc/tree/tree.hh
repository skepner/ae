#pragma once

#include <variant>

#include "ext/filesystem.hh"
#include "utils/named-type.hh"
#include "sequences/sequence.hh"
#include "tree/tree.hh"

// ======================================================================

namespace ae::tree
{
    using EdgeLength = named_double_t<struct tree_EdgeLength_tag>;
    using node_index_base_t = int;                                                      //  signed
    using node_index_t = named_number_t<node_index_base_t, struct tree_node_index_tag>; // signed! positive - leaves_, negative - inodes_, zero - root inode

    inline bool is_leaf(node_index_t index) { return index > 0; }

    // ----------------------------------------------------------------------

    struct Node
    {
        std::string name{};
        EdgeLength edge{0};
        EdgeLength cumulative_edge{0};
    };

    struct Leaf : public Node
    {
        Leaf() = default;
        Leaf(std::string_view a_name, EdgeLength a_edge) : Node{.name{a_name}, .edge{a_edge}} {}

        bool shown{true};
        std::string date;
        std::string continent;
        std::string country;
        sequences::sequence_aa_t aa;
        sequences::sequence_nuc_t nuc;
        std::vector<std::string> clades;
    };

    struct Inode : public Node
    {
        std::vector<node_index_t> children;
        // std::vector<std::string> aa_substs;
    };

    // ----------------------------------------------------------------------

    class Tree;

    class tree_iterator
    {
      public:
        using reference = std::variant<const Leaf*, const Inode*>;

        tree_iterator(const Tree& tree, node_index_t node_index);
        tree_iterator& operator++();
        reference operator*();
        bool operator==(const tree_iterator& rhs) const { return &tree_ == &rhs.tree_ && node_index_ == rhs.node_index_; }

      private:
        const Tree& tree_;
        node_index_t node_index_;
        std::vector<node_index_t> parents_;
    };

    // ----------------------------------------------------------------------

    class Tree
    {
      public:
        const Inode& root() const { return inodes_[0]; }
        Inode& root() { return inodes_[0]; }
        static node_index_t root_index() { return node_index_t{0}; }

        const Inode& inode(node_index_t index) const { return inodes_[-*index]; }
        Inode& inode(node_index_t index) { return inodes_[-*index]; }
        const Leaf& leaf(node_index_t index) const { return leaves_[*index]; }
        Leaf& leaf(node_index_t index) { return leaves_[*index]; }

        // parent==node_index_t{0} means adding to the root
        std::pair<node_index_t, Inode&> add_inode(node_index_t parent);
        std::pair<node_index_t, Leaf&> add_leaf(node_index_t parent, std::string_view name, EdgeLength edge);

        size_t depth() const; // max nesting level
        EdgeLength calculate_cumulative() const;

        tree_iterator begin() const { return {*this, node_index_t{0}}; }
        tree_iterator end() const { return {*this, node_index_t{-static_cast<node_index_base_t>(inodes_.size())}}; }

      private:
        std::vector<Inode> inodes_{Inode{}}; // root is always there
        std::vector<Leaf> leaves_{Leaf{}};   // first leaf is unused, node_index_t{0} is index of root inode
        mutable size_t depth_{0};
        mutable EdgeLength max_cumulative{-1.0};

        friend class tree_iterator;
    };

    // ----------------------------------------------------------------------

    std::shared_ptr<Tree> load(const std::filesystem::path& filename);
    void export_tree(const Tree& tree, const std::filesystem::path& filename);

    // ----------------------------------------------------------------------

    inline tree_iterator::tree_iterator(const Tree& tree, node_index_t node_index) : tree_{tree}, node_index_{node_index}
    {
        parents_.reserve(tree.depth());
        if (node_index == node_index_t{0})
            parents_.push_back(node_index_t{0});
    }

    inline tree_iterator& tree_iterator::operator++() { return *this; }

    inline tree_iterator::reference tree_iterator::operator*() { return &tree_.inodes_[0]; }

} // namespace ae::tree

// ======================================================================
