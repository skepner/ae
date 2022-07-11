#pragma once

#include <variant>
#include <string>
#include <vector>
#include <limits>

#include "ext/fmt.hh"
#include "utils/named-type.hh"
#include "utils/overload.hh"
#include "utils/concepts.hh"

// ======================================================================

namespace ae::tree
{
    using node_index_base_t = int;                                                      //  signed
    using node_index_t = named_number_t<node_index_base_t, struct tree_node_index_tag>; // signed! positive - leaves_, negative - inodes_, zero - root inode

    inline const node_index_t node_unlinked{std::numeric_limits<node_index_base_t>::max()}; // see Tree::remove

    class Tree;
    struct Node;
    struct Leaf;
    struct Inode;
    template <lvalue_reference TREE, pointer LEAF, pointer INODE, pointer NODE> class tree_iterator_t;

    template <lvalue_reference TREE, pointer LEAF, pointer INODE, pointer NODE> class tree_iterator_refence_t
    {
      public:
        constexpr bool pre() const { return pre_; }
        constexpr bool post() const { return !pre_; }

        bool is_root() const
        {
            return std::visit(overload{[](LEAF) { return false; }, [](INODE inode) { return inode->node_id_ == 0; }}, ref_);
        }

        node_index_t node_id() const
        {
            return std::visit(overload{[](auto* node) { return node->node_id_; }}, ref_);
        }

        std::string to_string() const
        {
            return std::visit(overload{[](LEAF leaf) { return fmt::format("<{}> \"{}\"", leaf->node_id_, leaf->name); },
                                       [this](INODE inode) { return fmt::format("<{} {}> children:{}", inode->node_id_, pre() ? "pre" : "post", inode->children.size()); }},
                              ref_);
        }

        template <typename... Callbacks> auto visit(Callbacks&&... callbacks) { return std::visit(overload{std::forward<Callbacks>(callbacks)...}, ref_); }
        template <typename... Callbacks> auto visit(Callbacks&&... callbacks) const { return std::visit(overload{std::forward<Callbacks>(callbacks)...}, ref_); }

        LEAF leaf()
        {
            return std::visit(overload{[](LEAF leaf) -> LEAF { return leaf; }, [](INODE) -> LEAF { return nullptr; }}, ref_);
        }

        INODE inode()
        {
            return std::visit(overload{[](LEAF) -> INODE { return nullptr; }, [](INODE inode) -> INODE { return inode; }}, ref_);
        }

        NODE node()
        {
            return std::visit([](auto* node) -> NODE { return node; }, ref_);
        }

        NODE node() const
        {
            return std::visit([](auto* node) -> NODE { return node; }, ref_);
        }

      private:
        std::variant<LEAF, INODE> ref_;
        bool pre_{true};

        constexpr tree_iterator_refence_t(LEAF leaf) : ref_{leaf} {}
        constexpr tree_iterator_refence_t(INODE inode, bool a_pre = true) : ref_{inode}, pre_{a_pre} {}

        friend class tree_iterator_t<TREE, LEAF, INODE, NODE>;
        friend std::decay_t<TREE>;
    };

    enum class tree_visiting { all, leaves, inodes, inodes_post, all_pre_post, all_post };

    template <lvalue_reference TREE, pointer LEAF, pointer INODE, pointer NODE> class tree_iterator_t
    {
      public:
        enum _init_end { init_end };

        using reference = tree_iterator_refence_t<TREE, LEAF, INODE, NODE>;

        tree_iterator_t(TREE tree, tree_visiting a_visiting);
        tree_iterator_t(TREE tree, tree_visiting a_visiting, _init_end);
        tree_iterator_t& operator++();
        reference operator*();
        bool operator==(const tree_iterator_t& rhs) const { return &tree_ == &rhs.tree_ && parents_.back() == rhs.parents_.back(); }

      private:
        TREE tree_;
        tree_visiting visiting_;
        std::vector<std::pair<node_index_t, size_t>> parents_{}; // parent and index in tree.inode(parents_.back()).children, index=-1 for tree.inode(parents_.back()) itself

        constexpr static size_t parent_itself{static_cast<size_t>(-1)};
        constexpr static size_t iteration_end{static_cast<size_t>(-2)};

        bool is_visiting_post() const
        {
            switch (visiting_) {
                case tree_visiting::all:
                case tree_visiting::inodes:
                case tree_visiting::leaves:
                    return false;
                case tree_visiting::all_pre_post:
                case tree_visiting::all_post:
                case tree_visiting::inodes_post:
                    return true;
            }
            return false;       // g++-11
        }
    };

    extern template class tree_iterator_t<Tree&, Leaf*, Inode*, Node*>;
    extern template class tree_iterator_t<const Tree&, const Leaf*, const Inode*, const Node*>;

    using tree_iterator = tree_iterator_t<Tree&, Leaf*, Inode*, Node*>;
    using const_tree_iterator = tree_iterator_t<const Tree&, const Leaf*, const Inode*, const Node*>;

    template <lvalue_reference TREE, pointer LEAF, pointer INODE, pointer NODE> class tree_visitor_t
    {
      public:
        using iterator = tree_iterator_t<TREE, LEAF, INODE, NODE>;

        constexpr tree_visitor_t(TREE tree, tree_visiting a_visiting) : tree_{tree}, visiting_{a_visiting} {}

        iterator begin() const { return iterator(tree_, visiting_); }
        iterator end() const { return iterator(tree_, visiting_, iterator::init_end); }

      private:
        TREE tree_;
        tree_visiting visiting_;
    };

    using tree_visitor = tree_visitor_t<Tree&, Leaf*, Inode*, Node*>;
    using const_tree_visitor = tree_visitor_t<const Tree&, const Leaf*, const Inode*, const Node*>;

} // namespace ae::tree

// ----------------------------------------------------------------------

// template <> struct fmt::formatter<ae::tree::node_index_t> : fmt::formatter<ae::fmt_helper::default_formatter> {
//     template <typename FormatCtx> constexpr auto format(const ae::tree::node_index_t& nind, FormatCtx& ctx) const
//     {
//         return format_to(ctx.out(), "<{}>", nind.get());
//     }
// };

// ======================================================================
