#pragma once

#include "utils/named-type.hh"

// ----------------------------------------------------------------------

namespace ae::virus::inline v2
{
    // INFLUENZA (default, if omitted), HPV, generic, DENGE
    class virus_t : public named_string_t<std::string, struct virus_tag>
    {
      public:
        using named_string_t<std::string, struct virus_tag>::named_string_t;
    };

} // namespace ae::virus::inline v2

// ----------------------------------------------------------------------
