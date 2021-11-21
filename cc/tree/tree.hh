#pragma once

#include <variant>

#include "ext/filesystem.hh"
#include "utils/named-type.hh"
#include "utils/overload.hh"
#include "utils/concepts.hh"
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
        node_index_t node_id_{0};
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

    template <pointer LEAF, pointer INODE> class tree_iterator_refence_t
    {
      public:
        tree_iterator_refence_t(LEAF leaf) : ref_{leaf} {}
        tree_iterator_refence_t(INODE inode) : ref_{inode} {}

        std::string to_string() const
        {
            return std::visit(overload{[](LEAF leaf) { return fmt::format("<{}> \"{}\"", leaf->node_id_, leaf->name); },
                                       [](INODE inode) { return fmt::format("<{}> children:{}", inode->node_id_, inode->children.size()); }},
                              ref_);
        }

      private:
        std::variant<LEAF, INODE> ref_;
    };

    enum class tree_visiting { all, leaves, inodes, inodes_post, all_post };

    template <lvalue_reference TREE, pointer LEAF, pointer INODE> class tree_iterator_t
    {
      public:
        enum _init_end { init_end };

        using reference = tree_iterator_refence_t<LEAF, INODE>;

        tree_iterator_t(TREE tree, tree_visiting a_visiting);
        tree_iterator_t(TREE tree, tree_visiting a_visiting, _init_end);
        tree_iterator_t& operator++();
        reference operator*();
        bool operator==(const tree_iterator_t& rhs) const { return &tree_ == &rhs.tree_ && parents_.back() == rhs.parents_.back(); }

      private:
        TREE tree_;
        tree_visiting visiting_;
        std::vector<std::pair<node_index_t, size_t>> parents_; // parent and index in tree.inode(parents_.back()).children, index=-1 for tree.inode(parents_.back()) itself

        constexpr static size_t parent_itself{static_cast<size_t>(-1)};
        constexpr static size_t iteration_end{static_cast<size_t>(-2)};

        bool is_visiting_post() const
        {
            switch (visiting_) {
                case tree_visiting::all:
                case tree_visiting::inodes:
                case tree_visiting::leaves:
                    return false;
                case tree_visiting::all_post:
                case tree_visiting::inodes_post:
                    return true;
            }
        }
    };

    using tree_iterator = tree_iterator_t<Tree&, Leaf*, Inode*>;
    using const_tree_iterator = tree_iterator_t<const Tree&, const Leaf*, const Inode*>;

    template <lvalue_reference TREE, pointer LEAF, pointer INODE> class tree_visitor_t
    {
      public:
        using iterator = tree_iterator_t<TREE, LEAF, INODE>;

        constexpr tree_visitor_t(TREE tree, tree_visiting a_visiting) : tree_{tree}, visiting_{a_visiting} {}

        iterator begin() const { return iterator(tree_, visiting_); }
        iterator end() const { return iterator(tree_, visiting_, iterator::init_end); }

      private:
        TREE tree_;
        tree_visiting visiting_;
    };

    using tree_visitor = tree_visitor_t<Tree&, Leaf*, Inode*>;
    using const_tree_visitor = tree_visitor_t<const Tree&, const Leaf*, const Inode*>;

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

        const_tree_iterator::reference node(node_index_t index) const
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
        EdgeLength calculate_cumulative() const;

        auto visit(tree_visiting visiting) const { return const_tree_visitor{*this, visiting}; }
        auto visit(tree_visiting visiting) { return tree_visitor{*this, visiting}; }
        auto visit_all() const { return visit(tree_visiting::all); }
        auto visit_all() { return visit(tree_visiting::all); }

        void set_node_id();

      private:
        std::vector<Inode> inodes_{Inode{}}; // root is always there
        std::vector<Leaf> leaves_{Leaf{}};   // first leaf is unused, node_index_t{0} is index of root inode
        mutable size_t depth_{0};
        mutable EdgeLength max_cumulative{-1.0};

        template <lvalue_reference TREE, pointer LEAF, pointer INODE> friend class tree_iterator_t;
    };

    // ----------------------------------------------------------------------

    std::shared_ptr<Tree> load(const std::filesystem::path& filename);
    void export_tree(const Tree& tree, const std::filesystem::path& filename);

    // ----------------------------------------------------------------------

    // begin
    template <lvalue_reference TREE, pointer LEAF, pointer INODE> inline tree_iterator_t<TREE, LEAF, INODE>::tree_iterator_t(TREE tree, tree_visiting a_visiting) : tree_{tree}, visiting_{a_visiting}
    {
        parents_.reserve(tree.depth());
        parents_.emplace_back(node_index_t{0}, parent_itself);
        switch (visiting_) {
            case tree_visiting::all:
            case tree_visiting::inodes:
                break;
            case tree_visiting::leaves:
            case tree_visiting::all_post:
            case tree_visiting::inodes_post:
                operator++();
                break;
        }
    }

    // end
    template <lvalue_reference TREE, pointer LEAF, pointer INODE>
    inline tree_iterator_t<TREE, LEAF, INODE>::tree_iterator_t(TREE tree, tree_visiting a_visiting, _init_end) : tree_{tree}, visiting_{a_visiting}
    {
        parents_.emplace_back(node_index_t{0}, iteration_end);
    }

    template <lvalue_reference TREE, pointer LEAF, pointer INODE> inline tree_iterator_t<TREE, LEAF, INODE>& tree_iterator_t<TREE, LEAF, INODE>::operator++()
    {
        // returns if suitable node found after diving
        const auto dive = [this](size_t child_no) -> bool {
            auto* parent = &parents_.back();
            parent->second = child_no;
            if (child_no != parent_itself) {
                if (const auto& parent_inode = tree_.inode(parent->first); child_no >= parent_inode.children.size())
                    return is_visiting_post();
                else if (const auto child_index = parent_inode.children[child_no]; !is_leaf(child_index)) {
                    parent = &parents_.emplace_back(child_index, parent_itself);
                    child_no = parent_itself;
                }
            }
            switch (visiting_) {
                case tree_visiting::all:
                    return true;
                case tree_visiting::inodes:
                    return child_no == parent_itself;
                case tree_visiting::leaves:
                case tree_visiting::all_post:
                    return child_no != parent_itself;
                case tree_visiting::inodes_post:
                    return false;
            }
        };

        // ----------------------------------------------------------------------

        while (true) {
            auto parent = parents_.end() - 1;
            if (parent->second == parent_itself) {
                if (dive(0))
                    break;
            }
            else if (const auto& parent_inode = tree_.inode(parent->first); parent->second < parent_inode.children.size()) {
                ++parent->second;
                if (parent->second < parent_inode.children.size()) {
                    if (dive(parent->second))
                        break;
                }
                else if (is_visiting_post()) {
                    break;
                }
            }
            else if (parents_.size() == 1) { // *post -> end
                parent->second = iteration_end;
                break;
            }
            else { // undive
                parents_.pop_back();
                parent = parents_.end() - 1;
                if (parents_.size() == 1) {
                    if (is_visiting_post())
                        ++parent->second; // *post -> pre-end
                    else
                        parent->second = iteration_end; // -> end
                    break;
                }
                else if (dive(parent->second + 1))
                    break;
            }
        }
        return *this;
    }

    template <lvalue_reference TREE, pointer LEAF, pointer INODE> inline typename tree_iterator_t<TREE, LEAF, INODE>::reference tree_iterator_t<TREE, LEAF, INODE>::operator*()
    {
        auto& [parent_index, child_no] = parents_.back();
        auto& parent = tree_.inode(parent_index);
        if (child_no == parent_itself)
            return &parent;
        else if (child_no < parent.children.size())
            return tree_.node(parent.children[child_no]);
        else if (is_visiting_post() && child_no != iteration_end)
            return &parent;
        else
            throw std::runtime_error{"tree_iterator_t: derefencing at the end?"};
    }

} // namespace ae::tree

// ======================================================================
