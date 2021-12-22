#pragma once

#include <string>
#include <string_view>
#include <memory>

#include "chart/v2/verify.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v2
{
    class Chart;
    using ChartP = std::shared_ptr<Chart>;

    ChartP import_from_file(std::string aFilename, Verify aVerify = Verify::None);
    inline ChartP import_from_file(std::string_view aFilename, Verify aVerify = Verify::None) { return import_from_file(std::string(aFilename), aVerify); }
    inline ChartP import_from_file(const char* aFilename, Verify aVerify = Verify::None) { return import_from_file(std::string(aFilename), aVerify); }
    ChartP import_from_data(std::string aData, Verify aVerify);
    ChartP import_from_data(std::string_view aData, Verify aVerify);
    ChartP import_from_decompressed_data(std::string aData, Verify aVerify);

} // namespace ae::chart::v2

// ----------------------------------------------------------------------
