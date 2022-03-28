#include "ext/omp.hh"
#include "chart/v3/avidity-test.hh"
#include "chart/v3/chart.hh"
#include "chart/v3/common.hh"
#include "chart/v3/procrustes.hh"
#include "chart/v3/serum-circles.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3::avidity_test
{
    static result_t test(const Chart& chart, const Projection& projection, antigen_index ag_no, const settings_t& settings);
    static per_adjust_t test(const Chart& chart, const Projection& projection, antigen_index ag_no, double logged_adjust, const optimization_options& options);
}

// ----------------------------------------------------------------------

ae::chart::v3::avidity_test::results_t ae::chart::v3::avidity_test::test(const Chart& chart, const Projection& projection, const settings_t& settings)
{
    results_t results{projection.stress(), chart.antigens().size()};
#ifdef _OPENMP
    const int num_threads = settings.threads <= 0 ? omp_get_max_threads() : settings.threads;
    const int slot_size = chart.antigens().size() < antigen_index{1000} ? 4 : 1;
#endif
#pragma omp parallel for default(shared) num_threads(num_threads) schedule(static, slot_size)
    for (size_t ag_no = 0; ag_no < *chart.antigens().size(); ++ag_no)
        results.data_[ag_no] = test(chart, projection, antigen_index{ag_no}, settings);
    results.post_process();
    return results;

} // ae::chart::v3::avidity_test::test

// ----------------------------------------------------------------------

ae::chart::v3::avidity_test::result_t ae::chart::v3::avidity_test::test(const Chart& chart, const Projection& projection, antigen_index antigen_no, const settings_t& settings)
{
    result_t result{.antigen_no = antigen_no, .best_logged_adjust = 0.0, .original = projection.layout()[antigen_no]};
    const optimization_options options{.num_threads = settings.threads};
    // low avidity
    for (double adjust = settings.adjust_step; adjust <= settings.max_adjust; adjust += settings.adjust_step)
        result.adjusts.push_back(test(chart, projection, antigen_no, adjust, options));
    // high avidity
    for (double adjust = - settings.adjust_step; adjust >= settings.min_adjust; adjust -= settings.adjust_step)
        result.adjusts.push_back(test(chart, projection, antigen_no, adjust, options));

    return result;

} // ae::chart::v3::avidity_test::test

// ----------------------------------------------------------------------

ae::chart::v3::avidity_test::per_adjust_t ae::chart::v3::avidity_test::test(const Chart& chart, const Projection& original_projection, antigen_index antigen_no, double logged_adjust,
                                                                            const optimization_options& options)
{
    const auto original_stress = original_projection.stress();
    Projection projection{original_projection};
    // AD_DEBUG("{}", projection.layout());
    auto& avidity_adjusts = projection.avidity_adjusts_access();
    resize(avidity_adjusts, chart.antigens().size(), chart.sera().size());
    set_logged(avidity_adjusts, antigen_no, logged_adjust);
    projection.comment(fmt::format("avidity {:+.1f} AG {}", logged_adjust, antigen_no));
    const auto status = projection.relax(chart, options);
    // AD_DEBUG("avidity relax AG {} adjust:{:4.1f} stress: {:10.4f} diff: {:8.4f}", antigen_no, logged_adjust, status.final_stress, status.final_stress - original_stress);

    const auto pc_data = procrustes(original_projection, projection, common_antigens_sera_t{chart}, procrustes_scaling_t::no);
    // AD_DEBUG("AG {} pc-rms:{}", antigen_no, pc_data.rms);
    const auto summary = procrustes_summary(original_projection.layout(), pc_data.secondary_transformed,
                                            procrustes_summary_parameters{.number_of_antigens = chart.antigens().size(), .number_of_sera = chart.sera().size(), .antigen_being_tested = antigen_no});

    per_adjust_t result{.logged_adjust = logged_adjust,
        .distance_test_antigen = summary.antigen_distances[*antigen_no],
        .angle_test_antigen = summary.test_antigen_angle,
        .average_procrustes_distances_except_test_antigen = summary.average_distance,
        .final_coordinates{projection.layout()[antigen_no]},
        .stress_diff = status.final_stress - original_stress};
    size_t most_moved_no{0};
    for (const auto ag_no : summary.antigens_by_distance) {
        if (ag_no != antigen_no) { // do not put antigen being tested into the most moved list
            result.most_moved[most_moved_no] = most_moved_t{ag_no, summary.antigen_distances[*ag_no]};
            ++most_moved_no;
            if (most_moved_no >= result.most_moved.size())
                break;
        }
    }
    // if (parameters().validVaccineAntigen()) {
    //     result.distance_vaccine_to_test_antigen = summary.distance_vaccine_to_test_antigen;
    //     result.angle_vaccine_to_test_antigen = summary.angle_vaccine_to_test_antigen;
    // }
    return result;

} // ae::chart::v3::avidity_test::test

// ----------------------------------------------------------------------

void ae::chart::v3::avidity_test::results_t::post_process()
{
    for (auto& result : data_)
        result.post_process();

} // ae::chart::v3::avidity_test::results_t::post_process

// ----------------------------------------------------------------------

const ae::chart::v3::avidity_test::per_adjust_t* ae::chart::v3::avidity_test::result_t::best_adjust() const
{
    if (const auto found = std::find_if(std::begin(adjusts), std::end(adjusts), [best = best_logged_adjust](const auto& en) { return float_equal(best, en.logged_adjust); });
        found != std::end(adjusts))
        return &*found;
    else
        return nullptr;

} // ae::chart::v3::avidity_test::result_t::best_adjust

// ----------------------------------------------------------------------

void ae::chart::v3::avidity_test::result_t::post_process()
{
    if (const auto best = std::min_element(std::begin(adjusts), std::end(adjusts), [](const auto& en1, const auto& en2) { return en1.stress_diff < en2.stress_diff; });
        best != std::end(adjusts) && best->stress_diff < 0) {
        best_logged_adjust = best->logged_adjust;
    }

} // ae::chart::v3::avidity_test::result_t::post_process

// ----------------------------------------------------------------------
