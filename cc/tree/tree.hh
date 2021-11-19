#pragma once

#include <memory>

#include "ext/filesystem.hh"
#include "utils/named-type.hh"
#include "sequences/sequence.hh"

// ======================================================================

namespace ae::tree
{
    using EdgeLength = named_double_t<struct tree_EdgeLength_tag>;
    using node_index_base_t = int;                                                      //  signed
    using node_index_t = named_number_t<node_index_base_t, struct tree_node_index_tag>; // signed! positive - leaves_, negative - inodes_

    struct Node
    {
        std::string name{};
        EdgeLength edge{0};
        EdgeLength cumulative_edge{0};
    };

    struct Leaf : public Node
    {
        Leaf(std::string_view a_name, EdgeLength a_edge) : Node{.name{a_name}, .edge{a_edge}} {}

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

      private:
        std::vector<Inode> inodes_{Inode{}}; // root is always there
        std::vector<Leaf> leaves_{};
    };

    std::shared_ptr<Tree> load(const std::filesystem::path& filename);
    void export_tree(const Tree& tree, const std::filesystem::path& filename);

} // namespace ae::tree

// ======================================================================
