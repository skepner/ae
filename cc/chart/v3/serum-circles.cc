#include "serum-circles.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
}

// ----------------------------------------------------------------------

ae::chart::v3::serum_circles_t ae::chart::v3::serum_circles(const Chart& chart, const Projection& projection, serum_circle_fold fold)
{
    const auto column_bases = chart.column_bases(projection.minimum_column_basis());
    serum_circles_t circles;

    for (const auto sr_no : chart.sera().size()) {
        // AD_DEBUG("homologous SR {:4d} \"{}\": {}", sr_no, chart.sera()[sr_no].designation(), chart.antigens().homologous(chart.sera()[sr_no]));
        auto& serum_data = circles.sera.emplace_back(sr_no, column_bases[sr_no], fold);
        for (const auto ag_no : chart.antigens().homologous(chart.sera()[sr_no])) {
            if (const auto& titer = chart.titers().titer(ag_no, sr_no); titer.is_regular()) {
                serum_data.antigens.push_back({.antigen_no = ag_no,
                                               .titer = titer,
                                               .theoretical = std::nullopt, // acmacs::chart::serum_circle_theoretical(ag_no, sr_no, chart, projection_no, 2.0, verbose),
                                               .empirical = std::nullopt, // acmacs::chart::serum_circle_empirical(ag_no, sr_no, chart, projection_no, 2.0, verbose)});
                                               .reason = serum_circle_failure_reason::good
                    });
            }
            else {
                serum_data.antigens.push_back({.antigen_no = ag_no, .titer = titer, .reason = serum_circle_failure_reason::non_regular_homologous_titer});
            }
        }
    }

    return circles;

} // ae::chart::v3::serum_circles

// ----------------------------------------------------------------------
