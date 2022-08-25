#pragma once

#include <unordered_set>

#include "ext/filesystem.hh"
#include "virus/type-subtype.hh"
#include "sequences/sequence.hh"
#include "sequences/lineage.hh"
#include "sequences/clades.hh"
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
        sequences::sequence_aa_t aa{};
        sequences::sequence_nuc_t nuc{};
        std::vector<std::string> aa_transitions{};
        std::vector<std::string> nuc_transitions{};

        template <typename Seq> const Seq& get_aa_nuc() const
        {
            if constexpr (std::is_same_v<Seq, ae::sequences::sequence_aa_t>)
                return aa;
            else
                return nuc;
        }
    };

    struct Leaf : public Node
    {
        Leaf() = default;
        Leaf(std::string_view a_name, EdgeLength a_edge) : Node{.name{std::string{a_name}}, .edge{a_edge}} {} // g++-11 wants std::string{a_name}
        Leaf(std::string_view a_name, EdgeLength a_edge, EdgeLength a_cumulative_edge) : Node{.name{std::string{a_name}}, .edge{a_edge}, .cumulative_edge{a_cumulative_edge}} {}

        size_t number_of_leaves() const { return 1; }

        bool shown{true};
        std::string date{};
        std::string continent{};
        std::string country{};
        sequences::clades_t clades{};
    };

    // struct transition_t
    // {
    //     char left;
    //     sequences::pos1_t pos;
    //     char right;
    // };

    struct Inode : public Node
    {
        size_t number_of_leaves() const { return number_of_leaves_; }

        std::vector<node_index_t> children{};
        size_t number_of_leaves_{0};

        // temporary data for raxml ancestral state reconstruction
        std::unordered_set<std::string> raxml_inode_names{};
    };

    struct Nodes
    {
        std::vector<node_index_t> nodes{};
        Tree& tree;

        Nodes& sort_by_cumulative();
        Nodes& filter_by_cumulative_more_than(double min_cumulative);
        Nodes& filter_seq_id(const std::vector<std::string>& seq_ids);
        Nodes& remove();
    };

    // ----------------------------------------------------------------------

    class Tree
    {
      public:
        const Inode& root() const { return inodes_[0]; }
        Inode& root() { return inodes_[0]; }
        static node_index_t root_index() { return node_index_t{0}; }

        const auto& subtype() const { return subtype_; }
        const auto& lineage() const { return lineage_; }

        const Inode& inode(node_index_t index) const { return inodes_[static_cast<size_t>(-*index)]; }
        Inode& inode(node_index_t index) { return inodes_[static_cast<size_t>(-*index)]; }
        const Leaf& leaf(node_index_t index) const { return leaves_[static_cast<size_t>(*index)]; }
        Leaf& leaf(node_index_t index) { return leaves_[static_cast<size_t>(*index)]; }
        node_index_t first_leaf(node_index_t index) const;

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

        // Node* node_base(node_index_t index)
        // {
        //     if (is_leaf(index))
        //         return &leaf(index);
        //     else
        //         return &inode(index);
        // }

        node_index_t parent(node_index_t child) const;

        // parent==node_index_t{0} means adding to the root
        std::pair<node_index_t, Inode&> add_inode(node_index_t parent);
        std::pair<node_index_t, Leaf&> add_leaf(node_index_t parent);
        std::pair<node_index_t, Leaf&> add_leaf(node_index_t parent, std::string_view name, EdgeLength edge);

        size_t depth() const; // max nesting level
        EdgeLength calculate_cumulative(bool force = false);
        EdgeLength maximum_cumulative() const { return max_cumulative; }

        auto visit(tree_visiting visiting) const { return const_tree_visitor{*this, visiting}; }
        auto visit(tree_visiting visiting) { return tree_visitor{*this, visiting}; }
        auto visit_all() const { return visit(tree_visiting::all); }
        auto visit_all() { return visit(tree_visiting::all); }

        enum class reset_node_id { no, yes };
        void set_node_id(reset_node_id reset);
        void populate_with_sequences(const virus::type_subtype_t& subtype);
        void populate_with_duplicates(const virus::type_subtype_t& subtype);
        void set_clades(const std::filesystem::path& clades_json_file);

        Nodes select_all();
        Nodes select_leaves();
        Nodes select_inodes();
        Nodes select_inodes_with_just_one_child();

        // unlink passed nodes
        // if a parent inode has no children afterwards, unlink it too
        void remove(const std::vector<node_index_t>& nodes);
        void remove_leaves_isolated_before(std::string_view date, const std::vector<std::string>& important);

        void subtype(const virus::type_subtype_t& subtype) { subtype_ = subtype; }
        void lineage(const sequences::lineage_t& lineage) { lineage_ = lineage; }
        void update_number_of_leaves_in_subtree();
        size_t number_of_leaves() const { return root().number_of_leaves(); }

        std::vector<std::string> fix_names_by_seqdb(const virus::type_subtype_t& subtype);

        enum class ladderize_method { none, number_of_leaves, max_edge_length };
        void ladderize(ladderize_method method);

        void set_raxml_ancestral_state_reconstruction_data(const std::filesystem::path& raxml_tree_file, const std::filesystem::path& raxml_states_file);

      private:
        virus::type_subtype_t subtype_{};
        sequences::lineage_t lineage_{};
        std::vector<Inode> inodes_{Inode{}}; // root is always there
        std::vector<Leaf> leaves_{Leaf{}};   // first leaf is unused, node_index_t{0} is index of root inode
        mutable size_t depth_{0};
        mutable EdgeLength max_cumulative{-1.0};

        template <lvalue_reference TREE, pointer LEAF, pointer INODE, pointer NODE> friend class tree_iterator_t;

        void set_inode_sequences_if_no_ancestral_data();
        void set_transition_labels_by_raxml_ancestral_state_reconstruction_data();
        void check_transition_label_flip();
    };

    // ----------------------------------------------------------------------

    std::shared_ptr<Tree> load(const std::filesystem::path& filename);
    void export_tree(const Tree& tree, const std::filesystem::path& filename);
    void export_subtree(const Tree& tree, node_index_t root, const std::filesystem::path& filename);

    // ----------------------------------------------------------------------

    inline Nodes& Nodes::remove()
    {
        tree.remove(nodes);
        return *this;
    }

} // namespace ae::tree

// ======================================================================
