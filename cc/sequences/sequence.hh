#pragma once

#include <string>

#include "sequences/pos.hh"

// ======================================================================

namespace ae::sequences
{
    template <typename Tag> class basic_sequence_t : public named_string_t<std::string, Tag>
    {
      public:
        using named_string_t<std::string, Tag>::named_string_t;
        using named_string_t<std::string, Tag>::operator=;

    };

    using sequence_nuc_t = basic_sequence_t<struct sequence_nuc_t_tag>;
    using sequence_aa_t = basic_sequence_t<struct sequence_aa_t_tag>;
}

// ======================================================================
