#include "chart/v3/merge.hh"
#include "chart/v3/chart.hh"

// ----------------------------------------------------------------------

std::pair<std::shared_ptr<ae::chart::v3::Chart>, ae::chart::v3::merge_report_t> ae::chart::v3::merge(const Chart& chart1, const Chart& chart2, const merge_settings_t& settings)
{
    const common_antigens_sera_t common{chart1, chart2, settings.match_level};

    merge_report_t report{}; // (chart1, chart2, settings);


    auto merged = std::make_shared<Chart>();

    return {std::move(merged), std::move(report)};

} // ae::chart::v3::merge

// ----------------------------------------------------------------------
