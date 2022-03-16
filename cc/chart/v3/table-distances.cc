#include "utils/log.hh"
#include "chart/v3/table-distances.hh"
#include "chart/v3/stress.hh"

// ----------------------------------------------------------------------

void ae::chart::v3::TableDistances::update(const Titer& titer, point_index p1, point_index p2, double column_basis, double adjust, multiply_antigen_titer_until_column_adjust mult)
{
    try {
        auto distance = column_basis - titer.logged() - adjust;
        if (distance < 0 && mult == multiply_antigen_titer_until_column_adjust::yes)
            distance = 0;
        add_value(titer.type(), p1, p2, distance);
    }
    catch (invalid_titer&) {
        // ignore dont-care
    }

} // ae::chart::v3::TableDistances::update

// ----------------------------------------------------------------------

void ae::chart::v3::TableDistances::update(const Titers& titers, const column_bases& col_bases, const StressParameters& parameters)
{
    const auto logged_adjusts = logged(parameters.m_avidity_adjusts);
    dodgy_is_regular(parameters.dodgy_titer_is_regular);
    if (titers.number_of_sera() > serum_index{0}) {
        for (const auto titer_ref : titers.titers_existing()) {
            if (!parameters.disconnected.exists(titer_ref.antigen) && !parameters.disconnected.exists(titers.number_of_antigens() + titer_ref.serum)) {
                double adj{0.0};
                if (!logged_adjusts.empty())
                    adj = logged_adjusts[*titer_ref.antigen] + logged_adjusts[*(titers.number_of_antigens() + titer_ref.serum)];
                update(titer_ref.titer, point_index{*titer_ref.antigen}, titers.number_of_antigens() + titer_ref.serum, col_bases.column_basis(titer_ref.serum), adj, parameters.mult);
            }
        }
    }
    else {
        throw std::runtime_error(AD_FORMAT("genetic table support not implemented"));
    }

} // ae::chart::v3::TableDistances::update

// ----------------------------------------------------------------------

ae::chart::v3::TableDistances::entries_for_point_t ae::chart::v3::TableDistances::entries_for_point(const entries_t& source, point_index point_no)
{
    entries_for_point_t result;
    for (const auto& src : source) {
        if (src.point_1 == point_no)
            result.emplace_back(src.point_2, src.distance);
        else if (src.point_2 == point_no)
            result.emplace_back(src.point_1, src.distance);
    }
    return result;

} // ae::chart::v3::TableDistances::entries_for_point

// ----------------------------------------------------------------------
