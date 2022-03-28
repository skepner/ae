#include "serum-circles.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
}

// ----------------------------------------------------------------------

ae::chart::v3::serum_circles_t ae::chart::v3::serum_circles(const Chart& chart, const Projection& projection, serum_circle_fold fold)
{
    // chart.set_homologous(acmacs::chart::find_homologous::all);

    for (const auto sr_no : chart.sera().size()) {
        AD_DEBUG("homologous SR {:4d} \"{}\": {}", sr_no, chart.sera()[sr_no].designation(), chart.antigens().homologous(chart.sera()[sr_no]));
        for (auto ag_no : chart.antigens().homologous(chart.sera()[sr_no])) {
        }
    }

    return serum_circles_t{};

} // ae::chart::v3::serum_circles

// ----------------------------------------------------------------------
