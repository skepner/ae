#include "chart/v3/chart.hh"
#include "chart/v3/avidity-test.hh"
#include "chart/v3/serum-circles.hh"

// ----------------------------------------------------------------------

namespace ae::py
{
    using Chart = ae::chart::v3::Chart;

    struct ProjectionRef
    {
        std::shared_ptr<Chart> chart;
        ae::chart::v3::Projection& projection;

        ProjectionRef(std::shared_ptr<Chart> a_chart, ae::chart::v3::Projection& a_projection) : chart{a_chart}, projection{a_projection} {}

        // ae::chart::v3::Projection& p() { return chart->projections()[projection_no]; }
        // const ae::chart::v3::Projection& p() const { return chart->projections()[projection_no]; }

        double stress() const { return projection.stress(*chart); }
        double recalculate_stress() const { projection.reset_stress(); return projection.stress(*chart); }
        std::string_view comment() const { return projection.comment(); }
        std::string minimum_column_basis() const { return projection.minimum_column_basis().format("{}", ae::chart::v3::minimum_column_basis::use_none::yes); }
        const std::vector<double>& forced_column_bases() const { return projection.forced_column_bases().data(); }
        std::vector<size_t> disconnected() const { return to_vector_base_t(projection.disconnected()); }
        std::vector<size_t> unmovable() const { return to_vector_base_t(projection.unmovable()); }
        std::vector<size_t> unmovable_in_the_last_dimension() const { return to_vector_base_t(projection.unmovable_in_the_last_dimension()); }
        ae::chart::v3::Transformation& transformation() { return projection.transformation(); }
        ae::chart::v3::Layout& layout() { return projection.layout(); }
        double relax(ae::chart::v3::optimization_precision precision)
        {
            // optimization_options opt;
            // opt.precision = precision;
            projection.relax(*chart, ae::chart::v3::optimization_options{.precision = precision});
            return projection.stress(*chart);
        }

        ae::chart::v3::avidity_test::results_t avidity_test(double adjust_step, double min_adjust, double max_adjust, bool rough)
        {
            return ae::chart::v3::avidity_test::test(*chart, projection,
                                                     ae::chart::v3::avidity_test::settings_t{.adjust_step = adjust_step, .min_adjust = min_adjust, .max_adjust = max_adjust, .rough = rough});
        }

        ae::chart::v3::serum_circles_t serum_circles(double fold) { return ae::chart::v3::serum_circles(*chart, projection, ae::chart::v3::serum_circle_fold{fold}); }
    };
}

// ----------------------------------------------------------------------
