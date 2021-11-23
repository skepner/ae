#pragma once

#include "ext/filesystem.hh"
#include "virus/type-subtype.hh"
#include "sequences/sequence.hh"
#include "sequences/lineage.hh"
#include "tree/tree-iterator.hh"

// ======================================================================

namespace ae::tree
{
    class Tree;

    using EdgeLength = named_double_t<struct tree_EdgeLength_tag>;

    inline bool is_leaf(node_index_t index) { return index > 0; }

    // ----------------------------------------------------------------------

    struct Node
    {
        std::string name{};
        EdgeLength edge{0};
        EdgeLength cumulative_edge{0};
        node_index_t node_id_{0};
    };

    struct Leaf : public Node
    {
        Leaf() = default;
        Leaf(std::string_view a_name, EdgeLength a_edge) : Node{.name{a_name}, .edge{a_edge}} {}
        Leaf(std::string_view a_name, EdgeLength a_edge, EdgeLength a_cumulative_edge) : Node{.name{a_name}, .edge{a_edge}, .cumulative_edge{a_cumulative_edge}} {}

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
        size_t number_of_leaves{0};
        // std::vector<std::string> aa_substs;
    };

    struct Nodes
    {
        std::vector<node_index_t> nodes;
        Tree& tree;

        Nodes& sort_by_cumulative();
        Nodes& filter_by_cumulative_more_than(double min_cumulative);
        Nodes& remove();
    };

    // ----------------------------------------------------------------------

    class Tree
    {
      public:
        const Inode& root() const { return inodes_[0]; }
        Inode& root() { return inodes_[0]; }
        static node_index_t root_index() { return node_index_t{0}; }

        constexpr const auto& subtype() const { return subtype_; }
        constexpr const auto& lineage() const { return lineage_; }

        const Inode& inode(node_index_t index) const { return inodes_[-*index]; }
        Inode& inode(node_index_t index) { return inodes_[-*index]; }
        const Leaf& leaf(node_index_t index) const { return leaves_[*index]; }
        Leaf& leaf(node_index_t index) { return leaves_[*index]; }

        const_tree_iterator::reference node(node_index_t index) const
        {
            if (is_leaf(index))
                return &leaf(index);
            else
                return &inode(index);
        }

        tree_iterator::reference node(node_index_t index)
        {
            if (is_leaf(index))
                return &leaf(index);
            else
                return &inode(index);
        }

        // parent==node_index_t{0} means adding to the root
        std::pair<node_index_t, Inode&> add_inode(node_index_t parent);
        std::pair<node_index_t, Leaf&> add_leaf(node_index_t parent, std::string_view name, EdgeLength edge);

        size_t depth() const; // max nesting level
        EdgeLength calculate_cumulative(bool force = false);
        EdgeLength maximum_cumulative() const { return max_cumulative; }

        auto visit(tree_visiting visiting) const { return const_tree_visitor{*this, visiting}; }
        auto visit(tree_visiting visiting) { return tree_visitor{*this, visiting}; }
        auto visit_all() const { return visit(tree_visiting::all); }
        auto visit_all() { return visit(tree_visiting::all); }

        void set_node_id();
        void populate_with_sequences(const virus::type_subtype_t& subtype);
        void populate_with_duplicates(const virus::type_subtype_t& subtype);

        Nodes select_all();
        Nodes select_leaves();
        Nodes select_inodes();

        // unlink passed nodes
        // if a parent inode has no children afterwards, unlink it too
        void remove(const std::vector<node_index_t>& nodes);

      private:
        virus::type_subtype_t subtype_;
        sequences::lineage_t lineage_;
        std::vector<Inode> inodes_{Inode{}}; // root is always there
        std::vector<Leaf> leaves_{Leaf{}};   // first leaf is unused, node_index_t{0} is index of root inode
        mutable size_t depth_{0};
        mutable EdgeLength max_cumulative{-1.0};

        void update_number_of_leaves_in_subtree();

        template <lvalue_reference TREE, pointer LEAF, pointer INODE> friend class tree_iterator_t;
    };

    // ----------------------------------------------------------------------

    std::shared_ptr<Tree> load(const std::filesystem::path& filename);
    void export_tree(const Tree& tree, const std::filesystem::path& filename);

    // ----------------------------------------------------------------------

    inline Nodes& Nodes::remove()
    {
        tree.remove(nodes);
        return *this;
    }

} // namespace ae::tree

// ======================================================================
