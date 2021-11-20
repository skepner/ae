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
            return std::visit(overload{[](LEAF leaf) { return fmt::format("\"{}\"", leaf->name); }, [](INODE inode) { return fmt::format("children:{}", inode->children.size()); }}, ref_);
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
        parents_.emplace_back(node_index_t{0}, tree.root().children.size());
    }

    template <lvalue_reference TREE, pointer LEAF, pointer INODE> inline tree_iterator_t<TREE, LEAF, INODE>& tree_iterator_t<TREE, LEAF, INODE>::operator++()
    {
        // returns if suitable node found after diving
        const auto undive = [this](auto& dive_ref) -> bool {
            if (parents_.size() == 1)
                return true; // end
            parents_.pop_back();
            // fmt::print(">>>>    undive {} {} parents_.size:{}\n", *parents_.back().first, parents_.back().second, parents_.size());
            return dive_ref(parents_.back().second + 1, dive_ref);
        };

        // returns if suitable node found after diving
        const auto dive = [this, undive](size_t child_no, auto& dive_ref) -> bool {
            // fmt::print(">>>>    dive {}\n", child_no);
            auto* parent = &parents_.back();
            parent->second = child_no;
            if (child_no != parent_itself) {
                if (const auto& parent_inode = tree_.inode(parent->first); child_no >= parent_inode.children.size())
                    return undive(dive_ref);
                else if (const auto child_index = parent_inode.children[child_no]; !is_leaf(child_index))
                    parent = &parents_.emplace_back(child_index, parent_itself);
            }
            switch (visiting_) {
                case tree_visiting::all:
                    return true;
                case tree_visiting::inodes:
                    return true;
                case tree_visiting::leaves:
                    return child_no != parent_itself && is_leaf(tree_.inode(parent->first).children[parent->second]);
                case tree_visiting::all_post:
                    return true;
                case tree_visiting::inodes_post:
                    return true;
            }
        };

        while (true)
        {
            const auto [parent_index, child_no] = parents_.back();
            if (child_no == parent_itself) {
                if (dive(0, dive))
                    break;
            }
            else if (const auto& parent_inode = tree_.inode(parent_index); child_no < parent_inode.children.size()) {
                if ((child_no + 1) >= parent_inode.children.size()) {
                    if (parents_.size() > 1) {
                        if (undive(dive))
                            break;
                    }
                    else
                        break;  // end
                }
                else if (dive(child_no + 1, dive))
                    break;
            }
            else
                break;          // end
        }
        return *this;
    }

    // template <lvalue_reference TREE, pointer LEAF, pointer INODE> inline tree_iterator_t<TREE, LEAF, INODE>& tree_iterator_t<TREE, LEAF, INODE>::operator++()
    // {
    //     const auto current_index = [this]() {
    //         if (const auto [parent_index, child_no] = parents_.back(); child_no == parent_itself)
    //             return parent_index;
    //         else
    //             return tree_.inode(parent_index).children[child_no];
    //     };

    //     const auto cont_for_visiting = [this](node_index_t parent_index, size_t child_no) {
    //         switch (visiting_) {
    //             case tree_visiting::all:
    //                 return false;
    //             case tree_visiting::inodes:
    //                 return true;
    //             case tree_visiting::leaves:
    //                 return child_no == parent_itself; // || !is_leaf(tree_.inode(parent_index).children[child_no]);
    //             case tree_visiting::all_post:
    //                 return true;
    //             case tree_visiting::inodes_post:
    //                 return true;
    //         }
    //     };

    //     bool end{false};
    //     for (bool cont{true}; cont && !end;) {
    //         auto& [parent_index, child_no] = parents_.back();
    //         if (child_no == parent_itself) {
    //             child_no = 0;
    //             cont = cont_for_visiting(parent_index, child_no);
    //         }
    //         else {
    //             auto& parent = tree_.inode(parent_index);
    //             if (child_no < parent.children.size()) {
    //                 ++child_no;
    //                 if (child_no == parent.children.size()) {
    //                     if (parents_.size() == 1)
    //                         end = true;
    //                     else
    //                         parents_.pop_back();
    //                 }
    //                 else
    //                     cont = cont_for_visiting(parent_index, child_no);
    //             }
    //             else
    //                 end = true;
    //         }
    //     }
    //     if (!end) {
    //         if (const auto cur_index = current_index(); !is_leaf(cur_index))
    //             parents_.emplace_back(cur_index, parent_itself);
    //     }
    //     return *this;
    // }

    template <lvalue_reference TREE, pointer LEAF, pointer INODE> inline typename tree_iterator_t<TREE, LEAF, INODE>::reference tree_iterator_t<TREE, LEAF, INODE>::operator*()
    {
        auto& [parent_index, child_no] = parents_.back();
        auto& parent = tree_.inode(parent_index);
        if (child_no == parent_itself)
            return &parent;
        else // if (child_no_ < parent.children.size())
            return tree_.node(parent.children[child_no]);
    }

} // namespace ae::tree

// ======================================================================
