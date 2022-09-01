#include "tree/aa-transitions.hh"
#include "tree/tree.hh"

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

void ae::tree::set_aa_nuc_transition_labels_consensus(Tree& tree, const AANucTransitionSettings& settings)
{

} // ae::tree::set_aa_nuc_transition_labels_consensus

// ----------------------------------------------------------------------
