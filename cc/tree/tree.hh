#pragma once

#include <charconv>
#include <unordered_set>
#include <memory>

#include "ext/filesystem.hh"
#include "fmt/core.h"
#include "utils/counter.hh"
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
        bool shown{true};

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

        std::string date{};
        std::string continent{};
        std::string country{};
        sequences::clades_t clades{};
    };

    struct transition_t
    {
        transition_t(char a_left, sequences::pos1_t a_pos, char a_right) : left{a_left}, pos{a_pos}, right{a_right} {}
        transition_t(std::string_view source) : left{source.front()}, pos{ae::from_chars<size_t>(source.substr(1, source.size() - 2))}, right{source.back()} {}
        char left{' '};
        sequences::pos1_t pos{999999};
        char right{' '};
    };

    struct transitions_t
    {
        std::vector<transition_t> transitions{};

        bool empty() const { return transitions.empty(); }
        void clear() { transitions.clear(); }
        void add(char left, sequences::pos1_t pos, char right) { transitions.emplace_back(left, pos, right); }
        void add(sequences::pos1_t pos, char right) { transitions.emplace_back(' ', pos, right); }
        void add(std::string_view source) { transitions.emplace_back(source); }
    };

    struct Inode : public Node
    {
        Inode() = default;
        Inode(const Inode&) = delete;
        Inode(Inode&&) = default;
        Inode& operator=(const Inode&) = delete;
        Inode& operator=(Inode&&) = default;

        size_t number_of_leaves() const { return number_of_leaves_; }

        std::vector<node_index_t> children{};
        size_t number_of_leaves_{0};

        transitions_t aa_transitions{};
        transitions_t nuc_transitions{};

        // temporary data for raxml ancestral state reconstruction
        std::unordered_set<std::string> raxml_inode_names{};

        // aa transition labels
        using counter_aa_t = counter_char_t<'-', '[', unsigned>;
        std::unique_ptr<counter_aa_t> common_aa{};
        void reset_common_aa()
        {
            if (common_aa)
                common_aa->reset();
            else
                common_aa = std::make_unique<counter_aa_t>();
        }
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
        Tree()
        {
            inodes_.emplace_back(); // root is always there
            leaves_.emplace_back(); // first leaf is unused, node_index_t{0} is index of root inode
        }

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
        node_index_t first_immediate_child_leaf(node_index_t index) const;  // throws if no immediate leaves found

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

        // longest aa, longest nuc
        std::pair<sequences::pos0_t, sequences::pos0_t> longest_sequence() const;

      private:
        virus::type_subtype_t subtype_{};
        sequences::lineage_t lineage_{};
        std::vector<Inode> inodes_{};
        std::vector<Leaf> leaves_{};
        mutable size_t depth_{0};
        mutable EdgeLength max_cumulative{-1.0};

        template <lvalue_reference TREE, pointer LEAF, pointer INODE, pointer NODE> friend class tree_iterator_t;

        void set_inode_sequences_if_no_ancestral_data();
        void set_transition_labels_by_raxml_ancestral_state_reconstruction_data();
        void check_transition_label_flip();
    };

    // ----------------------------------------------------------------------

    std::shared_ptr<Tree> load(const std::filesystem::path& filename);
    void load_subtree(const std::filesystem::path& filename, Tree& tree, node_index_t join_at);

    void export_tree(const Tree& tree, const std::filesystem::path& filename, size_t indent = 0);
    void export_subtree(const Tree& tree, node_index_t root, const std::filesystem::path& filename, size_t indent = 0);
    void export_subtree(const Tree& tree, const Inode& root, const std::filesystem::path& filename, size_t indent = 0);

    // ----------------------------------------------------------------------

    inline Nodes& Nodes::remove()
    {
        tree.remove(nodes);
        return *this;
    }

} // namespace ae::tree

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::tree::transition_t> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    auto format(const ae::tree::transition_t& tr, format_context& ctx) const { return fmt::format_to(ctx.out(), "{}{}{}", tr.left, tr.pos, tr.right); }
};

// "{}" - format all
// "{:3} - format 3 most important or all if total number of aa transitions <= 3
template <> struct fmt::formatter<ae::tree::transitions_t> : fmt::formatter<ae::fmt_helper::default_formatter> // fmt::formatter<std::string>
{

    // template <typename ParseContext> constexpr auto parse(ParseContext& ctx)
    // {
    //     auto it = ctx.begin();
    //     if (it != ctx.end() && *it == ':')
    //         ++it;
    //     if (it != ctx.end() && *it != '}') {
    //         char* end{nullptr};
    //         most_important_ = std::strtoul(&*it, &end, 10);
    //         it = std::next(it, end - &*it);
    //     }
    //     return std::find(it, ctx.end(), '}');
    // }

    auto format(const ae::tree::transitions_t& tr, format_context& ctx) const { return fmt::format_to(ctx.out(), "{}", fmt::join(tr.transitions, " ")); }

  private:
    // size_t most_important_{0};
};

// ======================================================================
