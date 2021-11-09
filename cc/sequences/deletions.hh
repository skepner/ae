#pragma once

#include "sequences/raw-sequence.hh"

// ======================================================================

namespace ae
{
    class Messages;
}

namespace ae::sequences
{
    void find_deletions_insertions_set_lineage(RawSequence& sequence, Messages& messages);

} // namespace ae::sequences

// ======================================================================
