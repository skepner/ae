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
        enum class visiting { all, leaves, inodes, all_post };
        enum _init_end { init_end };

        using reference = std::variant<const Leaf*, const Inode*>;

        tree_iterator(const Tree& tree, visiting a_visiting);
        tree_iterator(const Tree& tree, visiting a_visiting, _init_end);
        tree_iterator& operator++();
        reference operator*();
        bool operator==(const tree_iterator& rhs) const { return &tree_ == &rhs.tree_ && parents_.back() == rhs.parents_.back(); }

      private:
        const Tree& tree_;
        visiting visiting_;
        std::vector<std::pair<node_index_t, size_t>> parents_; // parent and index in tree.inode(parents_.back()).children, index=-1 for tree.inode(parents_.back()) itself

        constexpr static size_t parent_itself{static_cast<size_t>(-1)};

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

        tree_iterator::reference node(node_index_t index) const
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

        tree_iterator begin(tree_iterator::visiting a_visiting = tree_iterator::visiting::all) const {return {*this, a_visiting}; }
        tree_iterator end(tree_iterator::visiting a_visiting = tree_iterator::visiting::all) const { return {*this, a_visiting, tree_iterator::init_end}; }

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

    // begin
    inline tree_iterator::tree_iterator(const Tree& tree, visiting a_visiting) : tree_{tree}, visiting_{a_visiting}
    {
        parents_.reserve(tree.depth());
        parents_.emplace_back(node_index_t{0}, parent_itself);
        switch (visiting_) {
            case visiting::all:
            case visiting::inodes:
                break;
            case visiting::leaves:
            case visiting::all_post:
                operator++();
                break;
        }
        fmt::print(">>>> begin {} {}\n", *parents_.back().first, parents_.back().second);
    }

    // end
    inline tree_iterator::tree_iterator(const Tree& tree, visiting a_visiting, _init_end) : tree_{tree}, visiting_{a_visiting}
    {
        parents_.emplace_back(node_index_t{0}, tree.root().children.size());
        fmt::print(">>>> end {} {}\n", *parents_.back().first, parents_.back().second);
    }

    inline tree_iterator& tree_iterator::operator++()
    {
        const auto cont_for_visiting = [this]() {
            switch (visiting_) {
                case visiting::all:
                    return false;
                case visiting::inodes:
                    return true;
                case visiting::leaves:
                    return true;
                case visiting::all_post:
                    return true;
            }
        };

        const auto current_index = [this]() {
            if (const auto [parent_index, child_no] = parents_.back(); child_no == parent_itself)
                return parent_index;
            else
                return tree_.inode(parent_index).children[child_no];
        };

        bool end{false};
        for (bool cont{true}; cont && !end;) {
            auto& [parent_index, child_no] = parents_.back();
            if (child_no == parent_itself) {
                child_no = 0;
                cont = cont_for_visiting();
            }
            else {
                auto& parent = tree_.inode(parent_index);
                if (child_no < parent.children.size()) {
                    ++child_no;
                    if (child_no == parent.children.size()) {
                        if (parents_.size() == 1)
                            end = true;
                        else
                            parents_.pop_back();
                    }
                    else
                        cont = cont_for_visiting();
                }
                else
                    end = true;
            }
        }
        if (!end) {
            if (const auto cur_index = current_index(); !is_leaf(cur_index))
                parents_.emplace_back(cur_index, parent_itself);
            if (const auto cur_index = current_index(); is_leaf(cur_index)) {
                fmt::print(">>>> {} ++ {} {}\n", *cur_index, *parents_.back().first, parents_.back().second);
                fmt::print(">>>>     \"{}\"\n", tree_.leaf(cur_index).name);
            }
            else
                fmt::print(">>>> {} ++ {} {}\n", *cur_index, *parents_.back().first, parents_.back().second);
        }
        return *this;
    }

    inline tree_iterator::reference tree_iterator::operator*()
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
