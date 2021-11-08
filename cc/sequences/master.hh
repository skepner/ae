#pragma once

#include <vector>

#include "virus/type-subtype.hh"
#include "sequences/sequence.hh"

// ======================================================================

namespace ae::sequences
{
    struct MasterSequence
    {
        ae::virus::type_subtype_t type_subtype;
        std::string_view name;
        sequence_aa_t aa;
    };

    std::vector<const MasterSequence*> master_sequences(const ae::virus::type_subtype_t& ts);

    size_t min_hamming_distance_to_master(const ae::virus::type_subtype_t& ts, const sequence_aa_t& source);

    // returns closest master sequence and min hamming distance for it
    std::pair<const MasterSequence*, size_t> closest_subtype_by_min_hamming_distance_to_master(const sequence_aa_t& source);

    const MasterSequence* master_sequence_for(const ae::virus::type_subtype_t& ts);

    inline size_t ha_sequence_length_for(const ae::virus::type_subtype_t& ts)
    {
        if (const auto* master = master_sequence_for(ts); master)
            return master->aa.size();
        else
            return 0;
    }

} // namespace ae::sequences

// ======================================================================
