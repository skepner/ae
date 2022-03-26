#include "ext/omp.hh"
#include "chart/v3/grid-test.hh"
#include "chart/v3/chart.hh"
#include "chart/v3/stress.hh"
#include "chart/v3/area.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3::grid_test
{
    static void test(result_t& result, const Projection& projection, const Stress& stress, const settings_t& settings);
    static Area area_for(const Stress::TableDistancesForPoint& table_distances_for_point, const Layout& layout);
}

// ----------------------------------------------------------------------

ae::chart::v3::grid_test::results_t ae::chart::v3::grid_test::test(const Chart& chart, projection_index projection_no, const settings_t& settings)
{
    const auto& projection = chart.projections()[projection_no];
    results_t results(projection);
    const auto stress = stress_factory(chart, projection, optimization_options{}.mult);

#ifdef _OPENMP
    const int num_threads = settings.threads <= 0 ? omp_get_max_threads() : settings.threads;
    const int slot_size = chart.antigens().size() < antigen_index{1000} ? 4 : 1;
#endif
#pragma omp parallel for default(shared) num_threads(num_threads) schedule(static, slot_size)
    for (size_t entry_no = 0; entry_no < results.size(); ++entry_no)
        test(results[entry_no], projection, stress, settings);

    return results;

} // ae::chart::v3::grid_test::test

// ----------------------------------------------------------------------

void ae::chart::v3::grid_test::test(result_t& result, const Projection& projection, const Stress& stress, const settings_t& settings)
{
    if (result.diagnosis == result_t::not_tested) {
        const double hemisphering_distance_threshold = 1.0; // from acmacs-c2 hemi-local test: 1.0
        const double hemisphering_stress_threshold = 0.25;  // stress diff within threshold -> hemisphering, from acmacs-c2 hemi-local test: 0.25
        const optimization_options options;

        const auto table_distances_for_point = stress.table_distances_for(result.point_no);
        if (table_distances_for_point.empty()) { // no table distances, cannot test
            result.diagnosis = result_t::excluded;
            return;
        }

        result.diagnosis = result_t::normal;

        Layout layout{projection.layout()};
        const auto target_contribution = stress.contribution(result.point_no, table_distances_for_point, layout);
        const point_coordinates original_pos{layout[result.point_no]};
        auto best_contribution = target_contribution;
        point_coordinates best_coord, hemisphering_coord;
        const auto hemisphering_stress_thresholdrough = hemisphering_stress_threshold * 2;
        auto hemisphering_contribution = target_contribution + hemisphering_stress_thresholdrough;
        const auto area = area_for(table_distances_for_point, layout);
        for (auto it = area.begin(settings.step), last = area.end(); it != last; ++it) {
            // AD_DEBUG("grid_test iter {}", *it);
            layout.update(result.point_no, *it);
            const auto contribution = stress.contribution(result.point_no, table_distances_for_point, layout);
            if (contribution < best_contribution) {
                best_contribution = contribution;
                best_coord = *it;
            }
            else if (!best_coord.exists() && contribution < hemisphering_contribution && distance(original_pos, *it) > hemisphering_distance_threshold) {
                hemisphering_contribution = contribution;
                hemisphering_coord = *it;
            }
        }
        if (best_coord.exists()) {
            layout.update(result.point_no, best_coord);
            const auto status = optimize(options.method, stress, layout.span(), optimization_precision::rough);
            result.pos = layout[result.point_no];
            result.distance = distance(original_pos, result.pos);
            result.contribution_diff = status.final_stress - projection.stress();
            result.diagnosis = std::abs(result.contribution_diff) > hemisphering_stress_threshold ? result_t::trapped : result_t::hemisphering;
        }
        else if (hemisphering_coord.exists()) {
            // relax to find real contribution
            layout.update(result.point_no, hemisphering_coord);
            auto status = optimize(options.method, stress, layout.span(), optimization_precision::rough);
            result.pos = layout[result.point_no];
            result.distance = distance(original_pos, result.pos);
            if (result.distance > hemisphering_distance_threshold && result.distance < (hemisphering_distance_threshold * 1.2)) {
                status = optimize(options.method, stress, layout.span(), optimization_precision::fine);
                result.pos = layout[result.point_no];
                result.distance = distance(original_pos, result.pos);
            }
            result.contribution_diff = status.final_stress - projection.stress();
            if (result.distance > hemisphering_distance_threshold) {
                // if (const auto real_contribution_diff = stress.contribution(result.point_no, table_distances_for_point, layout.data()) - target_contribution;
                //     real_contribution_diff < hemisphering_stress_threshold) {
                if (std::abs(result.contribution_diff) < hemisphering_stress_threshold) {
                    // result.contribution_diff = real_contribution_diff;
                    result.diagnosis = result_t::hemisphering;
                }
            }
        }
        // AD_DEBUG("GridTest {} area: {:8.1f} units^{}  grid-step: {:5.3f}", result.report(chart_), area.area(), area.num_dim(), grid_step_);
    }

} // ae::chart::v3::grid_test::test

// ----------------------------------------------------------------------

ae::chart::v3::Area ae::chart::v3::grid_test::area_for(const Stress::TableDistancesForPoint& table_distances_for_point, const Layout& layout)
{
    point_index another_point;
    if (!table_distances_for_point.regular.empty())
        another_point = table_distances_for_point.regular.front().another_point;
    else if (!table_distances_for_point.less_than.empty())
        another_point = table_distances_for_point.less_than.front().another_point;
    else
        throw std::runtime_error("ae::chart::v3::grid_test::::area_for: table_distances_for_point has neither regulr nor less_than entries");
    Area area(layout[another_point]);
    auto extend = [&area, &layout](const auto& entry) {
        const auto coord = layout[entry.another_point];
        const auto radius = entry.distance; // + 1;
        area.extend(coord - radius);
        area.extend(coord + radius);
    };
    std::for_each(table_distances_for_point.regular.begin(), table_distances_for_point.regular.end(), extend);
    std::for_each(table_distances_for_point.less_than.begin(), table_distances_for_point.less_than.end(), extend);
    return area;

} // ae::chart::v3::grid_test::area_for

// ----------------------------------------------------------------------

ae::chart::v3::grid_test::results_t::results_t(const Projection& projection)
    : data_(*projection.number_of_points(), result_t{point_index{0}, projection.number_of_dimensions()})
{
    point_index point_no{0};
    for (auto& res : data_)
        res.point_no = point_no++;
    exclude_disconnected(projection);

} // ae::chart::v3::grid_test::results_t::results_t

// ----------------------------------------------------------------------

void ae::chart::v3::grid_test::results_t::exclude_disconnected(const Projection& projection)
{
    const auto exclude = [this](point_index point_no) {
        if (auto* found = find(point_no); found)
            found->diagnosis = result_t::excluded;
    };

    for (auto unmovable : projection.unmovable())
        exclude(unmovable);
    for (auto disconnected : projection.disconnected())
        exclude(disconnected);
    data_.erase(std::remove_if(data_.begin(), data_.end(), [](const auto& entry) { return entry.diagnosis == result_t::excluded; }), data_.end());

} // ae::chart::v3::grid_test::results_t::exclude_disconnected

// ----------------------------------------------------------------------

ae::chart::v3::grid_test::result_t* ae::chart::v3::grid_test::results_t::find(point_index point_no)
{
    if (const auto found = std::find_if(data_.begin(), data_.end(), [point_no](const auto& en) { return en.point_no == point_no; }); found != data_.end())
        return &*found;
    else
        return nullptr;

} // ae::chart::v3::grid_test::results_t::find

// ----------------------------------------------------------------------

std::vector<ae::chart::v3::grid_test::result_t> ae::chart::v3::grid_test::results_t::trapped_hemisphering() const
{
    std::vector<result_t> th;
    for (const auto& en : data_) {
        if (en.diagnosis == result_t::trapped || en.diagnosis == result_t::hemisphering)
            th.push_back(en);
    }
    return th;

} // ae::chart::v3::grid_test::trapped_hemisphering

// ----------------------------------------------------------------------

void ae::chart::v3::grid_test::results_t::apply(Layout& layout) const
{
    for (const auto& result : data_) {
        if (result.pos.exists() && result.contribution_diff < 0.0)
            layout.update(result.point_no, result.pos);
    }

} // ae::chart::v3::grid_test::results_t::apply

// ----------------------------------------------------------------------

void ae::chart::v3::grid_test::results_t::apply(Projection& projection) const // move points to their better locations
{
    apply(projection.layout());

} // ae::chart::v3::grid_test::results_t::apply

// ----------------------------------------------------------------------
