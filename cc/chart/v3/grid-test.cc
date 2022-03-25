#include "ext/omp.hh"
#include "chart/v3/grid-test.hh"
#include "chart/v3/chart.hh"

// ----------------------------------------------------------------------

ae::chart::v3::grid_test::result_t ae::chart::v3::grid_test::test(const Chart& chart, projection_index projection_no, const settings_t& settings)
{
    const int threads{0};
    result_t results(chart.projections()[projection_no]);

#pragma omp parallel for default(none) shared(results, chart) num_threads(threads <= 0 ? omp_get_max_threads() : threads) schedule(static, chart.antigens().size() < antigen_index{1000} ? 4 : 1)
    for (size_t entry_no = 0; entry_no < results.size(); ++entry_no) {
        // test(results[entry_no]);
    }

    return results;

} // ae::chart::v3::grid_test::test

// ----------------------------------------------------------------------
