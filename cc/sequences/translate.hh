#pragma once

#include <string>

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

    std::string translate_nucleotides_to_amino_acids(std::string_view nucleotides, size_t offset = 0);
}

// ======================================================================
