#include "ext/omp.hh"
#include "chart/v3/chart.hh"
#include "chart/v3/stress.hh"
#include "chart/v3/randomizer.hh"
#include "chart/v3/optimize.hh"

#include "chart/v3/disconnected-points-handler.hh"
#include "chart/v3/selected-antigens-sera.hh"

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
            cb.add(mcb.apply(titers().raw_column_basis(sr_no)));
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

void ae::chart::v3::Chart::forced_column_bases(class column_bases& cb)
{
    if (!cb.empty()) {
        if (cb.size() != sera().size())
            throw Error{fmt::format("invalid number of entries in column_bases: {}, number of sera in the chart: {}", cb.size(), sera().size())};
        // forced column bases are stored with the sera
        for (const auto sr_no : sera().size())
            sera()[sr_no].forced_column_basis(cb[sr_no]);
    }

} // ae::chart::v3::Chart::forced_column_bases

// ----------------------------------------------------------------------

void ae::chart::v3::Chart::relax(number_of_optimizations_t number_of_optimizations, minimum_column_basis mcb, number_of_dimensions_t number_of_dimensions, const optimization_options& options, const disconnected_points& disconnected, const unmovable_points& unmovable)
{
    const auto start_num_dim = options.dimension_annealing == use_dimension_annealing::yes && number_of_dimensions < number_of_dimensions_t{5} ? number_of_dimensions_t{5} : number_of_dimensions;
    auto stress = stress_factory(*this, start_num_dim, mcb, disconnected, unmovable, options);
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
#pragma omp parallel for default(shared) num_threads(num_threads) firstprivate(stress) schedule(static, slot_size)
    for (size_t p_no = *first; p_no < *projections().size(); ++p_no) {
        auto& projection = projections()[projection_index{p_no}];
        projection.randomize_layout(*rnd);
        auto& layout = projection.layout();
        stress.change_number_of_dimensions(start_num_dim);
        const auto status1 =
            optimize(options.method, stress, layout.span(), start_num_dim > number_of_dimensions ? optimization_precision::rough : options.precision);
        // {
        //     DisconnectedPointsHandler disconnected_point_handler{stress, layout.span()};
        //     AD_DEBUG("final_stress: {}  stress: {} diff: {}", status1.final_stress, stress.value(layout.span()), status1.final_stress - stress.value(layout));
        // }
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

void ae::chart::v3::Chart::relax_incremental(projection_index source_projection_no, number_of_optimizations_t number_of_optimizations, const optimization_options& options, const disconnected_points& disconnected, const unmovable_points& unmovable)
{
    // cannot keep Projection& to the source bacause projection storage can be reallocated while adding new projections below
    const auto src = [this, source_projection_no](){ return projections()[source_projection_no]; };

    const auto num_dim = src().number_of_dimensions();
    const auto mcb = src().minimum_column_basis();

    disconnected_points my_disconnected{disconnected};
    if (options.disconnect_too_few_numeric_titers == disconnect_few_numeric_titers::yes)
        my_disconnected.insert_if_not_present(titers().having_too_few_numeric_titers());

    unmovable_points my_unmovable{unmovable};
    if (options.unnp == unmovable_non_nan_points::yes)
        my_unmovable.insert_if_not_present(src().non_nan_points());

    auto stress = stress_factory(*this, num_dim, mcb, my_disconnected, my_unmovable, options);

    if (const auto num_connected = antigens().size().get() + sera().size().get() - stress.number_of_disconnected(); num_connected < 3)
        throw std::runtime_error{AD_FORMAT("cannot relax: too few connected points: {}", num_connected)};
    // report_disconnected_unmovable(stress.parameters().disconnected, stress.parameters().unmovable);

    auto rnd = randomizer_plain_from_sample_optimization(*this, stress, num_dim, mcb, options.randomization_diameter_multiplier);

    point_indexes points_with_nan_coordinates;
    for (const auto p_no : src().layout().number_of_points()) {
        if (!src().layout().point_has_coordinates(p_no) && !disconnected.contains(p_no))
            points_with_nan_coordinates.insert(p_no);
    }

    const auto first = projections().size();
    for ([[maybe_unused]] const auto opt_no : number_of_optimizations) {
        auto& projection = projections().add(src());
        projection.disconnected() = stress.parameters().disconnected;
        projection.unmovable() = stress.parameters().unmovable;
    }

#ifdef _OPENMP
    const int num_threads = options.num_threads <= 0 ? omp_get_max_threads() : options.num_threads;
    const int slot_size = antigens().size() < antigen_index{1000} ? 4 : 1;
#endif
#pragma omp parallel for default(shared) num_threads(num_threads) firstprivate(stress) schedule(static, slot_size)
    for (size_t p_no = *first; p_no < *projections().size(); ++p_no) {
        auto& projection = projections()[projection_index{p_no}];
        projection.randomize_layout(points_with_nan_coordinates, *rnd);
        auto& layout = projection.layout();
        const auto status = optimize(options.method, stress, layout.span(), optimization_precision::rough);
        if (!std::isnan(status.final_stress))
            projection.stress(status.final_stress);
    }

    if (options.rsp == remove_source_projection::yes)
        projections().remove(source_projection_no);
    projections().sort(*this);

    if (options.precision == optimization_precision::fine) {
        const projection_index top_projections{std::min(5UL, *number_of_optimizations)};
#pragma omp parallel for default(shared) num_threads(num_threads) schedule(static, slot_size)
        for (size_t p_no = 0; p_no < *top_projections; ++p_no) {
            // for (const auto p_no : top_projections)
            projections()[projection_index{p_no}].relax(*this, options); // do not omp parallel, occasionally fails
        }
        projections().sort(*this);
    }

} // ae::chart::v3::Chart::relax_incremental

// ----------------------------------------------------------------------

void ae::chart::v3::Chart::combine_projections(const Chart& merge_in)
{
    if (antigens() != merge_in.antigens() || sera() != merge_in.sera() || titers() != merge_in.titers())
        throw std::runtime_error{AD_FORMAT("cannot combine projections, tables are not the same")};
    for (const auto& proj : projections())
        projections().add(proj);
    projections().sort(*this);

} // ae::chart::v3::Chart::combine_projections

// ----------------------------------------------------------------------

void ae::chart::v3::Chart::duplicates_distinct()
{
    antigens().duplicates_distinct(antigens().find_duplicates());
    sera().duplicates_distinct(sera().find_duplicates());

} // ae::chart::v3::Chart::duplicates_distinct

// ----------------------------------------------------------------------

void ae::chart::v3::Chart::throw_if_duplicates() const
{
    const auto dupa = antigens().find_duplicates();
    const auto dups = sera().find_duplicates();

    if (!dupa.empty() || !dups.empty()) {
        fmt::memory_buffer output;
        fmt::format_to(std::back_inserter(output), "\"{}\" has duplicates: AG:{} SR:{}\n", name(), dupa.size(), dups.size());
        for (const auto& ags : dupa) {
            for (const auto ag_no : ags)
                fmt::format_to(std::back_inserter(output), "  AG {:5d} {}\n", ag_no, antigens()[ag_no].designation());
            fmt::format_to(std::back_inserter(output), "\n");
        }
        for (const auto& srs : dups) {
            for (const auto sr_no : srs)
                fmt::format_to(std::back_inserter(output), "  SR {:5d} {}\n", sr_no, sera()[sr_no].designation());
            fmt::format_to(std::back_inserter(output), "\n");
        }
        throw Error{fmt::to_string(output)};
    }

    // if (!dupa.empty()) {
    //     if (!dups.empty())
    //         throw Error{"{}: duplicating antigens: {}  duplicating sera: {}", name(), dupa, dups};
    //     else
    //         throw Error{"{}: duplicating antigens: {}", name(), dupa};
    // }
    // else if (!dups.empty())
    //     throw Error{"{}: duplicating sera: {}", name(), dups};

} // ae::chart::v3::Chart::throw_if_duplicates

// ----------------------------------------------------------------------

ae::antigen_indexes ae::chart::v3::Chart::reference() const
{
    antigen_indexes indexes;
    for (const auto ag_no : antigens().size()) {
        const auto& ag = antigens()[ag_no];
        if (!ag.annotations().distinct()) {
            for (const auto& sr : sera()) {
                if (ag.name() == sr.name() && ag.annotations() == sr.annotations()) {
                    indexes.push_back(ag_no);
                    break;
                }
            }
        }
    }
    return indexes;

} // ae::chart::v3::Chart::reference

// ----------------------------------------------------------------------

ae::antigen_indexes ae::chart::v3::Chart::test() const // inversion of the reference() list
{
    const auto ref = reference();
    antigen_indexes indexes;
    for (const auto ag_no : antigens().size()) {
        if (!ref.contains(ag_no))
            indexes.push_back(ag_no);
    }
    return indexes;

} // ae::chart::v3::Chart::test

// ----------------------------------------------------------------------

ae::sequences::lineage_t ae::chart::v3::Chart::lineage() const // major lineage
{
    const auto present_lineages = lineages();
    if (!present_lineages.empty() && info().type_subtype().h_or_b() != "B")
        AD_WARNING("Lineages for \"{}\": {}", info().type_subtype(), present_lineages);
    const auto max = std::max_element(std::begin(present_lineages), std::end(present_lineages), [](const auto& en1, const auto& en2) { return en1.second < en2.second; });
    if (max != std::end(present_lineages))
        return max->first;
    else
        return sequences::lineage_t{};

} // ae::chart::v3::Chart::lineage

// ----------------------------------------------------------------------

std::unordered_map<ae::sequences::lineage_t, size_t, ae::sequences::lineage_t_hash_for_unordered_map, std::equal_to<>> ae::chart::v3::Chart::lineages() const // lineage to antigen count
{
    std::unordered_map<sequences::lineage_t, size_t, ae::sequences::lineage_t_hash_for_unordered_map, std::equal_to<>> lineages;
    for (const auto& antigen : antigens()) {
        if (const auto& lineage = antigen.lineage(); !lineage.empty()) {
            // AD_DEBUG("{} --> lineage {}", antigen.designation(), lineage);
            ++lineages.try_emplace(lineage, 0).first->second;
        }
    }
    return lineages;

} // ae::chart::v3::Chart::lineages

// ----------------------------------------------------------------------

void ae::chart::v3::Chart::remove_antigens(const SelectedAntigens& to_remove)
{
    AD_DEBUG("remove_antigens {}", to_remove);
    antigens().remove(to_remove.indexes);
    titers().remove_antigens(to_remove.indexes);
    const auto points_to_remove = to_point_indexes(to_remove.indexes);
    // projections().remove_points(points_to_remove);
    styles().clear();
    // legacy_plot_spec().remove_points(points_to_remove);
    throw std::runtime_error("ae::chart::v3::Chart::remove_antigens not implemented");

} // ae::chart::v3::Chart::remove_antigens

// ----------------------------------------------------------------------

void ae::chart::v3::Chart::remove_sera(const SelectedSera& to_remove)
{
    sera().remove(to_remove.indexes);
    titers().remove_sera(to_remove.indexes);
    const auto points_to_remove = to_point_indexes(to_remove.indexes, antigens().size());
    // projections().remove_points(points_to_remove);
    styles().clear();
    // legacy_plot_spec().remove_points(points_to_remove);
    throw std::runtime_error("ae::chart::v3::Chart::remove_sera not implemented");

} // ae::chart::v3::Chart::remove_sera

// ----------------------------------------------------------------------
