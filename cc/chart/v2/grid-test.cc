#include "ext/range-v3.hh"
#include "ext/fmt.hh"
#include "ext/omp.hh"
#include "utils/log.hh"
#include "utils/file.hh"
#include "ad/to-json.hh"
#include "ad/data-formatter.hh"
#include "chart/v2/grid-test.hh"
#include "chart/v2/name-format.hh"

// #include "acmacs-base/enumerate.hh"

// ======================================================================

// #ifndef __clang__
// // workaround for a bug in gcc 7.2
// static std::vector<acmacs::chart::GridTest::Result> _gcc72_bug;
// #endif

// ----------------------------------------------------------------------

std::string ae::chart::v2::GridTest::point_name(size_t point_no) const
{
    if (antigen(point_no)) {
        return fmt::format("AG {} {}", point_no, (*chart_.antigens())[point_no]->format("{name_full}"));
    }
    else {
        const auto serum_no = antigen_serum_no(point_no);
        return fmt::format("SR {} {}", serum_no, (*chart_.sera())[serum_no]->format("{name_full}"));
    }

} // ae::chart::v2::GridTest::point_name

// ----------------------------------------------------------------------

ae::chart::v2::Area ae::chart::v2::GridTest::area_for(const Stress::TableDistancesForPoint& table_distances_for_point) const
{
    size_t another_point;
    if (!table_distances_for_point.regular.empty())
        another_point = table_distances_for_point.regular.front().another_point;
    else if (!table_distances_for_point.less_than.empty())
        another_point = table_distances_for_point.less_than.front().another_point;
    else
        throw std::runtime_error("ae::chart::v2::GridTest::area_for: table_distances_for_point has neither regulr nor less_than entries");
    Area result(original_layout_.at(another_point));
    auto extend = [&result,this](const auto& entry) {
        const auto coord = this->original_layout_.at(entry.another_point);
        const auto radius = entry.distance; // + 1;
        result.extend(coord - radius);
        result.extend(coord + radius);
    };
    std::for_each(table_distances_for_point.regular.begin(), table_distances_for_point.regular.end(), extend);
    std::for_each(table_distances_for_point.less_than.begin(), table_distances_for_point.less_than.end(), extend);
    return result;

} // ae::chart::v2::GridTest::area_for

// ----------------------------------------------------------------------

ae::chart::v2::GridTest::Result ae::chart::v2::GridTest::test(size_t point_no)
{
    Result result(point_no, original_layout_.number_of_dimensions());
    test(result);
    return result;

} // ae::chart::v2::GridTest::test_point

// ----------------------------------------------------------------------

void ae::chart::v2::GridTest::test(Result& result)
{
    if (result.diagnosis == Result::not_tested) {
        const auto table_distances_for_point = stress_.table_distances_for(result.point_no);
        if (table_distances_for_point.empty()) { // no table distances, cannot test
            result.diagnosis = Result::excluded;
            return;
        }

        result.diagnosis = Result::normal;

        Layout layout(original_layout_);
        const auto target_contribution = stress_.contribution(result.point_no, table_distances_for_point, layout.data());
        const auto original_pos = original_layout_.at(result.point_no);
        auto best_contribution = target_contribution;
        PointCoordinates best_coord(original_pos.number_of_dimensions()),
                hemisphering_coord(original_pos.number_of_dimensions());
        const auto hemisphering_stress_threshold_rough = hemisphering_stress_threshold_ * 2;
        auto hemisphering_contribution = target_contribution + hemisphering_stress_threshold_rough;
        const auto area = area_for(table_distances_for_point);
        for (auto it = area.begin(grid_step_), last = area.end(); it != last; ++it) {
            layout.update(result.point_no, *it);
            const auto contribution = stress_.contribution(result.point_no, table_distances_for_point, layout.data());
            if (contribution < best_contribution) {
                best_contribution = contribution;
                best_coord = *it;
            }
            else if (!best_coord.exists() && contribution < hemisphering_contribution && distance(original_pos, *it) > hemisphering_distance_threshold_) {
                hemisphering_contribution = contribution;
                hemisphering_coord = *it;
            }
        }
        if (best_coord.exists()) {
            layout.update(result.point_no, best_coord);
            const auto status = optimize(optimization_method_, stress_, layout.data(), layout.data() + layout.size(), optimization_precision::rough);
            result.pos = layout.at(result.point_no);
            result.distance = distance(original_pos, result.pos);
            result.contribution_diff = status.final_stress - projection_->stress();
            result.diagnosis = std::abs(result.contribution_diff) > hemisphering_stress_threshold_ ? Result::trapped : Result::hemisphering;
        }
        else if (hemisphering_coord.exists()) {
            // relax to find real contribution
            layout.update(result.point_no, hemisphering_coord);
            auto status = optimize(optimization_method_, stress_, layout.data(), layout.data() + layout.size(), optimization_precision::rough);
            result.pos = layout.at(result.point_no);
            result.distance = distance(original_pos, result.pos);
            if (result.distance > hemisphering_distance_threshold_ && result.distance < (hemisphering_distance_threshold_ * 1.2)) {
                status = optimize(optimization_method_, stress_, layout.data(), layout.data() + layout.size(), optimization_precision::fine);
                result.pos = layout.at(result.point_no);
                result.distance = distance(original_pos, result.pos);
            }
            result.contribution_diff = status.final_stress - projection_->stress();
            if (result.distance > hemisphering_distance_threshold_) {
                // if (const auto real_contribution_diff = stress_.contribution(result.point_no, table_distances_for_point, layout.data()) - target_contribution;
                //     real_contribution_diff < hemisphering_stress_threshold_) {
                if (std::abs(result.contribution_diff) < hemisphering_stress_threshold_) {
                    // result.contribution_diff = real_contribution_diff;
                    result.diagnosis = Result::hemisphering;
                }
            }
        }
        // AD_DEBUG("GridTest {} area: {:8.1f} units^{}  grid-step: {:5.3f}", result.report(chart_), area.area(), area.num_dim(), grid_step_);
    }

} // ae::chart::v2::GridTest::test

// ----------------------------------------------------------------------

ae::chart::v2::GridTest::Results ae::chart::v2::GridTest::test(const std::vector<size_t>& points, [[maybe_unused]] int threads)
{
    Results results(points, *projection_);

#pragma omp parallel for default(none) shared(results) num_threads(threads <= 0 ? omp_get_max_threads() : threads) schedule(static, chart_.number_of_antigens() < 1000 ? 4 : 1)
    for (size_t entry_no = 0; entry_no < results.size(); ++entry_no) {
        test(results[entry_no]);
    }

    return results;

} // ae::chart::v2::GridTest::test

// ----------------------------------------------------------------------

ae::chart::v2::GridTest::Results ae::chart::v2::GridTest::test_all([[maybe_unused]] int threads)
{
    Results results(*projection_);

#pragma omp parallel for default(none) shared(results) num_threads(threads <= 0 ? omp_get_max_threads() : threads) schedule(static, chart_.number_of_antigens() < 1000 ? 4 : 1)
    for (size_t entry_no = 0; entry_no < results.size(); ++entry_no) {
        test(results[entry_no]);
    }

    return results;

} // ae::chart::v2::GridTest::test_all

// ----------------------------------------------------------------------

ae::chart::v2::ProjectionModifyP ae::chart::v2::GridTest::make_new_projection_and_relax(const Results& results, verbose verb)
{
    auto projection = chart_.projections_modify().new_by_cloning(*projection_);
    auto layout = projection->layout_modified();
    for (const auto& result : results) {
        if (result && result.contribution_diff < 0) {
            layout->update(result.point_no, result.pos);
        }
    }
    const auto status = optimize(optimization_method_, stress_, layout->data(), layout->data() + layout->size(), optimization_precision::fine);
    AD_INFO(verb, "stress: {} --> {}", projection_->stress(), status.final_stress);
    return projection;

} // ae::chart::v2::GridTest::make_new_projection_and_relax

// ----------------------------------------------------------------------

std::string ae::chart::v2::GridTest::Results::report() const
{
    size_t trapped = 0, hemi = 0;
    std::for_each(begin(), end(), [&](const auto& r) { if (r.diagnosis == Result::trapped) ++trapped; else if (r.diagnosis == Result::hemisphering) ++hemi; });
    if (trapped || hemi)
        return fmt::format("trapped:{} hemisphering:{}", trapped, hemi);
    else
        return "nothing found";

} // ae::chart::v2::GridTest::Results::report

// ----------------------------------------------------------------------

std::string ae::chart::v2::GridTest::Results::report(const ChartModify& chart, std::string_view pattern) const
{
    fmt::memory_buffer out;
    fmt::format_to(std::back_inserter(out), "{}\n", report());
    for (const auto& result : *this) {
        if (result)
            fmt::format_to(std::back_inserter(out), "{} {} diff:{:8.4f} dist:{:7.4f}\n", result.diagnosis_str(true), format_point(pattern, chart, result.point_no, collapse_spaces_t::yes),
                           result.contribution_diff, result.distance);
    }
    return fmt::to_string(out);

} // ae::chart::v2::GridTest::Results::report

// ----------------------------------------------------------------------

std::string ae::chart::v2::GridTest::Result::diagnosis_str(bool brief) const
{
    if (brief) {
        switch (diagnosis) {
            case excluded:
                return "excl";
            case not_tested:
                return "nott";
            case normal:
                return "norm";
            case trapped:
                return "trap";
            case hemisphering:
                return "hemi";
        }
    }
    else {
        switch (diagnosis) {
            case excluded:
                return "excluded";
            case not_tested:
                return "not_tested";
            case normal:
                return "normal";
            case trapped:
                return "trapped";
            case hemisphering:
                return "hemisphering";
        }
    }
    return "not_tested";

} // ae::chart::v2::GridTest::Result::diagnosis_str

// ----------------------------------------------------------------------

ae::chart::v2::GridTest::Results::Results(const Projection& projection)
    : std::vector<Result>(projection.number_of_points(), Result(0, projection.number_of_dimensions()))
{
    size_t point_no = 0;
    for (auto& res : *this)
        res.point_no = point_no++;
    exclude_disconnected(projection);

} // ae::chart::v2::GridTest::Results::Results

// ----------------------------------------------------------------------

ae::chart::v2::GridTest::Results::Results(const std::vector<size_t>& points, const Projection& projection)
    : std::vector<Result>(points.size(), Result(0, projection.number_of_dimensions()))
{
    for (auto [index, point_no] : acmacs::enumerate(points))
        at(index).point_no = point_no;
    exclude_disconnected(projection);

} // ae::chart::v2::GridTest::Results::Results

// ----------------------------------------------------------------------

void ae::chart::v2::GridTest::Results::exclude_disconnected(const Projection& projection)
{
    const auto exclude = [this](size_t point_no) {
        if (auto* found = find(point_no); found)
            found->diagnosis = Result::excluded;
    };

    for (auto unmovable : projection.unmovable())
        exclude(unmovable);
    for (auto disconnected : projection.disconnected())
        exclude(disconnected);
    erase(std::remove_if(begin(), end(), [](const auto& entry) { return entry.diagnosis == Result::excluded; }), end());

} // ae::chart::v2::GridTest::Results::exclude_disconnected

// ----------------------------------------------------------------------

std::string ae::chart::v2::GridTest::Results::export_to_json(const ChartModify& chart, size_t number_of_relaxations) const
{
    const auto export_point = [&chart](const auto& en) -> to_json::object {
        return to_json::object{
            to_json::key_val{"point_no", en.point_no},
            to_json::key_val{"name", en.point_no < chart.number_of_antigens() ? chart.antigen(en.point_no)->format("{name_full}") : chart.serum(en.point_no - chart.number_of_antigens())->format("{name_full}")},
            to_json::key_val{"distance", en.distance},
            to_json::key_val{"contribution_diff", en.contribution_diff},
            to_json::key_val{"pos", to_json::array(en.pos.begin(), en.pos.end())},
        };
    };

    to_json::array hemisphering, trapped, tested;
    for (const auto& en : *this) {
        tested << en.point_no;
        switch (en.diagnosis) {
          case Result::trapped:
              trapped << export_point(en);
              break;
          case Result::hemisphering:
              hemisphering << export_point(en);
              break;
          case Result::excluded:
          case Result::not_tested:
          case Result::normal:
              break;
        }
    }

    return fmt::format("{}\n", to_json::object{
            to_json::key_val{"  version", "grid-test-v1"},
            to_json::key_val{"chart", chart.make_name()},
            // to_json::key_val{"tested", std::move(tested)},
            to_json::key_val{"hemisphering", std::move(hemisphering)},
            to_json::key_val{"trapped", std::move(trapped)},
            to_json::key_val{"relaxed", number_of_relaxations}
        });

} // ae::chart::v2::GridTest::Results::export_to_json

// ----------------------------------------------------------------------

std::string ae::chart::v2::GridTest::Results::export_to_layout_csv(const ChartModify& chart, const Projection& projection) const
{
    using DF = acmacs::DataFormatterCSV;

    auto antigens = chart.antigens();
    const auto number_of_antigens = antigens->size();
    auto sera = chart.sera();
    auto layout = projection.layout();
    const auto number_of_dimensions = layout->number_of_dimensions();

    std::string result;

    const auto add = [&](size_t point_no) {
        for (number_of_dimensions_t dim{0}; dim < number_of_dimensions; ++dim)
            DF::second_field(result, (*layout)(point_no, dim));
        if (const auto* res = find(point_no); res) {
            DF::second_field(result, res->diagnosis_str());
            DF::second_field(result, res->distance);
            DF::second_field(result, res->contribution_diff);
            for (number_of_dimensions_t dim{0}; dim < number_of_dimensions; ++dim)
                DF::second_field(result, res->pos[dim]);
        }
        else {
            DF::second_field(result, "not-tested");
            DF::second_field(result, "");
            DF::second_field(result, "");
        }
    };

    DF::first_field(result, "");
    DF::second_field(result, "name");
    for (number_of_dimensions_t dim{0}; dim < number_of_dimensions; ++dim)
        DF::second_field(result, fmt::format("coord {}", dim));
    DF::second_field(result, "hemisphering");
    DF::second_field(result, "distance");
    DF::second_field(result, "contribution_diff");
    for (number_of_dimensions_t dim{0}; dim < number_of_dimensions; ++dim)
        DF::second_field(result, fmt::format("other coord {}", dim));
    DF::end_of_record(result);

    for (auto [ag_no, antigen] : acmacs::enumerate(*antigens)) {
        DF::first_field(result, "AG");
        DF::second_field(result, antigen->format("{name_full}"));
        add(ag_no);
        DF::end_of_record(result);
    }

    for (auto [sr_no, serum] : acmacs::enumerate(*sera)) {
        DF::first_field(result, "SR");
        DF::second_field(result, serum->format("{name_full}"));
        add(sr_no + number_of_antigens);
        DF::end_of_record(result);
    }

    return result;
} // ae::chart::v2::GridTest::Results::export_to_layout_csv

    // ----------------------------------------------------------------------

std::pair<ae::chart::v2::GridTest::Results, size_t> ae::chart::v2::grid_test(ChartModify& chart, size_t projection_no, double grid_step, int threads, size_t relax_attempts,
                                                                             std::string_view export_filename, verbose verb)
{
    const size_t total_attempts = relax_attempts ? relax_attempts : 1;
    size_t grid_projections = 0;
    GridTest::Results results;
    for (size_t attempt = 0; attempt < total_attempts; ++attempt) {
        GridTest test{chart, projection_no, grid_step};
        results = test.test_all(threads);
        AD_INFO(verb, "{}", results.report());
        for (const auto& result : results) {
            if (result)
                AD_LOG(ae::log::report_stresses, "{}", result.report(chart));
        }
        if (relax_attempts) {
            auto projection = test.make_new_projection_and_relax(results, verb);
            ++grid_projections;
            projection->comment(fmt::format("grid-test-{}", attempt));
            projection_no = projection->projection_no();
            if (ranges::all_of(results, [](const auto& result) { return result.diagnosis != GridTest::Result::trapped; }))
                break;
            // if (std::all_of(results.begin(), results.end(), [](const auto& result) { return result.diagnosis != GridTest::Result::trapped; }))
            //     break;
        }
    }
    chart.projections_modify().sort();

    if (!export_filename.empty())
        ae::file::write(export_filename, results.export_to_json(chart, grid_projections));

    return {results, grid_projections};

} // ae::chart::v2::grid_test

// ======================================================================
