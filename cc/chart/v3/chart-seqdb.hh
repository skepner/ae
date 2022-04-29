#pragma once

#include <cstddef>
#include <utility>

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Chart;

    /// populate with sequences from seqdb, set clades, returns number of antigens and number of sera that have sequences
    std::pair<size_t, size_t> populate_from_seqdb(Chart& chart);
}

// ======================================================================
