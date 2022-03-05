#pragma once

#include "utils/named-type.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    using number_of_dimensions_t = named_size_t<struct number_of_dimensions_tag>;

    constexpr bool valid(number_of_dimensions_t nd) { return nd.get() > 0; }

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
