#pragma once

#include "utils/named-type.hh"

// ----------------------------------------------------------------------

namespace ae::virus
{
    class Reassortant : public named_string_t<std::string, struct virus_reassortant_tag_t>
    {
     public:
        using ae::named_string_t<std::string, struct virus_reassortant_tag_t>::named_string_t;

    }; // class Reassortant

    namespace reassortant
    {
        std::tuple<Reassortant, std::string> parse(std::string_view source);
    }

} // namespace acmacs::virus

// ----------------------------------------------------------------------
