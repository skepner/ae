#include "chart/v3/merge.hh"
#include "chart/v3/chart.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
}

// ----------------------------------------------------------------------

std::pair<std::shared_ptr<ae::chart::v3::Chart>, ae::chart::v3::merge_report_t> ae::chart::v3::merge(const Chart& chart1, const Chart& chart2, const merge_settings_t& settings)
{
    chart1.throw_if_duplicates();
    chart2.throw_if_duplicates();

    const common_antigens_sera_t common{chart1, chart2, settings.match_level};

    merge_report_t report{}; // (chart1, chart2, settings);

    auto merged = std::make_shared<Chart>();

    return {std::move(merged), std::move(report)};

    // ----------------------------------------------------------------------

    // const auto relax_type4_type5 = [](const MergeReport& report, ChartModify& chart) {
    //     UnmovablePoints unmovable_points;
    //     // set unmovable for all points of chart1 including common ones
    //     for (const auto& [index1, index_merge_common] : report.antigens_primary_target)
    //         unmovable_points.insert(index_merge_common.index);
    //     for (const auto& [index1, index_merge_common] : report.sera_primary_target)
    //         unmovable_points.insert(index_merge_common.index + chart.number_of_antigens());
    //     auto projection = chart.projection_modify(0);
    //     projection->set_unmovable(unmovable_points);
    //     projection->relax(optimization_options{});
    // };

    // // --------------------------------------------------

    // MergeReport report(chart1, chart2, settings);

    // ChartModifyP result = std::make_shared<ChartNew>(report.target_antigens, report.target_sera);
    // merge_info(*result, chart1, chart2);
    // auto& result_antigens = result->antigens_modify();
    // auto& result_sera = result->sera_modify();
    // merge_antigens_sera(result_antigens, *chart1.antigens(), report.antigens_primary_target, true);
    // merge_antigens_sera(result_antigens, *chart2.antigens(), report.antigens_secondary_target, false);
    // merge_antigens_sera(result_sera, *chart1.sera(), report.sera_primary_target, true);
    // merge_antigens_sera(result_sera, *chart2.sera(), report.sera_secondary_target, false);
    // if (const auto rda = result_antigens.find_duplicates(), rds = result_sera.find_duplicates(); !rda.empty() || !rds.empty()) {
    //     fmt::memory_buffer msg;
    //     fmt::format_to_mb(msg, "Merge \"{}\" has duplicates: AG:{} SR:{}\n", result->description(), rda, rds);
    //     for (const auto& dups : rda) {
    //         for (const auto ag_no : dups)
    //             fmt::format_to_mb(msg, "  AG {:5d} {}\n", ag_no, result_antigens.at(ag_no).name_full());
    //         fmt::format_to_mb(msg, "\n");
    //     }
    //     for (const auto& dups : rds) {
    //         for (const auto sr_no : dups)
    //             fmt::format_to_mb(msg, "  SR {:5d} {}\n", sr_no, result_sera.at(sr_no).name_full());
    //         fmt::format_to_mb(msg, "\n");
    //     }
    //     const auto err_message = fmt::to_string(msg);
    //     // AD_ERROR("{}", err_message);
    //     throw merge_error{err_message};
    // }

    // merge_titers(*result, chart1, chart2, report);
    // merge_plot_spec(*result, chart1, chart2, report);

    // if (chart1.number_of_projections()) {
    //     switch (settings.projection_merge) {
    //         case projection_merge_t::type1:
    //             break; // no projections in the merge
    //         case projection_merge_t::type2:
    //             merge_projections_type2(*result, chart1, chart2, report);
    //             break;
    //         case projection_merge_t::type3:
    //             if (chart2.number_of_projections() == 0)
    //                 throw merge_error{"cannot perform type3 merge: secondary chart has no projections"};
    //             merge_projections_type3(*result, chart1, chart2, report);
    //             break;
    //         case projection_merge_t::type4:
    //             if (chart2.number_of_projections() == 0)
    //                 throw merge_error{"cannot perform type4 merge: secondary chart has no projections"};
    //             merge_projections_type3(*result, chart1, chart2, report);
    //             // fmt::print(stderr, "DEBUG: merge type4 before relax stress {}\n", result->projection(0)->stress());
    //             relax_type4_type5(report, *result);
    //             // fmt::print(stderr, "DEBUG: merge type4 after relax stress {}\n", result->projection(0)->stress());
    //             break;
    //         case projection_merge_t::type5:
    //             if (chart2.number_of_projections() == 0)
    //                 throw merge_error{"cannot perform type5 merge: secondary chart has no projections"};
    //             merge_projections_type5(*result, chart1, chart2, report);
    //             // fmt::print(stderr, "DEBUG: merge type5 before relax stress {}\n", result->projection(0)->stress());
    //             relax_type4_type5(report, *result);
    //             // fmt::print(stderr, "DEBUG: merge type5 after relax stress {}\n", result->projection(0)->stress());
    //             break;
    //     }
    // }

    // return {std::move(result), std::move(report)};

} // ae::chart::v3::merge

// ----------------------------------------------------------------------
