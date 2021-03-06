#pragma once

// ======================================================================

namespace ae
{
    class Messages;
}

namespace ae::sequences
{
    struct RawSequence;

    // returns if sequence was translated
    bool align(RawSequence& sequence, Messages& messages);

    void calculate_hash(RawSequence& sequence);
}

// ======================================================================
