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

    // returns closest subtype and min hamming distance for it
    std::pair<ae::virus::type_subtype_t, size_t> closest_subtype_by_min_hamming_distance_to_master(const sequence_aa_t& source);

} // namespace ae::sequences

// ======================================================================
