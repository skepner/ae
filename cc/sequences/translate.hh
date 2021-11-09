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
    bool translate(RawSequence& sequence, Messages& messages);
}

// ======================================================================
