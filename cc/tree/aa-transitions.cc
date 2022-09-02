#include "tree/aa-transitions.hh"
#include "tree/tree.hh"
#include "utils/timeit.hh"

// ======================================================================

namespace ae::tree
{
    static void remove_aa_transition_labels(Tree& tree);
    static void remove_nuc_transition_labels(Tree& tree);
    static void set_aa_nuc_transition_labels_consensus(Tree& tree, const AANucTransitionSettings& settings);
}

// ----------------------------------------------------------------------

void ae::tree::set_aa_nuc_transition_labels(Tree& tree, const AANucTransitionSettings& settings)
{
    if (settings.set_aa_labels)
        remove_aa_transition_labels(tree);
    if (settings.set_nuc_labels)
        remove_nuc_transition_labels(tree);
    switch (settings.method) {
        case aa_nuc_transition_method::consensus:
            set_aa_nuc_transition_labels_consensus(tree, settings);
            break;
    }

} // ae::tree::set_aa_nuc_transition_labels

// ----------------------------------------------------------------------

void ae::tree::remove_aa_transition_labels(Tree& tree)
{
    for (auto ref : tree.visit(tree_visiting::inodes))
        ref.inode()->aa_transitions.clear();

} // ae::tree::remove_aa_transition_labels

// ----------------------------------------------------------------------

void ae::tree::remove_nuc_transition_labels(Tree& tree)
{
    for (auto ref : tree.visit(tree_visiting::inodes))
        ref.inode()->nuc_transitions.clear();

} // ae::tree::remove_nuc_transition_labels

// ----------------------------------------------------------------------

namespace ae::tree
{
    enum class aa_nuc_e { aa, nuc };

    template <aa_nuc_e aa_nuc> class set_aa_nuc_transition_labels_consensus_t
    {
      public:
        set_aa_nuc_transition_labels_consensus_t(Tree& tree, size_t longest_seq, const Leaf& root_leaf, const AANucTransitionSettings& settings)
            : tree_{tree}, longest_seq_{longest_seq}, root_leaf_{root_leaf}, settings_{settings}
        {
        }

        void for_pos(size_t pos)
        {
            update_common(pos);

            //     // AD_DEBUG(parameters.debug, "eu-20200915 set aa transitions =============================================================");
            //     set_aa_transitions_eu_20210205_for_pos(tree, pos, parameters);
            //     // AD_DEBUG(parameters.debug, "eu-20200915 update aa transitions ================================================================================");

            //     // AD_DEBUG("update aa transitions");
            //     // const Timeit ti{"update aa transitions"};
            //     update_aa_transitions_eu_20200915_stage_3(tree, pos, root_sequence, parameters);
        }

        void update_common(size_t pos)
        {
            for (auto ref : tree_.visit(tree_visiting::inodes)) {
                AD_DEBUG("reset_common_aa {}", ref.node_id());
                ref.inode()->reset_common_aa();
            }

            // unsigned max_count{0};
            for (auto ref : tree_.visit(tree_visiting::inodes_post)) {
                auto& node = *ref.inode();
                AD_DEBUG(!node.common_aa, "node.common_aa {} -- {} {}", fmt::ptr(node.common_aa.get()), ref.node_id(), node.children);
                for (const auto child_id : node.children) {
                    if (is_leaf(child_id)) {
                        if (auto& child = tree_.leaf(child_id); child.shown) {
                            if (child.aa.size() > pos)
                                node.common_aa->count(child.aa[pos]);
                        }
                    }
                    else {
                        if (auto& child = tree_.inode(child_id); child.shown)
                            node.common_aa->update(*child.common_aa);
                    }
                }
                // max_count = std::max(max_count, node.common_aa->max().second);
            }
        }

      private:
        Tree& tree_;
        size_t longest_seq_;
        const Leaf& root_leaf_;
        const AANucTransitionSettings& settings_;
    };
} // namespace ae::tree

void ae::tree::set_aa_nuc_transition_labels_consensus(Tree& tree, const AANucTransitionSettings& settings)
{
    const auto& root_leaf = tree.leaf(tree.first_leaf(tree.root_index()));
    // const auto total_leaves = tree.number_leaves_in_subtree();
    const auto [max_aa, max_nuc] = tree.longest_sequence();
    set_aa_nuc_transition_labels_consensus_t<aa_nuc_e::aa> set_aa{tree, max_aa, root_leaf, settings};
    set_aa_nuc_transition_labels_consensus_t<aa_nuc_e::nuc> set_nuc{tree, max_nuc, root_leaf, settings};
    auto start = ae::clock_t::now(), chunk_start = start;
    for (size_t pos{0}; pos < max_aa; ++pos) {
        if (settings.set_aa_labels)
            set_aa.for_pos(pos);
        if (settings.set_nuc_labels)
            set_nuc.for_pos(pos);
        if ((pos % 100) == 99) {
            AD_DEBUG("set_aa_nuc_transition_labels_consensus pos:{:5d}  time: {:%H:%M:%S}", pos, ae::elapsed(chunk_start));
            chunk_start = ae::clock_t::now();
        }
    }
    AD_DEBUG("set_aa_nuc_transition_labels_consensus positions: {}  time: {:%H:%M:%S}", max_aa, ae::elapsed(start));

    // tree::iterate_pre(tree, [&parameters](Node& node) { node.aa_transitions_.remove_left_right_same(parameters, node); });

    // if (!tree.aa_transitions_.empty())
    //     AD_WARNING("Root AA transions: {} (hide some roots to show this transion(s) in the first branch)", tree.aa_transitions_);

} // ae::tree::set_aa_nuc_transition_labels_consensus

// ----------------------------------------------------------------------
