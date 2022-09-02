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
        set_aa_nuc_transition_labels_consensus_t(Tree& tree, sequences::pos0_t longest_seq, const Leaf& root_leaf, const AANucTransitionSettings& settings)
            : tree_{tree}, longest_seq_{longest_seq}, root_leaf_{root_leaf}, settings_{settings}
        {
        }

        void for_pos(sequences::pos0_t pos)
        {
            update_common(pos);

            //     // AD_DEBUG(parameters.debug, "eu-20200915 set aa transitions =============================================================");
            set_transitions(pos);
            //     // AD_DEBUG(parameters.debug, "eu-20200915 update aa transitions ================================================================================");

            //     // AD_DEBUG("update aa transitions");
            //     // const Timeit ti{"update aa transitions"};
            //     update_aa_transitions_eu_20200915_stage_3(tree, pos, root_sequence, parameters);
        }

        sequences::pos0_t seq_size(const Leaf& leaf) const
        {
            if constexpr (aa_nuc == aa_nuc_e::aa)
                return leaf.aa.size();
            else
                return leaf.nuc.size();
        }

        char seq_at(const Leaf& leaf, sequences::pos0_t pos) const
        {
            if constexpr (aa_nuc == aa_nuc_e::aa)
                return leaf.aa[pos];
            else
                return leaf.nuc[pos];
        }

        void update_common(sequences::pos0_t pos)
        {
            for (auto ref : tree_.visit(tree_visiting::inodes))
                ref.inode()->reset_common_aa();

            // unsigned max_count{0};
            for (auto ref : tree_.visit(tree_visiting::inodes_post)) {
                auto& node = *ref.inode();
                for (const auto child_id : node.children) {
                    if (is_leaf(child_id)) {
                        if (auto& child = tree_.leaf(child_id); child.shown) {
                            if (seq_size(child) > pos)
                                node.common_aa->count(seq_at(child, pos));
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

        void set_transitions(sequences::pos0_t pos)
        {
            const auto non_common_tolerance = settings_.non_common_tolerance_for(pos);
            for (auto ref : tree_.visit(tree_visiting::inodes_post))
                set_transitions(*ref.inode(), pos, non_common_tolerance);
        }

        void set_transitions(Inode& node, sequences::pos0_t pos, double non_common_tolerance)
        {
            // if (!is_common_with_tolerance(node, pos, non_common_tolerance)) {
            //     for (const auto child_id : node.children) {
            //         if (!is_leaf(child_id)) {
            //             if (auto& child = tree_.inode(child_id); is_common_with_tolerance_for_child(child, pos, non_common_tolerance))
            //                 child.aa_transitions.add(pos, child.common_aa->at(pos, non_common_tolerance));
            //         }
            //     }
            // }
            // else {
            //     for (const auto child_id : node.children) {
            //         if (!is_leaf(child_id)) {
            //             if (auto& child = tree_.inode(child_id); !child.common_aa->empty(pos)) {
            //                 const auto child_aa = child.common_aa->at(pos, non_common_tolerance);
            //                 if (const auto [common_child, msg_child] = is_common_with_tolerance_for_child(child, pos, non_common_tolerance /*, dbg*/); /* child_aa != node_aa && */ common_child)
            //                     child.replace_aa_transition(pos, child_aa);
            //             }
            //         }
            //     }
            // }
        }

        // bool is_common_with_tolerance(const Inode& node, sequences::pos0_t pos, double tolerance)
        // {
        //     const auto aa = node.common_aa->at(pos, tolerance);
        //     if (aa == NoCommon) {
        //         if constexpr (dbg)
        //             fmt::format_to_mb(msg, "common:no");
        //         return {false, fmt::to_string(msg)};
        //     }
        //     // tolerance problem: aa is common with tolerance but in
        //     // reality just 1 or 2 child nodes have this aa and other
        //     // children (with much fewer leaves) have different
        //     // aa's. In that case consider that aa to be not
        //     // common. See H3 and M346L labelling in the 3a clade.
        //     const auto [num_common_aa_children, common_children] = number_of_children_with_the_same_common_aa<dbg>(node, aa, pos, tolerance);
        //     const auto not_common = // num_common_aa_children > 0 && number_of_children_with_the_same_common_aa <= 1 &&
        //         node.subtree.size() > static_cast<size_t>(num_common_aa_children);
        //     if constexpr (dbg) {
        //         fmt::format_to_mb(msg, fmt::runtime("common:{} <-- {} {:5.3} aa:{} tolerance:{} number_of_children_with_the_same_common_aa:{} ({}) subtree-size:{}"), !not_common, pos, node.node_id,
        //                           aa, tolerance, num_common_aa_children, common_children, node.subtree.size());
        //     }
        //     return !not_common;
        // }

        // bool is_common_with_tolerance_for_child(const Inode& node, sequences::pos0_t pos, double tolerance)
        // {
        //     fmt::memory_buffer msg;
        //     const auto aa = node.common_aa_->at(pos, tolerance);
        //     if (aa == NoCommon) {
        //         if constexpr (dbg)
        //             fmt::format_to_mb(msg, "common:no");
        //         return {false, fmt::to_string(msg)};
        //     }
        //     else {
        //         const auto [num_common_aa_children, common_children] = number_of_children_with_the_same_common_aa<dbg>(node, aa, pos, tolerance);
        //         const auto not_common = num_common_aa_children <= 1; // && node.subtree.size() > static_cast<size_t>(num_common_aa_children);
        //         if constexpr (dbg) {
        //             fmt::format_to_mb(msg, fmt::runtime("common:{} <-- {} {:5.3} aa:{} tolerance:{} number_of_children_with_the_same_common_aa:{} ({}) subtree-size:{}"), !not_common, pos,
        //                               node.node_id, aa, tolerance, num_common_aa_children, common_children, node.subtree.size());
        //         }
        //         return {!not_common, fmt::to_string(msg)};
        //     }
        // }

      private:
        Tree& tree_;
        sequences::pos0_t longest_seq_;
        const Leaf& root_leaf_;
        const AANucTransitionSettings& settings_;
    };
} // namespace ae::tree

// ----------------------------------------------------------------------

void ae::tree::set_aa_nuc_transition_labels_consensus(Tree& tree, const AANucTransitionSettings& settings)
{
    const auto& root_leaf = tree.leaf(tree.first_leaf(tree.root_index()));
    // const auto total_leaves = tree.number_leaves_in_subtree();
    const auto [max_aa, max_nuc] = tree.longest_sequence();
    set_aa_nuc_transition_labels_consensus_t<aa_nuc_e::aa> set_aa{tree, max_aa, root_leaf, settings};
    set_aa_nuc_transition_labels_consensus_t<aa_nuc_e::nuc> set_nuc{tree, max_nuc, root_leaf, settings};
    auto start = ae::clock_t::now(), chunk_start = start;
    for (sequences::pos0_t pos{0}; pos < max_aa; ++pos) {
        if (settings.set_aa_labels)
            set_aa.for_pos(pos);
        if (settings.set_nuc_labels)
            set_nuc.for_pos(pos);
        if ((*pos % 100) == 99) {
            AD_DEBUG("set_aa_nuc_transition_labels_consensus pos:{}  time: {:%H:%M:%S}", pos, ae::elapsed(chunk_start));
            chunk_start = ae::clock_t::now();
        }
    }
    AD_DEBUG("set_aa_nuc_transition_labels_consensus positions: {}  time: {:%H:%M:%S}", max_aa, ae::elapsed(start));

    // tree::iterate_pre(tree, [&parameters](Node& node) { node.aa_transitions_.remove_left_right_same(parameters, node); });

    // if (!tree.aa_transitions_.empty())
    //     AD_WARNING("Root AA transions: {} (hide some roots to show this transion(s) in the first branch)", tree.aa_transitions_);

} // ae::tree::set_aa_nuc_transition_labels_consensus

// ----------------------------------------------------------------------
