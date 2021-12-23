#pragma once

#include <string>

// ----------------------------------------------------------------------

namespace ae::chart::v2
{
    class Chart;

    std::string export_lispmds(const Chart& aChart, std::string_view aProgramName);

} // namespace ae::chart::v2

// ----------------------------------------------------------------------
