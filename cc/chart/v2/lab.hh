#pragma once

#include "utils/named-type.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v2
{
    class Lab : public ae::named_string_t<std::string, struct chart_lab_tag_t>
    {
      public:
        using ae::named_string_t<std::string, struct chart_lab_tag_t>::named_string_t;
    };

} // namespace ae::chart::v2

// ----------------------------------------------------------------------
