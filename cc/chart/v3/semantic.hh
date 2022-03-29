#pragma once

#include "sequences/clades.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class SemanticAttributes
    {
      public:
        bool operator==(const SemanticAttributes&) const = default;

        sequences::clades_t clades;
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
