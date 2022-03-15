#include "ext/omp.hh"
#include "chart/v3/chart.hh"
#include "chart/v3/stress.hh"
#include "chart/v3/randomizer.hh"
#include "chart/v3/optimize.hh"

#include "chart/v3/disconnected-points-handler.hh"

// ----------------------------------------------------------------------

std::string ae::chart::v3::Chart::name(std::optional<projection_index> aProjectionNo) const
{
    fmt::memory_buffer name;
    if (!info().name().empty()) {
        fmt::format_to(std::back_inserter(name), "{}", info().name());
    }
    else {
        fmt::format_to(std::back_inserter(name), "{}",
                       ae::string::join(" ", info().make_lab(), info().make_virus_not_influenza(), info().make_virus_type(), info().make_assay(Assay::assay_name_t::no_HI), info().make_rbc_species(),
                                        info().make_date()));
    }
    if (const auto& prjs = projections(); !prjs.empty() && aProjectionNo.has_value() && *aProjectionNo < prjs.size()) {
        const auto& prj = prjs[*aProjectionNo];
        fmt::format_to(std::back_inserter(name), "{}", prj.minimum_column_basis().format(" >={}", minimum_column_basis::use_none::no));
        if (const auto stress = prj.stress(); !std::isnan(stress))
            fmt::format_to(std::back_inserter(name), " {}d {:.4f}", prj.number_of_dimensions(), stress);
    }
    return fmt::to_string(name);

} // ae::chart::v3::Chart::name

// ----------------------------------------------------------------------

std::string ae::chart::v3::Chart::name_for_file() const
{
    std::string name;
    if (!info().name().empty())
        name = info().name();
    else
        name = ae::string::join("-", info().make_virus_not_influenza(), info().make_virus_subtype(), info().make_assay(Assay::assay_name_t::brief), info().make_rbc_species(), info().make_lab(), info().make_date(Info::include_number_of_tables::no));
    ae::string::replace_in_place(name, '/', '-');
    ae::string::replace_in_place(name, ' ', '-');
    ae::string::lowercase_in_place(name);
    return name;

} // ae::chart::v3::Chart::name_for_file

// ----------------------------------------------------------------------

ae::chart::v3::column_bases ae::chart::v3::Chart::column_bases(minimum_column_basis mcb) const
{
    // forced column bases are stored with the sera
    class column_bases cb;
    for (const auto sr_no : sera().size()) {
        if (const auto fcb = sera()[sr_no].forced_column_basis(); fcb.has_value())
            cb.add(mcb.apply(*fcb));
        else
            cb.add(mcb.apply(titers().column_basis(sr_no)));
    }
    return cb;

} // ae::chart::v3::Chart::column_bases

// ----------------------------------------------------------------------

ae::chart::v3::column_bases ae::chart::v3::Chart::forced_column_bases() const
{
    // forced column bases are stored with the sera
    class column_bases cb;
    for (const auto sr_no : sera().size()) {
        if (const auto fcb = sera()[sr_no].forced_column_basis(); fcb.has_value())
            cb.add(*fcb);
        else
            cb.add(0.0);
    }
    if (std::all_of(cb.begin(), cb.end(), [](double cbs) { return float_zero(cbs); }))
        return ae::chart::v3::column_bases{};
    else
        return cb;

} // ae::chart::v3::Chart::forced_column_bases

// ----------------------------------------------------------------------

void ae::chart::v3::Chart::relax(number_of_optimizations_t number_of_optimizations, minimum_column_basis mcb, number_of_dimensions_t number_of_dimensions, const optimization_options& options, const disconnected_points& disconnected)
{
    const auto start_num_dim = options.dimension_annealing == use_dimension_annealing::yes && number_of_dimensions < number_of_dimensions_t{5} ? number_of_dimensions_t{5} : number_of_dimensions;
    // auto titrs = titers();
    auto stress = stress_factory(*this, start_num_dim, mcb, options.mult, dodgy_titer_is_regular_e::no);
    stress.set_disconnected(disconnected);
    if (options.disconnect_too_few_numeric_titers == disconnect_few_numeric_titers::yes)
        stress.extend_disconnected(titers().having_too_few_numeric_titers());
    if (const auto num_connected = antigens().size().get() + sera().size().get() - stress.number_of_disconnected(); num_connected < 3)
        throw std::runtime_error{AD_FORMAT("cannot relax: too few connected points: {}", num_connected)};
    // report_disconnected_unmovable(stress.parameters().disconnected, stress.parameters().unmovable);
    auto rnd = randomizer_plain_from_sample_optimization(*this, stress, start_num_dim, mcb, options.randomization_diameter_multiplier);

    const auto first = projections().size();
    for ([[maybe_unused]] const auto opt_no : number_of_optimizations) {
        auto& projection = projections().add(number_of_points(), start_num_dim, mcb);
        projection.disconnected() = stress.parameters().disconnected;
        projection.unmovable() = stress.parameters().unmovable;
    }

#ifdef _OPENMP
    const int num_threads = options.num_threads <= 0 ? omp_get_max_threads() : options.num_threads;
    const int slot_size = antigens().size() < antigen_index{1000} ? 4 : 1;
#endif
// #pragma omp parallel for default(shared) num_threads(num_threads) firstprivate(stress) schedule(static, slot_size)
    for (size_t p_no = *first; p_no < *projections().size(); ++p_no) {
        auto& projection = projections()[projection_index{p_no}];
        projection.randomize_layout(rnd);
        auto& layout = projection.layout();
        stress.change_number_of_dimensions(start_num_dim);
        const auto status1 =
            optimize(options.method, stress, layout.span(), start_num_dim > number_of_dimensions ? optimization_precision::rough : options.precision);
        {
            DisconnectedPointsHandler disconnected_point_handler{stress, layout.span()};
            AD_DEBUG("final_stress: {}  stress: {} diff: {}", status1.final_stress, stress.value(layout.span()), status1.final_stress - stress.value(layout));
        }
        if (start_num_dim > number_of_dimensions) {
            do_dimension_annealing(options.method, stress, projection.number_of_dimensions(), number_of_dimensions, layout.span());
            layout.change_number_of_dimensions(number_of_dimensions);
            stress.change_number_of_dimensions(number_of_dimensions);
            const auto status2 = optimize(options.method, stress, layout.span(), options.precision);
            if (!std::isnan(status2.final_stress))
                projection.stress(status2.final_stress);
        }
        else {
            if (!std::isnan(status1.final_stress))
                projection.stress(status1.final_stress);
        }
        projection.transformation_reset();
        // AD_DEBUG("{:3d} {:.4f}", p_no, projection.stress());
    }

} // ae::chart::v3::Chart::relax

// ----------------------------------------------------------------------

void ae::chart::v3::Chart::relax_incremental(projection_index source_projection_no, number_of_optimizations_t number_of_optimizations, const optimization_options& options)
{

} // ae::chart::v3::Chart::relax_incremental

// ----------------------------------------------------------------------
