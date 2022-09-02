#pragma once

#include "sequences/pos.hh"

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
        double non_common_tolerance{0.6};  // if in the intermediate node most freq aa occupies more that this value (relative to total), consider the most freq aa to be common in this node

        double non_common_tolerance_for(sequences::pos0_t /*pos*/) const
        {
            // if (non_common_tolerance_per_pos.size() <= *pos || non_common_tolerance_per_pos[*pos] < 0.0)
            return non_common_tolerance;
            // else
            //     return non_common_tolerance_per_pos[*pos];
        }
    };

    void set_aa_nuc_transition_labels(Tree& tree, const AANucTransitionSettings& settings);
}

// ======================================================================
