#pragma once

// ======================================================================

namespace ae::tree
{
    class Tree;

    enum class aa_nuc_transition_method { consensus };

    struct AANucTransitionSettings
    {
        bool set_aa_labels{true};
        bool set_nuc_labels{false};
        aa_nuc_transition_method method{aa_nuc_transition_method::consensus};
    };

    void set_aa_nuc_transition_labels(Tree& tree, const AANucTransitionSettings& settings);
}

// ======================================================================
