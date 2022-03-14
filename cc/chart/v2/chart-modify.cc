#include <random>
#include <functional>
#include <limits>
#include <algorithm>

#include "utils/log.hh"
#include "ext/omp.hh"
#include "ad/flat-set.hh"
#include "ext/range-v3.hh"
#include "ad/enumerate.hh"
#include "utils/string.hh"
#include "virus/name-parse.hh"
#include "utils/statistics.hh"
#include "locdb/v3/locdb.hh"
#include "chart/v2/chart-modify.hh"

using namespace std::string_literals;
using namespace ae::chart::v2;

// ----------------------------------------------------------------------

ChartModify::ChartModify(size_t number_of_antigens, size_t number_of_sera)
    : info_{std::make_shared<InfoModify>()},
      antigens_{std::make_shared<AntigensModify>(number_of_antigens)},
      sera_{std::make_shared<SeraModify>(number_of_sera)},
      titers_{std::make_shared<TitersModify>(number_of_antigens, number_of_sera)},
      projections_{std::make_shared<ProjectionsModify>(*this)},
      plot_spec_{std::make_shared<PlotSpecModify>(number_of_antigens, number_of_sera)}
{
} // ChartModify::ChartModify

// ----------------------------------------------------------------------

ChartModify::ChartModify(const Chart& source, bool copy_projections, bool copy_plot_spec)
    : info_{std::make_shared<InfoModify>(source.info())},
      antigens_{std::make_shared<AntigensModify>(source.antigens())},
      sera_{std::make_shared<SeraModify>(source.sera())},
      titers_{std::make_shared<TitersModify>(source.titers())},
      projections_{copy_projections ? std::make_shared<ProjectionsModify>(*this, source.projections()) : std::make_shared<ProjectionsModify>(*this)},
      plot_spec_{copy_plot_spec ? std::make_shared<PlotSpecModify>(source.plot_spec(), source.number_of_antigens()) : std::make_shared<PlotSpecModify>(source.number_of_antigens(), source.number_of_sera())}
{
} // ChartModify::ChartModify

// ----------------------------------------------------------------------

InfoP ChartModify::info() const
{
    if (info_)
        return info_;
    else
        return main_->info();

} // ChartModify::info

// ----------------------------------------------------------------------

InfoModify& ChartModify::info_modify()
{
    if (!info_)
        info_ = std::make_shared<InfoModify>(main_->info());
    return *info_;

} // ChartModify::info_modify

// ----------------------------------------------------------------------

AntigensP ChartModify::antigens() const
{
    if (antigens_)
        return antigens_;
    else
        return main_->antigens();

} // ChartModify::antigens

// ----------------------------------------------------------------------

std::shared_ptr<AntigensModify> ChartModify::antigens_modify_ptr()
{
    if (!antigens_)
        antigens_ = std::make_shared<AntigensModify>(main_->antigens());
    return antigens_;

} // ChartModify::antigens_modify_ptr

// ----------------------------------------------------------------------

AntigensModify& ChartModify::antigens_modify()
{
    return *antigens_modify_ptr();
}

// ----------------------------------------------------------------------

SeraP ChartModify::sera() const
{
    if (sera_)
        return sera_;
    else
        return main_->sera();

} // ChartModify::sera

// ----------------------------------------------------------------------

std::shared_ptr<SeraModify> ChartModify::sera_modify_ptr()
{
    if (!sera_)
        sera_ = std::make_shared<SeraModify>(main_->sera());
    return sera_;

} // ChartModify::sera_modify_ptr

// ----------------------------------------------------------------------

TitersP ChartModify::titers() const
{
    if (titers_)
        return titers_;
    else
        return main_->titers();

} // ChartModify::titers

// ----------------------------------------------------------------------

TitersModify& ChartModify::titers_modify()
{
    if (!titers_)
        titers_ = std::make_shared<TitersModify>(main_->titers());
    return *titers_;

} // ChartModify::titers_modify

// ----------------------------------------------------------------------

ProjectionsP ChartModify::projections() const
{
    if (projections_)
        return projections_;
    else
        return main_->projections();

} // ChartModify::projections

// ----------------------------------------------------------------------

PlotSpecP ChartModify::plot_spec() const
{
    if (plot_spec_)
        return plot_spec_;
    else
        return main_->plot_spec();

} // ChartModify::plot_spec

// ----------------------------------------------------------------------

ColumnBasesP ChartModify::forced_column_bases(MinimumColumnBasis aMinimumColumnBasis) const
{
    if (forced_column_bases_)
        return forced_column_bases_;
    else if (main_)
        return main_->forced_column_bases(aMinimumColumnBasis);
    else
        return {};

} // ChartModify::forced_column_bases

// ----------------------------------------------------------------------

std::shared_ptr<ColumnBasesModify> ChartModify::forced_column_bases_modify(MinimumColumnBasis aMinimumColumnBasis)
{
    if (!forced_column_bases_ && main_) {
        if (auto fcb = main_->forced_column_bases(aMinimumColumnBasis); fcb)
            forced_column_bases_ = std::make_shared<ColumnBasesModify>(fcb);
    }
    return forced_column_bases_;

} // ChartModify::forced_column_bases_modify

// ----------------------------------------------------------------------

ColumnBasesModify& ChartModify::forced_column_bases_modify(const ColumnBases& source)
{
    forced_column_bases_ = std::make_shared<ColumnBasesModify>(source);
    return *forced_column_bases_;

} // ChartModify::forced_column_bases_modify

// ----------------------------------------------------------------------

const rjson::value& ChartModify::extension_fields() const
{
    if (extensions_.is_null() && main_)
        return main_->extension_fields();
    else
        return extensions_;

} // ChartModify::extension_fields

// ----------------------------------------------------------------------

const rjson::value& ChartModify::extension_field(std::string_view field_name) const
{
    if (extensions_.is_null() && main_)
        return main_->extension_field(field_name);
    else
        return extensions_.get(field_name);

} // ChartModify::extension_field

// ----------------------------------------------------------------------

const rjson::value& ChartModify::extension_field_modify(std::string field_name)
{
    if (extensions_.is_null()) {
        if (const auto& ef = main_ ? main_->extension_fields() : rjson::ConstNull; !ef.is_null())
            extensions_ = ef;
        else
            extensions_ = rjson::object{};
    }
    return extensions_.get(field_name);

} // ChartModify::extension_field_modify

// ----------------------------------------------------------------------

void ChartModify::extension_field_modify(std::string field_name, const rjson::value& value)
{
    extension_field_modify(field_name);
    extensions_.set(field_name) = value;

} // ChartModify::extension_field_modify

// ----------------------------------------------------------------------

ProjectionsModify& ChartModify::projections_modify()
{
    if (!projections_)
        projections_ = std::make_shared<ProjectionsModify>(*this, main_->projections());
    return *projections_;

} // ChartModify::projections_modify

// ----------------------------------------------------------------------

ProjectionModifyP ChartModify::projection_modify(size_t aProjectionNo)
{
    return projections_modify().at(aProjectionNo);

} // ChartModify::projection_modify

// ----------------------------------------------------------------------

PlotSpecModify& ChartModify::plot_spec_modify()
{
    if (!plot_spec_)
        plot_spec_ = std::make_shared<PlotSpecModify>(main_->plot_spec(), number_of_antigens());
    return *plot_spec_;

} // ChartModify::plot_spec_modify

// ----------------------------------------------------------------------

void ChartModify::report_disconnected_unmovable(const DisconnectedPoints& disconnected, const UnmovablePoints& unmovable) const
{
    if (!disconnected.empty())
        AD_LOG(ae::log::relax, "(ag:{} sr:{}) Disconnected: {}", number_of_antigens(), number_of_sera(), disconnected);
    if (!unmovable.empty())
        AD_LOG(ae::log::relax, "(ag:{} sr:{}) Unmovable: {}", number_of_antigens(), number_of_sera(), unmovable);

} // ChartModify::report_disconnected_unmovable

// ----------------------------------------------------------------------

bool ChartModify::has_sequences() const
{
    auto ags = antigens();
    return std::any_of(ags->begin(), ags->end(), [](const auto& ag) { return !ag->sequence_aa().empty() || !ag->sequence_nuc().empty(); });

} // ChartModify::has_sequences

// ----------------------------------------------------------------------

std::pair<optimization_status, ProjectionModifyP> ChartModify::relax(MinimumColumnBasis minimum_column_basis, number_of_dimensions_t number_of_dimensions, use_dimension_annealing dimension_annealing, const optimization_options& options, LayoutRandomizer::seed_t seed, const DisconnectedPoints& disconnect_points)
{
    const auto start = std::chrono::high_resolution_clock::now();
    const auto start_num_dim = dimension_annealing == use_dimension_annealing::yes && *number_of_dimensions < 5 ? number_of_dimensions_t{5} : number_of_dimensions;
    auto projection = projections_modify().new_from_scratch(start_num_dim, minimum_column_basis);
    projection->set_disconnected(disconnect_points);
    projection->disconnect_having_too_few_numeric_titers(options, *titers());
    report_disconnected_unmovable(projection->get_disconnected(), projection->get_unmovable());
    auto layout = projection->layout_modified();
    auto stress = stress_factory(*projection, options.mult);
    if (const auto num_connected = projection->layout_modified()->number_of_points() - stress.number_of_disconnected(); num_connected < 3)
        throw std::runtime_error{AD_FORMAT("cannot relax projection: too few connected points: {}", num_connected)};
    auto rnd = randomizer_plain_from_sample_optimization(*projection, stress, options.randomization_diameter_multiplier, seed);
    projection->randomize_layout(rnd);
    auto status = optimize(options.method, stress, layout->data(), layout->data() + layout->size(), optimization_precision::rough);
    if (start_num_dim > number_of_dimensions) {
        do_dimension_annealing(options.method, stress, projection->number_of_dimensions(), number_of_dimensions, layout->data(), layout->data() + layout->size());
        layout->change_number_of_dimensions(number_of_dimensions);
        stress.change_number_of_dimensions(number_of_dimensions);
        const auto status2 = optimize(options.method, stress, layout->data(), layout->data() + layout->size(), options.precision);
        status.number_of_iterations += status2.number_of_iterations;
        status.number_of_stress_calculations += status2.number_of_stress_calculations;
        status.termination_report = status2.termination_report;
        status.initial_stress = status2.initial_stress;
        status.final_stress = status2.final_stress;
    }
    if (!std::isnan(status.final_stress))
        projection->stress_ = status.final_stress;
    projection->transformation_reset();
    status.time = std::chrono::duration_cast<decltype(status.time)>(std::chrono::high_resolution_clock::now() - start);
    return {status, projection};

} // ChartModify::relax

// ----------------------------------------------------------------------

void ChartModify::relax(number_of_optimizations_t number_of_optimizations, MinimumColumnBasis minimum_column_basis, number_of_dimensions_t number_of_dimensions,
                        use_dimension_annealing dimension_annealing, const optimization_options& options, const DisconnectedPoints& disconnect_points)
{
    const auto start_num_dim = dimension_annealing == use_dimension_annealing::yes && *number_of_dimensions < 5 ? number_of_dimensions_t{5} : number_of_dimensions;
    auto titrs = titers();
    auto stress = stress_factory(*this, start_num_dim, minimum_column_basis, options.mult, dodgy_titer_is_regular::no);
    stress.set_disconnected(disconnect_points);
    if (options.disconnect_too_few_numeric_titers == disconnect_few_numeric_titers::yes)
        stress.extend_disconnected(titrs->having_too_few_numeric_titers());
    if (const auto num_connected = number_of_antigens() + number_of_sera() - stress.number_of_disconnected(); num_connected < 3)
        throw std::runtime_error{AD_FORMAT("cannot relax: too few connected points: {}", num_connected)};
    report_disconnected_unmovable(stress.parameters().disconnected, stress.parameters().unmovable);
    auto rnd = randomizer_plain_from_sample_optimization(*this, stress, start_num_dim, minimum_column_basis, options.randomization_diameter_multiplier);

    std::vector<std::shared_ptr<ProjectionModifyNew>> projections(*number_of_optimizations);
    std::transform(projections.begin(), projections.end(), projections.begin(), [start_num_dim, minimum_column_basis, this, &stress](const auto&) {
        auto projection = projections_modify().new_from_scratch(start_num_dim, minimum_column_basis);
        projection->set_disconnected(stress.parameters().disconnected);
        projection->set_unmovable(stress.parameters().unmovable);
        return projection;
    });

#ifdef _OPENMP
    const int num_threads = options.num_threads <= 0 ? omp_get_max_threads() : options.num_threads;
    const int slot_size = number_of_antigens() < 1000 ? 4 : 1;
#endif
#pragma omp parallel for default(shared) num_threads(num_threads) firstprivate(stress) schedule(static, slot_size)
    for (size_t p_no = 0; p_no < projections.size(); ++p_no) {
        auto projection = projections[p_no];
        projection->randomize_layout(rnd);
        auto layout = projection->layout_modified();
        stress.change_number_of_dimensions(start_num_dim);
        const auto status1 =
            optimize(options.method, stress, layout->data(), layout->data() + layout->size(), start_num_dim > number_of_dimensions ? optimization_precision::rough : options.precision);
        if (start_num_dim > number_of_dimensions) {
            do_dimension_annealing(options.method, stress, projection->number_of_dimensions(), number_of_dimensions, layout->data(), layout->data() + layout->size());
            layout->change_number_of_dimensions(number_of_dimensions);
            stress.change_number_of_dimensions(number_of_dimensions);
            const auto status2 = optimize(options.method, stress, layout->data(), layout->data() + layout->size(), options.precision);
            if (!std::isnan(status2.final_stress))
                projection->stress_ = status2.final_stress;
        }
        else {
            if (!std::isnan(status1.final_stress))
                projection->stress_ = status1.final_stress;
        }
        projection->transformation_reset();
        AD_LOG(ae::log::report_stresses, "{:3d} {:.4f}", p_no, *projection->stress_);
    }

} // ChartModify::relax

// ----------------------------------------------------------------------

void ChartModify::relax_projections(const optimization_options& options, size_t first_projection_no, const DisconnectedPoints& disconnect_points)
{
    auto titrs = titers();
    auto& projections = projections_modify();

    auto first_projection = projections.at(first_projection_no);
    auto rnd = randomizer_plain_from_sample_optimization(*this, stress_factory(*first_projection, options.mult), first_projection->number_of_dimensions(), first_projection->minimum_column_basis(), options.randomization_diameter_multiplier);

#ifdef _OPENMP
    const int num_threads = options.num_threads <= 0 ? omp_get_max_threads() : options.num_threads;
    const int slot_size = number_of_antigens() < 1000 ? 4 : 1;
#endif
#pragma omp parallel for default(shared) num_threads(num_threads) firstprivate(rnd) schedule(static, slot_size)
    for (size_t p_no = first_projection_no; p_no < projections.size(); ++p_no) {
        auto projection = projections.at(p_no);

        auto stress = stress_factory(*projection, options.mult);
        stress.set_disconnected(disconnect_points);
        if (options.disconnect_too_few_numeric_titers == disconnect_few_numeric_titers::yes)
            stress.extend_disconnected(titrs->having_too_few_numeric_titers());
        if (const auto num_connected = number_of_antigens() + number_of_sera() - stress.number_of_disconnected(); num_connected < 3)
            throw std::runtime_error{AD_FORMAT("cannot relax: too few connected points: {}", num_connected)};

        projection->randomize_layout(rnd);
        projection->set_disconnected(stress.parameters().disconnected);
        projection->set_unmovable(stress.parameters().unmovable);
        auto layout = projection->layout_modified();
        const auto status1 =
            optimize(options.method, stress, layout->data(), layout->data() + layout->size(), options.precision);
        if (!std::isnan(status1.final_stress))
            projection->stress_ = status1.final_stress;
        projection->transformation_reset();
        // if (p_no < (first_projection_no + 5))
        // AD_DEBUG("stress {}: {}", p_no, *projection->stress_);
        AD_LOG(ae::log::report_stresses, "{:3d} {:.4f}", p_no, *projection->stress_);
    }

} // ChartModify::relax_all_projections

// ----------------------------------------------------------------------

void ChartModify::relax_incremental(size_t source_projection_no, number_of_optimizations_t number_of_optimizations, const optimization_options& options, remove_source_projection rsp,
                                    unmovable_non_nan_points unnp)
{
    auto source_projection = projection_modify(source_projection_no);
    const auto num_dim = source_projection->number_of_dimensions();
    const auto minimum_column_basis = source_projection->minimum_column_basis();
    auto stress = stress_factory(*this, num_dim, minimum_column_basis, options.mult, dodgy_titer_is_regular::no);

    // source_projection->modify();
    const UnmovablePoints unmovable_points{unnp == unmovable_non_nan_points::yes ? source_projection->non_nan_points() : PointIndexList{}};
    if (!unmovable_points.empty()) {
        // AD_DEBUG("relax_incremental unmovable_points: {}", unmovable_points);
        stress.set_unmovable(unmovable_points);
    }

    DisconnectedPoints disconnected_points;
    // remove unmovable points from disconnected, it affects stress
    // value but keeping them is ambiguous: if after merge one of the
    // unmovable point (i.e. from the primary chart) has too few
    // numreic titers, disconnecting it leads to erasing its
    // coordinate, this leaves of impression of a bug in the program
    // which is hard to catch.
    if (!disconnected_points.empty()) {
        if (!unmovable_points.empty())
            disconnected_points.remove(ReverseSortedIndexes{*unmovable_points});
        stress.set_disconnected(disconnected_points);
    }
    if (options.disconnect_too_few_numeric_titers == disconnect_few_numeric_titers::yes) {
        auto to_disconnect = titers()->having_too_few_numeric_titers();
        if (!unmovable_points.empty())
            to_disconnect.remove(ReverseSortedIndexes{*unmovable_points});
        stress.extend_disconnected(to_disconnect);
    }

    if (const auto num_connected = number_of_antigens() + number_of_sera() - stress.number_of_disconnected(); num_connected < 3)
        throw std::runtime_error{AD_FORMAT("cannot relax: too few connected points: {}", num_connected)};
    report_disconnected_unmovable(stress.parameters().disconnected, stress.parameters().unmovable);

    // AD_DEBUG("relax_incremental: {}", number_of_points());
    auto rnd = randomizer_plain_from_sample_optimization(*this, stress, num_dim, minimum_column_basis, options.randomization_diameter_multiplier);

    auto make_points_with_nan_coordinates = [&source_projection, &disconnected_points]() -> PointIndexList {
        PointIndexList result;
        auto source_layout = source_projection->layout();
        for (size_t p_no = 0; p_no < source_layout->number_of_points(); ++p_no) {
            if (!source_layout->point_has_coordinates(p_no) && !disconnected_points.contains(p_no))
                result.insert(p_no);
        }
        return result;
    };
    const auto points_with_nan_coordinates = make_points_with_nan_coordinates();
    // AD_INFO("about to randomize coordinates of {} points", points_with_nan_coordinates.size());

    std::vector<std::shared_ptr<ProjectionModifyNew>> projections(*number_of_optimizations);
    std::transform(projections.begin(), projections.end(), projections.begin(), [&stress, &source_projection, this](const auto&) {
        auto projection = projections_modify().new_by_cloning(*source_projection);
        projection->set_disconnected(stress.parameters().disconnected);
        projection->set_unmovable(stress.parameters().unmovable);
        return projection;
    });

#ifdef _OPENMP
    const int num_threads = options.num_threads <= 0 ? omp_get_max_threads() : options.num_threads;
    const int slot_size = number_of_antigens() < 1000 ? 4 : 1;
#endif
#pragma omp parallel for default(shared) num_threads(num_threads) firstprivate(stress) schedule(static, slot_size)
    for (size_t p_no = 0; p_no < projections.size(); ++p_no) {
        auto projection = projections[p_no];
        projection->randomize_layout(points_with_nan_coordinates, rnd);
        auto layout = projection->layout_modified();
        const auto status = optimize(options.method, stress, layout->data(), layout->data() + layout->size(), optimization_precision::rough);
        if (!std::isnan(status.final_stress))
            projection->stress_ = status.final_stress;
    }

    if (rsp == remove_source_projection::yes)
        projections_modify().remove(source_projection_no);
    projections_modify().sort();

    if (options.precision == optimization_precision::fine) {
        const size_t top_projections = std::min(5UL, *number_of_optimizations);
        for (size_t p_no = 0; p_no < top_projections; ++p_no)
            projections_modify().at(p_no)->relax(options); // do not omp parallel, occasionally fails
        projections_modify().sort();
    }

} // ChartModify::relax_incremental

// ----------------------------------------------------------------------

void ChartModify::remove_layers()
{
    titers_modify().remove_layers();
    info_modify().remove_sources();

} // ChartModify::remove_layers

// ----------------------------------------------------------------------

void ChartModify::remove_antigens(const ReverseSortedIndexes& indexes)
{
    // obtain *_modify pointers before removing antigens
    // otherwise they are incrrectly created with the new number of antigens
    auto& antigens_mod = antigens_modify();
    auto& titers_mod = titers_modify();
    auto& projections_mod = projections_modify();
    auto& plot_spec_mod = plot_spec_modify();

    antigens_mod.remove(indexes);
    titers_mod.remove_antigens(indexes);
    projections_mod.remove_antigens(indexes);
    plot_spec_mod.remove_antigens(indexes);

} // ChartModify::remove_antigens

// ----------------------------------------------------------------------

void ChartModify::remove_sera(const ReverseSortedIndexes& indexes)
{
    sera_modify().remove(indexes);
    titers_modify().remove_sera(indexes);
    projections_modify().remove_sera(indexes, number_of_antigens());
    plot_spec_modify().remove_sera(indexes);
    if (auto fcb = forced_column_bases_modify(MinimumColumnBasis{}); fcb)
        fcb->remove(indexes);

} // ChartModify::remove_sera

// ----------------------------------------------------------------------

constexpr const char* const sNames[] = {"Eddard Stark", "Robert Baratheon", "Jaime Lannister", "Catelyn Stark", "Cersei Lannister", "Daenerys Targaryen", "Jorah Mormont", "Petyr Baelish", "Viserys Targaryen", "Jon Snow", "Sansa Stark", "Arya Stark", "Robb Stark", "Theon Greyjoy", "Bran Stark", "Joffrey Baratheon", "Sandor Clegane", "Tyrion Lannister", "Khal Drogo", "Tywin Lannister", "Davos Seaworth", "Samwell Tarly", "Margaery Tyrell", "Stannis Baratheon", "Melisandre", "Jeor Mormont", "Bronn", "Varys", "Shae", "Ygritte", "Talisa Maegyr", "Gendry", "Tormund Giantsbane", "Gilly", "Brienne of Tarth", "Ramsay Bolton", "Ellaria Sand", "Daario Naharis", "Missandei", "Jaqen Hghar", "Tommen Baratheon", "Roose Bolton", "The High Sparrow", "Grand Maester Pycelle", "Meryn Trant", "Hodor", "Grenn", "Osha", "Rickon Stark", "Ros", "Gregor Clegane", "Janos Slynt", "Lancel Lannister", "Myrcella Baratheon", "Rodrik Cassel", "Maester Luwin", "Irri", "Doreah", "Kevan Lannister", "Barristan Selmy", "Rast", "Maester Aemon", "Pypar", "Alliser Thorne", "Othell Yarwyck", "Loras Tyrell", "Hot Pie", "Beric Dondarrion", "Podrick Payne", "Eddison Tollett", "Yara Greyjoy", "Selyse Florent", "Little Sam", "Grey Worm", "Qyburn", "Olenna Tyrell", "Shireen Baratheon", "Meera Reed", "Jojen Reed", "Thoros of Myr", "Yohn Royce", "Olly", "Mace Tyrell", "The Waif", "Bowen Marsh"};
constexpr size_t sNamesSize = sizeof(sNames) / sizeof(sNames[0]);

AntigenModifyP ChartModify::insert_antigen(size_t before)
{
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(0, sNamesSize - 1);

    // obtain *_modify pointers before inserting antigens
    // otherwise they are incrrectly created with the new number of antigens
    auto& antigens_mod = antigens_modify();
    auto& titers_mod = titers_modify();
    auto& projections_mod = projections_modify();
    auto& plot_spec_mod = plot_spec_modify();

    titers_mod.modifiable_check();
    auto result = antigens_mod.insert(before);
    result->name(sNames[dis(gen)]);
    titers_mod.insert_antigen(before);
    projections_mod.insert_antigen(before);
    plot_spec_mod.insert_antigen(before);
    return result;

} // ChartModify::insert_antigen

// ----------------------------------------------------------------------

SerumModifyP ChartModify::insert_serum(size_t before)
{
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(0, sNamesSize - 1);

    titers_modify().modifiable_check();
    auto result = sera_modify().insert(before);
    result->name(sNames[dis(gen)]);
    titers_modify().insert_serum(before);
    projections_modify().insert_serum(before, number_of_antigens());
    plot_spec_modify().insert_serum(before);
    if (auto fcb = forced_column_bases_modify(MinimumColumnBasis{}); fcb) {
        AD_WARNING("inserting serum in the table having forced column bases, please set column basis for that serum");
        fcb->insert(before, 7.0); // all titers for the new serum are dont-care, there is no real column basis, use 1280 just to have something
    }
    return result;

} // ChartModify::insert_serum

// ----------------------------------------------------------------------

void ChartModify::detect_reference_antigens(remove_reference_before_detecting rrbd)
{
    auto& antigens = antigens_modify();
    auto ser = sera();

    if (rrbd == remove_reference_before_detecting::yes)
        ranges::for_each(range_from_0_to(antigens.size()), [&antigens](auto ag_no) { antigens.at(ag_no).reference(false); });

    // set refernece attribute for all antigens whose names (without reassortant, annotations, passage) are the same as sera names
    // DISTINCT antigens are not marked to avoid marking control duplicates of CDC (CDC-LV can be reference, see h1 20200710)

    acmacs::flat_set_t<ae::virus::Name> serum_names;
    ranges::for_each(range_from_0_to(ser->size()), [&ser, &serum_names](auto sr_no) { serum_names.add(ser->at(sr_no)->name()); });

    ranges::for_each(range_from_0_to(antigens.size()), [&antigens, &serum_names](auto ag_no) {
        if (auto& antigen = antigens.at(ag_no); !antigen.annotations().distinct() && serum_names.exists(antigen.name())) {
            // if (auto& antigen = antigens.at(ag_no); antigen.annotations().empty() && serum_names.exists(antigen.name())) { // TODO: recognized annotations?
            antigen.reference(true);
        }
    });

} // ChartModify::detect_reference_antigens

// ----------------------------------------------------------------------

void ChartModify::combine_projections(ChartModify& merge_in)
{
    if (!same_tables(*this, merge_in, true))
        throw std::runtime_error{AD_FORMAT("cannot combine projections, tables are not the same")};
    auto& merge_in_projections = merge_in.projections_modify();
    for (const auto proj_no : range_from_0_to(merge_in_projections.size()))
        projections_modify().new_by_cloning(*merge_in_projections.at(proj_no), *this);
    projections_modify().sort();

} // ChartModify::combine_projections

// ----------------------------------------------------------------------

ChartNew::ChartNew(size_t number_of_antigens, size_t number_of_sera)
    : ChartModify(number_of_antigens, number_of_sera)
{
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(0, sNamesSize - 1);

    auto& antigens = antigens_modify();
      //size_t name_no = 0;
    std::string suffix = "";
    for (auto ag_no : range_from_0_to(number_of_antigens)) {
        antigens.at(ag_no).name(sNames[dis(gen)] + suffix);
        // ++name_no;
        // if (name_no >= sNamesSize) {
        //     suffix += "A";
        //     name_no = 0;
        // }
    }

    auto& sera = sera_modify();
    suffix = "";
    for (auto sr_no : range_from_0_to(number_of_sera)) {
        sera.at(sr_no).name(sNames[dis(gen)] + suffix);
        // ++name_no;
        // if (name_no >= sNamesSize) {
        //     suffix += "S";
        //     name_no = 0;
        // }
    }

} // ChartNew::ChartNew

// ----------------------------------------------------------------------

ChartClone::ChartClone(const Chart& source, clone_data cd)
    : ChartModify(source, cd == clone_data::projections || cd == clone_data::projections_plot_spec, cd == clone_data::plot_spec || cd == clone_data::projections_plot_spec)
{
} // ChartClone::ChartClone

// ----------------------------------------------------------------------

InfoModify::InfoModify(InfoP main)
    : name_{main->name(Compute::No)},
      computed_name_{main->name(Compute::Yes)},
      virus_{main->virus(Compute::Yes)},
      virus_type_{main->virus_type(Compute::Yes)},
      subset_{main->subset(Compute::Yes)},
      assay_{main->assay(Compute::Yes)},
      lab_{main->lab(Compute::Yes)},
      rbc_species_{main->rbc_species(Compute::Yes)},
      date_{main->date(Compute::Yes)},
      sources_(main->number_of_sources())
{
    for (size_t source_no = 0; source_no < sources_.size(); ++source_no)
        sources_[source_no] = main->source(source_no);

} // InfoModify::InfoModify

// ----------------------------------------------------------------------

void InfoModify::replace_with(const Info& main)
{
    name_ = main.name(Compute::No);
    computed_name_ = main.name(Compute::Yes);
    virus_ = main.virus(Compute::Yes);
    virus_type_ = main.virus_type(Compute::Yes);
    subset_ = main.subset(Compute::Yes);
    assay_ = main.assay(Compute::Yes);
    lab_ = main.lab(Compute::Yes);
    rbc_species_ = main.rbc_species(Compute::Yes);
    date_ = main.date(Compute::Yes);
    if (main.number_of_sources() > 1) {
        sources_.resize(main.number_of_sources());
        for (size_t source_no = 0; source_no < main.number_of_sources(); ++source_no)
            sources_[source_no] = main.source(source_no);
    }
    else
        sources_.clear();

} // InfoModify::replace_with

// ----------------------------------------------------------------------

template <typename Field, typename Func> static inline Field info_modify_make_field(ae::chart::v2::Info::Compute aCompute, const ae::chart::v2::InfoModify& info, const Field& field, Func func)
{
    if (!field.empty() || aCompute == ae::chart::v2::Info::Compute::No)
        return field;

    std::set<std::string> composition;
    std::transform(index_iterator(0UL), index_iterator(info.number_of_sources()), std::inserter(composition, composition.begin()),
                   [&info,&func](size_t index) { return static_cast<std::string>(std::invoke(func, *info.source(index), ae::chart::v2::Info::Compute::No)); });
    return Field{ae::string::join("+", composition)};
}

Virus       InfoModify::virus(Compute aCompute) const { return ::info_modify_make_field(aCompute, *this, virus_, &Info::virus); }
ae::virus::type_subtype_t   InfoModify::virus_type(Compute aCompute) const { return ::info_modify_make_field(aCompute, *this, virus_type_, &Info::virus_type); }
std::string InfoModify::subset(Compute aCompute) const { return ::info_modify_make_field(aCompute, *this, subset_, &Info::subset); }
Assay       InfoModify::assay(Compute aCompute) const { return ::info_modify_make_field(aCompute, *this, assay_, &Info::assay); }
Lab         InfoModify::lab(Compute aCompute, FixLab fix) const { return ::info_modify_make_field(aCompute, *this, lab_, [fix](const auto& inf, auto compute) { return inf.lab(compute, fix); }); }
RbcSpecies  InfoModify::rbc_species(Compute aCompute) const { return ::info_modify_make_field(aCompute, *this, rbc_species_, &Info::rbc_species); }

// ----------------------------------------------------------------------

TableDate InfoModify::date(Compute aCompute) const
{
    if (!date_.empty() || aCompute == Info::Compute::No)
        return TableDate{date_};
    if (number_of_sources() == 0)
        return {};
    std::vector<std::string> composition{number_of_sources()};
    std::transform(index_iterator(0UL), index_iterator(number_of_sources()), composition.begin(), [this](size_t index) { return source(index)->date(); });
    return table_date_from_sources(std::move(composition));

} // InfoModify::date

// ----------------------------------------------------------------------

AntigenModify::AntigenModify(const Antigen& main)
    : name_{main.name()},               //
      date_{main.date()},               //
      passage_{main.passage()},         //
      lineage_{main.lineage()},         //
      reassortant_{main.reassortant()}, //
      annotations_{main.annotations()}, //
      lab_ids_{main.lab_ids()},         //
      clades_{main.clades()},           //
      continent_{main.continent()},     //
      reference_{main.reference()},     //
      sequence_aa_{main.sequence_aa()}, //
      sequence_nuc_{main.sequence_nuc()}
{
} // AntigenModify::AntigenModify

// ----------------------------------------------------------------------

void AntigenModify::replace_with(const Antigen& main)
{
    name_ = main.name();
    date_ = main.date();
    passage_ = main.passage();
    lineage_ = main.lineage();
    reassortant_ = main.reassortant();
    annotations_ = main.annotations();
    lab_ids_ = main.lab_ids();
    clades_ = main.clades();
    continent_ = main.continent();
    reference_ = main.reference();
    sequence_aa_ = main.sequence_aa();
    sequence_nuc_ = main.sequence_nuc();

} // AntigenModify::replace_with

// ----------------------------------------------------------------------

void AntigenModify::update_with(const Antigen& main)
{
    if (date_.empty()) {
        date_ = main.date();
    }
    else if (!main.date().empty() && date_ != main.date()) {
        AD_WARNING("merged antigen dates {} vs. {}", *date_, *main.date());
    }

    if (lineage_ == BLineage::Unknown) {
        lineage_ = main.lineage();
    }
    else if (main.lineage() != BLineage::Unknown && lineage_ != main.lineage()) {
        AD_WARNING("merged antigen lineages {} vs. {}", lineage_, main.lineage());
    }
    lab_ids_.insert_if_not_present(main.lab_ids());
    clades_.insert_if_not_present(main.clades());

    reference_ |= main.reference();

} // AntigenModify::update_with

// ----------------------------------------------------------------------

void AntigenModify::set_continent()
{
    if (continent().empty()) {
        const auto& locdb = ae::locdb::v3::get();
        try {
            continent(locdb.continent(ae::virus::name::location(name())));
        }
        catch (std::exception& err) {
            AD_WARNING("cannot figure out continent for \"{}\": {}", *name(), err.what());
        }
        catch (...) {
            AD_WARNING("cannot figure out continent for \"{}\": unknown exception", *name());
        }
    }

} // AntigenModify::set_continent

// ----------------------------------------------------------------------

SerumModify::SerumModify(const Serum& main)
    : name_{main.name()},                               //
      passage_{main.passage()},                         //
      lineage_{main.lineage()},                         //
      reassortant_{main.reassortant()},                 //
      annotations_{main.annotations()},                 //
      clades_{main.clades()},                           //
      serum_id_{main.serum_id()},                       //
      serum_species_{main.serum_species()},             //
      homologous_antigens_{main.homologous_antigens()}, //
      sequence_aa_{main.sequence_aa()},                 //
      sequence_nuc_{main.sequence_nuc()}
{
} // SerumModify::SerumModify

// ----------------------------------------------------------------------

void SerumModify::replace_with(const Serum& main)
{
    name_ = main.name();
    passage_ = main.passage();
    lineage_ = main.lineage();
    reassortant_ = main.reassortant();
    annotations_ = main.annotations();
    clades_ = main.clades();
    serum_id_ = main.serum_id();
    serum_species_ = main.serum_species();
    // homologous_antigens_ = main.homologous_antigens();
    sequence_aa_ = main.sequence_aa();
    sequence_nuc_ = main.sequence_nuc();

} // SerumModify::replace_with

// ----------------------------------------------------------------------

void SerumModify::update_with(const Serum& main)
{
    if (lineage_ == BLineage::Unknown) {
        lineage_ = main.lineage();
    }
    else if (main.lineage() != BLineage::Unknown && lineage_ != main.lineage()) {
        AD_WARNING("merged serum lineages {} vs. {}", lineage_, main.lineage());
    }

    if (serum_species_.empty()) {
        serum_species_ = main.serum_species();
    }
    else if (!main.serum_species().empty() && serum_species_ != main.serum_species()) {
        AD_WARNING("merged serum serum_speciess {} vs. {}", *serum_species_, *main.serum_species());
    }

} // SerumModify::update_with

// ----------------------------------------------------------------------

TitersModify::TitersModify(size_t number_of_antigens, size_t number_of_sera)
    : number_of_sera_{number_of_sera}, titers_{dense_t(number_of_antigens * number_of_sera)}
{
} // TitersModify::TitersModify

// ----------------------------------------------------------------------

TitersModify::TitersModify(TitersP main)
    : number_of_sera_{main->number_of_sera()},
      titers_{main->is_dense() ? titers_t{dense_t{}} : titers_t{sparse_t{}}}
{
      // Titers

    auto fill_titers = [this, &main](auto &titers) {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>) {
              // Dense ==================================================
            titers.resize(main->number_of_antigens() * this->number_of_sera_);
            for (const auto &entry : main->titers_existing()) { // order of iterations is not specified!
                // std::cerr << entry.antigen << ' ' << entry.serum << ' ' << entry.titer << '\n';
                titers[entry.antigen * this->number_of_sera_ + entry.serum] = entry.titer;
            }
        }
        else { // Sparse ==================================================
            titers.resize(main->number_of_antigens());
            for (const auto &entry : main->titers_existing()) { // order of iterations is not specified!
                titers[entry.antigen].emplace_back(entry.serum, entry.titer);
            }
              // sort entries by serum no
            for (auto& row : titers)
                std::sort(row.begin(), row.end(), [](const auto& e1, const auto& e2) { return e1.first < e2.first; });
        }
    };

    std::visit(fill_titers, titers_);

      // Layers
    if (main->number_of_layers() > 0) {
        layers_.resize(main->number_of_layers());
        try {
            rjson::for_each(main->rjson_layers(), [this](const rjson::value& source_layer, size_t layer_no) {
                auto& target = this->layers_[layer_no];
                target.resize(source_layer.size());
                rjson::for_each(source_layer, [&target](const rjson::value& source_row, size_t ag_no) {
                    using target_t = std::remove_reference_t<decltype(target[ag_no])>;
                    using value_type = typename target_t::value_type;
                    if (source_row.is_object())
                        rjson::transform(source_row, std::back_inserter(target[ag_no]), [](const rjson::object::value_type& kv) -> value_type { return {std::stoul(kv.first), Titer{kv.second.to<std::string_view>()}}; });
                    else if (source_row.is_array())
                        rjson::transform(source_row, std::back_inserter(target[ag_no]), [](const rjson::value& titer, size_t serum_no) -> value_type { return {serum_no, Titer{titer.to<std::string_view>()}}; });
                    else
                        throw invalid_data{AD_FORMAT("invalid layer {} type: ", ag_no, source_row.actual_type())};
                });
                  // sort entries by serum no
                for (auto& row : target)
                    std::sort(row.begin(), row.end(), [](const auto& e1, const auto& e2) { return e1.first < e2.first; });
            });
        }
        catch (data_not_available&) {
            for (size_t layer_no = 0; layer_no < main->number_of_layers(); ++layer_no) {
                auto& target = layers_[layer_no];
                target.resize(main->number_of_antigens());
                for (auto ag_no : range_from_0_to(target.size())) {
                    for (auto sr_no : range_from_0_to(number_of_sera_))
                        if (const auto titer = main->titer_of_layer(layer_no, ag_no, sr_no); !titer.is_dont_care() && !titer.is_invalid())
                            target[ag_no].emplace_back(sr_no, titer);
                }
            }
        }
    }

} // TitersModify::TitersModify

// ----------------------------------------------------------------------

inline ae::chart::v2::Titer TitersModify::find_titer_for_serum(const sparse_row_t& aRow, size_t aSerumNo)
{
    if (aRow.empty())
        return {};
    if (const auto found = std::lower_bound(aRow.begin(), aRow.end(), aSerumNo,
                                            [](const auto& e1, size_t sr_no) { return e1.first < sr_no; }); found != aRow.end() && found->first == aSerumNo)
        return found->second;
    return {};

} // TitersModify::find_titer_for_serum

// ----------------------------------------------------------------------

inline ae::chart::v2::Titer TitersModify::titer_in_sparse_t(const sparse_t& aSparse, size_t aAntigenNo, size_t aSerumNo)
{
    return find_titer_for_serum(aSparse[aAntigenNo], aSerumNo);

} // TitersModify::titer_in_sparse_t

// ----------------------------------------------------------------------

Titer TitersModify::titer(size_t aAntigenNo, size_t aSerumNo) const
{
    auto get = [this,aAntigenNo,aSerumNo](const auto& titers) -> Titer {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>)
            return titers[aAntigenNo * this->number_of_sera_ + aSerumNo];
        else
            return titer_in_sparse_t(titers, aAntigenNo, aSerumNo);
    };
    return std::visit(get, titers_);

} // TitersModify::titer

// ----------------------------------------------------------------------

Titer TitersModify::titer_of_layer(size_t aLayerNo, size_t aAntigenNo, size_t aSerumNo) const
{
    return titer_in_sparse_t(layers_[aLayerNo], aAntigenNo, aSerumNo);

} // TitersModify::titer_of_layer

// ----------------------------------------------------------------------

std::vector<Titer> TitersModify::titers_for_layers(size_t aAntigenNo, size_t aSerumNo, include_dotcare inc) const
{
    if (layers_.empty())
        throw data_not_available("no layers");
    std::vector<Titer> result;
    for (const auto& layer: layers_) {
        if (const auto titer = find_titer_for_serum(layer[aAntigenNo], aSerumNo); !titer.is_dont_care())
            result.push_back(titer);
        else if (inc == include_dotcare::yes)
            result.push_back({});
    }
    return result;

} // TitersModify::titers_for_layers

// ----------------------------------------------------------------------

std::vector<size_t> TitersModify::layers_with_antigen(size_t aAntigenNo) const
{
    if (layers_.empty())
        throw data_not_available("no layers");
    const auto num_sera = number_of_sera();
    std::vector<size_t> result;
    for (auto [no, layer] : acmacs::enumerate(layers_)) {
        for (size_t serum_no = 0; serum_no < num_sera; ++serum_no) {
            if (const auto titer = find_titer_for_serum(layer[aAntigenNo], serum_no); !titer.is_dont_care()) {
                result.push_back(no);
                break;
            }
        }
    }
    return result;

} // TitersModify::layers_with_antigen

// ----------------------------------------------------------------------

std::vector<size_t> TitersModify::layers_with_serum(size_t aSerumNo) const
{
    if (layers_.empty())
        throw data_not_available("no layers");
    const auto num_antigens = number_of_antigens();
    std::vector<size_t> result;
    for (auto [no, layer] : acmacs::enumerate(layers_)) {
        for (size_t antigen_no = 0; antigen_no < num_antigens; ++antigen_no) {
            if (const auto titer = find_titer_for_serum(layer[antigen_no], aSerumNo); !titer.is_dont_care()) {
                result.push_back(no);
                break;
            }
        }
    }
    return result;

} // TitersModify::layers_with_antigen

// ----------------------------------------------------------------------

void TitersModify::remove_layers()
{
    layers_.clear();

} // TitersModify::remove_layers

// ----------------------------------------------------------------------

void TitersModify::create_layers(size_t number_of_layers, size_t number_of_antigens)
{
    if (number_of_layers < 2)
        throw invalid_data{AD_FORMAT("invalid number of layers to create: {}", number_of_layers)};
    if (!layers_.empty())
        throw invalid_data{"cannot create layers: already present"};
    layers_.resize(number_of_layers);
    for (auto& layer : layers_)
        layer.resize(number_of_antigens);
    layer_titer_modified_ = true;

} // TitersModify::create_layers

// ----------------------------------------------------------------------

void TitersModify::set_titer(sparse_t& titers, size_t aAntigenNo, size_t aSerumNo, const Titer& aTiter)
{
    auto& row = titers[aAntigenNo];
    if (row.empty()) {
        row.emplace_back(aSerumNo, aTiter);
    }
    else {
        if (auto found = std::lower_bound(row.begin(), row.end(), aSerumNo, [](const auto& e1, size_t sr_no) { return e1.first < sr_no; }); found != row.end() && found->first == aSerumNo)
            found->second = aTiter;
        else
            row.emplace(found, aSerumNo, aTiter);
    }

} // TitersModify::set_titer

// ----------------------------------------------------------------------

void TitersModify::titer(size_t aAntigenNo, size_t aSerumNo, size_t aLayerNo, const Titer& aTiter)
{
    set_titer(layers_.at(aLayerNo), aAntigenNo, aSerumNo, aTiter);
    layer_titer_modified_ = true;

} // TitersModify::titer

// ----------------------------------------------------------------------

std::unique_ptr<TitersModify::titer_merge_report> TitersModify::set_from_layers(ChartModify& chart)
{
    // merge titers from layers
    // ~/ac/acmacs/acmacs/core/chart.py:1281

    if (number_of_layers() < 2)
        throw std::runtime_error("table has no layers");

    std::shared_ptr<ColumnBases> column_bases;
    MinimumColumnBasis no_column_bases{};
    if (has_morethan_in_layers()) {
          // std::cerr << AD_FORMAT("DEBUG: has_morethan_in_layers");
        set_titers_from_layers(more_than_thresholded::adjust_to_next);
        column_bases = computed_column_bases(no_column_bases);
    }
    auto report = set_titers_from_layers(more_than_thresholded::to_dont_care);
    if (column_bases) {
        chart.forced_column_bases_modify(*column_bases);
        AD_INFO("forced column bases: {}", *chart.forced_column_bases(no_column_bases));
    }
    return report;

} // TitersModify::set_from_layers

// ----------------------------------------------------------------------

// if there are more-than thresholded titers and more_than_thresholded
// is 'dont-care', ignore them, if more_than_thresholded is
// 'adjust-to-next', those titers are converted to the next value,
// e.g. >5120 to 10240.
std::unique_ptr<TitersModify::titer_merge_report> TitersModify::set_titers_from_layers(more_than_thresholded mtt)
{
      // core/antigenic_table.py:266
      // backend/antigenic-table.hh:892

    constexpr double standard_deviation_threshold = 1.0; // lispmds: average-multiples-unless-sd-gt-1-ignore-thresholded-unless-only-entries-then-min-threshold
    const auto number_of_antigens = layers_[0].size();
    auto titers = std::make_unique<std::vector<titer_merge_data>>();
    for (auto ag_no : range_from_0_to(number_of_antigens)) {
        for (auto sr_no : range_from_0_to(number_of_sera_)) {
            auto [titer, report] = titer_from_layers(ag_no, sr_no, mtt, standard_deviation_threshold);
            titers->emplace_back(std::move(titer), ag_no, sr_no, report);
        }
    }

    if (titers->size() < (number_of_antigens * number_of_sera_ / 2))
        titers_ = sparse_t(number_of_antigens);
    else
        titers_ = dense_t(number_of_antigens * number_of_sera_);
    // std::cerr << "DEBUG: titers: " << titers.size() << " ag:" << number_of_antigens << " sr: " << number_of_sera_ << '\n';
    for (const auto& data : *titers) {
        if (!data.titer.is_dont_care())
            std::visit([&data,this](auto& target) { this->set_titer(target, data.antigen, data.serum, data.titer); }, titers_);
    }

    return titers;

} // TitersModify::set_titers_from_layers

// ----------------------------------------------------------------------

// lispmds algorithm 2014-12-06 (from mds/src/mds/mds/hi-table.lisp)
// lispmds does not support > at all, it is added to acmacs
// 1. If there are > and < titers, result is *
// 2. If there are just *, result is *
// 3. If there are just thresholded titers, result is min (<) or max (>) of them
// 4. Convert > and < titers to their next values, i.e. <40 to 20, >10240 to 20480, etc.
// 5. Compute SD, if SD > 1, result is *
// 6. If there are no < nor >, result is mean of log titers.
// 7. if max(<) of thresholded is more than max on non-thresholded (e.g. <40 20), then find minimum of thresholded which is more than max on non-thresholded, it is the result with <
// 8. if min(>) of thresholded is less than min on non-thresholded (e.g. >1280 2560), then find maximum of thresholded which is less than min on non-thresholded, it is the result with >
// 9. otherwise result is next of of max/min non-thresholded with </> (e.g. <20 40 --> <80, <20 80 --> <160) "min-more-than >= min-regular", "max-less-than <= max-regular"

// backend/antigenic-table.hh:1087

std::pair<Titer, TitersModify::titer_merge> TitersModify::merge_titers(const std::vector<Titer>& titers, more_than_thresholded mtt, double standard_deviation_threshold)
{
    constexpr auto max_limit = std::numeric_limits<decltype(std::declval<Titer>().value())>::max();
    size_t min_less_than = max_limit, min_more_than = max_limit, min_regular = max_limit;
    size_t max_less_than = 0, max_more_than = 0, max_regular = 0;
    for (const auto& titer : titers) {
        const auto val = titer.value();
        switch (titer.type()) {
            case Titer::Regular:
                min_regular = std::min(min_regular, val);
                max_regular = std::max(max_regular, val);
                break;
            case Titer::LessThan:
                min_less_than = std::min(min_less_than, val);
                max_less_than = std::max(max_less_than, val);
                break;
            case Titer::MoreThan:
                min_more_than = std::min(min_more_than, val);
                max_more_than = std::max(max_more_than, val);
                break;
            case Titer::Dodgy:
            case Titer::Invalid:
            case Titer::DontCare:
                throw std::invalid_argument{"TitersModify::merge_titers cannot handle dont-care, dodgy, invalid titers"};
        }
    }

    if (titers.empty()) // 2. just dontcare
        return {{}, titer_merge::all_dontcare};
    if (max_less_than != 0 && min_more_than != max_limit) // 1. both thresholded
        return {{}, titer_merge::less_and_more_than};
    if (min_regular == max_limit) { // 3. no regular, just thresholded
        if (min_less_than != max_limit)
            return {Titer('<', min_less_than), titer_merge::less_than_only};
        if (mtt == more_than_thresholded::adjust_to_next)
            return {Titer('>', max_more_than), titer_merge::more_than_only_adjust_to_next};
        else
            return {{}, titer_merge::more_than_only_to_dontcare};
    }

    // compute SD
    std::vector<double> adjusted_log(titers.size());
    std::transform(titers.begin(), titers.end(), adjusted_log.begin(), [](const auto& titer) -> double { return titer.logged_with_thresholded(); }); // 4.
    const auto sd_mean = ae::statistics::standard_deviation(adjusted_log.begin(), adjusted_log.end());
    if (sd_mean.population_sd() > standard_deviation_threshold)
        return {Titer{}, titer_merge::sd_too_big};        // 5. if SD > 1, result is *
    if (max_less_than == 0 && min_more_than == max_limit) // 6. just regular
        return {Titer::from_logged(sd_mean.mean()), titer_merge::regular_only};
    if (max_less_than) { // 7.
        if (max_less_than > max_regular) {
            auto result = max_less_than;
            for (const auto& titer : titers) {
                if (titer.is_less_than())
                    if (const auto tval = titer.value(); tval < result && tval > max_regular)
                        result = tval;
            }
            return {Titer('<', result), titer_merge::max_less_than_bigger_than_max_regular};
        }
        else
            return {Titer('<', max_regular * 2), titer_merge::less_than_and_regular};
    }
    if (min_more_than < min_regular) { // 8.
        auto result = min_more_than;
        for (const auto& titer : titers) {
            if (titer.is_more_than())
                if (const auto tval = titer.value(); tval > result && tval < min_regular)
                    result = tval;
        }
        return {Titer('>', result), titer_merge::min_more_than_less_than_min_regular};
    }
    else
        return {Titer('>', min_regular / 2), titer_merge::more_than_and_regular};

} // TitersModify::merge_titers

// ----------------------------------------------------------------------

std::pair<Titer, TitersModify::titer_merge> TitersModify::titer_from_layers(size_t ag_no, size_t sr_no, more_than_thresholded mtt, double standard_deviation_threshold) const
{
    std::vector<Titer> titers;
    for (auto layer_no : range_from_0_to(layers_.size())) {
        if (auto titer = titer_in_sparse_t(layers_[layer_no], ag_no, sr_no); !titer.is_dont_care()) {
            titers.emplace_back(std::move(titer));
        }
    }

    return merge_titers(titers, mtt, standard_deviation_threshold);

} // TitersModify::titer_from_layers

// ----------------------------------------------------------------------

std::string TitersModify::titer_merge_report_brief(titer_merge data)
{
    switch (data) {
        case titer_merge::all_dontcare:
            return "*";
        case titer_merge::less_and_more_than:
            return "<>";
        case titer_merge::less_than_only:
            return "<";
        case titer_merge::more_than_only_adjust_to_next:
            return ">+1";
        case titer_merge::more_than_only_to_dontcare:
            return ">*";
        case titer_merge::sd_too_big:
            return "sd>";
        case titer_merge::regular_only:
            return "+";
        case titer_merge::max_less_than_bigger_than_max_regular:
            return "<<+";
        case titer_merge::less_than_and_regular:
            return "<+";
        case titer_merge::min_more_than_less_than_min_regular:
            return ">>+";
        case titer_merge::more_than_and_regular:
            return ">+";
    }
    return "???";

} // TitersModify::titer_merge_report_brief

// ----------------------------------------------------------------------

std::string TitersModify::titer_merge_report_long(titer_merge data)
{
    switch (data) {
        case titer_merge::all_dontcare:
            return "all source titers are dont-care";
        case titer_merge::less_and_more_than:
            return "both less-than and more-than present";
        case titer_merge::less_than_only:
            return "less-than only";
        case titer_merge::more_than_only_adjust_to_next:
            return "more-than only, adjust to next";
        case titer_merge::more_than_only_to_dontcare:
            return "more-than only, convert to dont-care";
        case titer_merge::sd_too_big:
            return "standard deviation is too big";
        case titer_merge::regular_only:
            return "regular only";
        case titer_merge::max_less_than_bigger_than_max_regular:
            return "max of less than is bigger than max of regulars";
        case titer_merge::less_than_and_regular:
            return "less than and regular";
        case titer_merge::min_more_than_less_than_min_regular:
            return "min of more-than is less than min of regular";
        case titer_merge::more_than_and_regular:
            return "more than and regular";
    }
    return "(internal error)";

} // TitersModify::titer_merge_report_long

// ----------------------------------------------------------------------

std::string TitersModify::titer_merge_report_description()
{
    std::string result;
    for (auto tm : {titer_merge::all_dontcare, titer_merge::less_and_more_than, titer_merge::less_than_only, titer_merge::more_than_only_adjust_to_next, titer_merge::more_than_only_to_dontcare, titer_merge::sd_too_big, titer_merge::regular_only, titer_merge::max_less_than_bigger_than_max_regular, titer_merge::less_than_and_regular, titer_merge::min_more_than_less_than_min_regular, titer_merge::more_than_and_regular}) {
        auto brief = titer_merge_report_brief(tm);
        brief += std::string(3 - brief.size(), ' ');
        result += brief + "  " + titer_merge_report_long(tm) + '\n';
    }
    return result;

} // TitersModify::titer_merge_report_description

// ----------------------------------------------------------------------

size_t TitersModify::number_of_antigens() const
{
    auto num_ags = [this](const auto& titers) -> size_t {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>)
            return titers.size() / this->number_of_sera_;
        else
            return titers.size();
    };
    return std::visit(num_ags, titers_);

} // TitersModify::number_of_antigens

// ----------------------------------------------------------------------

size_t TitersModify::number_of_non_dont_cares() const
{
    auto num_non_dont_cares = [](const auto& titers) -> size_t {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>)
            return std::accumulate(titers.begin(), titers.end(), size_t{0}, [](size_t a, const auto& titer) -> size_t { return a + (titer.is_dont_care() ? size_t{0} : size_t{1}); });
        else
            return std::accumulate(titers.begin(), titers.end(), size_t{0}, [](size_t a, const auto& row) -> size_t { return a + row.size(); });
    };
    return std::visit(num_non_dont_cares, titers_);

} // TitersModify::number_of_non_dont_cares

// ----------------------------------------------------------------------

size_t TitersModify::titrations_for_antigen(size_t antigen_no) const
{
    auto num_non_dont_cares = [antigen_no, this](const auto& titers) -> size_t {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>)
            return static_cast<size_t>(
                std::count_if(&titers[antigen_no * this->number_of_sera_], &titers[(antigen_no + 1) * this->number_of_sera_], [](const Titer& titer) { return !titer.is_dont_care(); }));
        else
            return titers[antigen_no].size();
    };
    return std::visit(num_non_dont_cares, titers_);

} // TitersModify::titrations_for_antigen

// ----------------------------------------------------------------------

size_t TitersModify::titrations_for_serum(size_t serum_no) const
{
    auto num_non_dont_cares = [serum_no, this](const auto& titers) -> size_t {
        using T = std::decay_t<decltype(titers)>;
        size_t result{0};
        if constexpr (std::is_same_v<T, dense_t>) {
            for (auto antigen_no = 0ul; antigen_no < (titers.size() / this->number_of_sera_); ++antigen_no) {
                if (!titers[antigen_no * this->number_of_sera_ + serum_no].is_dont_care())
                    ++result;
            }
        }
        else {
            for (auto& row : titers) {
                if (auto found = std::lower_bound(row.begin(), row.end(), serum_no, [](const auto& e1, size_t sr_no) { return e1.first < sr_no; }); found != row.end() && found->first == serum_no) {
                    if (!found->second.is_dont_care())
                        ++result;
                }
            }
        }
        return result;
    };
    return std::visit(num_non_dont_cares, titers_);

} // TitersModify::titrations_for_serum

// ----------------------------------------------------------------------

void TitersModify::titer(size_t aAntigenNo, size_t aSerumNo, const Titer& aTiter)
{
    modifiable_check();
    std::visit([aAntigenNo,aSerumNo,&aTiter,this](auto& titers) { this->set_titer(titers, aAntigenNo, aSerumNo, aTiter); }, titers_);

} // TitersModify::titer

// ----------------------------------------------------------------------

void TitersModify::dontcare_for_antigen(size_t aAntigenNo)
{
    modifiable_check();
    auto set_dontcare = [aAntigenNo, this](auto& titers) {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>) {
            std::fill_n(titers.begin() + static_cast<typename T::difference_type>(aAntigenNo * this->number_of_sera_), this->number_of_sera_, Titer{});
        }
        else {
            titers[aAntigenNo].clear();
        }
    };
    return std::visit(set_dontcare, titers_);

} // TitersModify::dontcare_for_antigen

// ----------------------------------------------------------------------

void TitersModify::dontcare_for_serum(size_t aSerumNo)
{
    modifiable_check();
    auto set_dontcare = [aSerumNo, this](auto& titers) {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>) {
            for (auto ag_no : range_from_0_to(number_of_antigens()))
                titers[ag_no * this->number_of_sera_ + aSerumNo] = Titer{};
        }
        else {
            for (auto& row : titers) {
                if (auto found = std::lower_bound(row.begin(), row.end(), aSerumNo, [](const auto& e1, size_t sr_no) { return e1.first < sr_no; }); found != row.end() && found->first == aSerumNo)
                    row.erase(found);
            }
        }
    };
    return std::visit(set_dontcare, titers_);

} // TitersModify::dontcare_for_serum

// ----------------------------------------------------------------------

void TitersModify::multiply_by_for_antigen(size_t aAntigenNo, double multiply_by)
{
    modifiable_check();
    auto multiply = [aAntigenNo,multiply_by,this](auto& titers) {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>) {
            auto first = titers.begin() + static_cast<typename T::difference_type>(aAntigenNo * this->number_of_sera_);
            std::for_each(first, first + static_cast<typename T::difference_type>(this->number_of_sera_), [multiply_by](Titer& titer) { titer = titer.multiplied_by(multiply_by); });
        }
        else {
            std::for_each(titers[aAntigenNo].begin(), titers[aAntigenNo].end(), [multiply_by](sparse_entry_t& entry) { entry.second = entry.second.multiplied_by(multiply_by); });
        }
    };
    return std::visit(multiply, titers_);

} // TitersModify::multiply_by_for_antigen

// ----------------------------------------------------------------------

void TitersModify::multiply_by_for_serum(size_t aSerumNo, double multiply_by)
{
    modifiable_check();
    const auto multiply = [aSerumNo, multiply_by, this](auto& titers) {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>) {
            for (auto ag_no : range_from_0_to(number_of_antigens())) {
                auto& titer = titers[ag_no * this->number_of_sera_ + aSerumNo];
                titer = titer.multiplied_by(multiply_by);
            }
        }
        else {
            for (auto& row : titers) {
                if (auto found = std::lower_bound(row.begin(), row.end(), aSerumNo, [](const auto& e1, size_t sr_no) { return e1.first < sr_no; }); found != row.end() && found->first == aSerumNo)
                    found->second = found->second.multiplied_by(multiply_by);
            }
        }
    };
    return std::visit(multiply, titers_);

} // TitersModify::multiply_by_for_serum

// ----------------------------------------------------------------------

std::vector<TiterIterator::Data> TitersModify::replace_all(const std::regex& look_for, std::string_view replacement)
{
    modifiable_check();
    std::vector<TiterIterator::Data> replacements;
    for (const auto& titer_ref : titers_existing()) {
        if (std::smatch match; std::regex_search(titer_ref.titer.get(), match, look_for)) {
            auto repl{titer_ref};
            repl.titer = Titer{match.format(std::string{replacement})};
            replacements.push_back(repl);
        }
    }
    if (!replacements.empty()) {
        for (const auto& ref : replacements)
            titer(ref.antigen, ref.serum, ref.titer);
    }
    // else
    //     AD_WARNING("TitersModify::replace_all: no matches found, nothing replaced");

    return replacements;

} // TitersModify::replace_all

// ----------------------------------------------------------------------

// acmacs/backend/antigenic-table.hh setProportionToDontCare()
void TitersModify::set_proportion_of_titers_to_dont_care(double proportion)
{
    modifiable_check();

    if (proportion <= 0.0 || proportion > 0.5)
        throw std::invalid_argument(fmt::format("invalid proportion for set_proportion_of_titers_to_dont_care: {}", proportion));

    // collect all non-dont-care titers (row, col), randomly shuffle them, choose first proportion*size entries and set the to don't care

    std::vector<std::pair<size_t, size_t>> cells;
    for (const auto& titer_ref : titers_existing())
        cells.emplace_back(titer_ref.antigen, titer_ref.serum);

    std::mt19937 generator{std::random_device{}()};
    std::shuffle(cells.begin(), cells.end(), generator);
    const auto entries_to_dont_care = static_cast<size_t>(std::lround(static_cast<double>(cells.size()) * proportion));
    const auto set_to_dont_care = [entries_to_dont_care, &cells, number_of_sera = number_of_sera_](auto& titers) {
        for (auto index : range_from_0_to(entries_to_dont_care)) {
            const auto ag_no = cells[index].first, sr_no = cells[index].second;
            using T = std::decay_t<decltype(titers)>;
            if constexpr (std::is_same_v<T, dense_t>) {
                titers[ag_no * number_of_sera + sr_no] = Titer{};
            }
            else {
                if (auto& row = titers[ag_no]; !row.empty()) {
                    if (auto found = std::lower_bound(row.begin(), row.end(), sr_no, [](const auto& e1, size_t sr_no_1) { return e1.first < sr_no_1; }); found != row.end() && found->first == sr_no)
                        row.erase(found);
                }
            }
        }
    };
    std::visit(set_to_dont_care, titers_);

} // TitersModify::set_proportion_of_titers_to_dont_care

// ----------------------------------------------------------------------

void TitersModify::remove_antigens(const ReverseSortedIndexes& indexes)
{
    auto do_remove_antigens_sparse = [&indexes](auto& titers) { remove(indexes, titers); };

    auto do_remove_antigens_dense = [&indexes, this](auto& titers) {
        for (auto index : indexes) {
            const auto first = titers.begin() + static_cast<Indexes::difference_type>(index * this->number_of_sera_);
            titers.erase(first, first + static_cast<Indexes::difference_type>(this->number_of_sera_));
        }
    };

    auto do_remove_antigens = [&do_remove_antigens_sparse, &do_remove_antigens_dense](auto& titers) {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>)
            do_remove_antigens_dense(titers);
        else
            do_remove_antigens_sparse(titers);
    };

    std::visit(do_remove_antigens, titers_);
    for (auto& layer : layers_)
        do_remove_antigens_sparse(layer);

} // TitersModify::remove_antigens

// ----------------------------------------------------------------------

void TitersModify::insert_antigen(size_t before)
{
    modifiable_check();

    auto do_insert_antigen_sparse = [before](auto& titers) { titers.insert(titers.begin() + static_cast<Indexes::difference_type>(before), sparse_row_t{}); };

    auto do_insert_antigen_dense = [before, this](auto& titers) {
        titers.insert(titers.begin() + static_cast<Indexes::difference_type>(before * this->number_of_sera_), this->number_of_sera_, Titer{});
    };

    auto do_insert_antigen = [&do_insert_antigen_sparse, &do_insert_antigen_dense](auto& titers) {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>)
            do_insert_antigen_dense(titers);
        else
            do_insert_antigen_sparse(titers);
    };

    std::visit(do_insert_antigen, titers_);

} // TitersModify::insert_antigen

// ----------------------------------------------------------------------

void TitersModify::remove_sera(const ReverseSortedIndexes& indexes)
{
    auto do_remove_sera_sparse = [&indexes](auto& titers) {
        for (auto& row : titers) {
            for (auto index : indexes) {
                  // remove entry for index, then renumber entries for >index
                if (auto found = std::lower_bound(row.begin(), row.end(), index, [](const auto& e1, size_t sr_no) { return e1.first < sr_no; }); found != row.end()) {
                    if (found->first == index)
                        found = row.erase(found);
                    for (; found != row.end(); ++found)
                        --found->first;
                }
            }
        }
    };

    auto do_remove_sera_dense = [&indexes, this](auto& titers) {
        using diff_t = Indexes::difference_type;
        for (auto ag_no = static_cast<diff_t>(this->number_of_antigens()) - 1; ag_no >= 0; --ag_no) {
            for (auto index : indexes)
                titers.erase(titers.begin() + ag_no * static_cast<diff_t>(this->number_of_sera_) + static_cast<diff_t>(index));
        }
    };

    auto do_remove_sera = [&do_remove_sera_sparse, &do_remove_sera_dense](auto& titers) {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>)
            do_remove_sera_dense(titers);
        else
            do_remove_sera_sparse(titers);
    };

    std::visit(do_remove_sera, titers_);
    for (auto& layer : layers_)
        do_remove_sera_sparse(layer);

    number_of_sera_ -= indexes.size();

} // TitersModify::remove_sera

// ----------------------------------------------------------------------

void TitersModify::insert_serum(size_t before)
{
    modifiable_check();

    auto do_insert_serum_sparse = [before](auto& titers) {
        // renumber serum entries with indexes >= before
        for (auto& row : titers) {
            for (auto it = std::lower_bound(row.begin(), row.end(), before, [](const auto&e1, size_t sr_no) { return e1.first < sr_no; }); it != row.end(); ++it)
                ++it->first;
        }
    };

    auto do_insert_serum_dense = [before, this](auto& titers) {
        using diff_t = Indexes::difference_type;
        for (auto ag_no = static_cast<diff_t>(this->number_of_antigens()) - 1; ag_no >= 0; --ag_no)
            titers.insert(titers.begin() + ag_no * static_cast<diff_t>(this->number_of_sera_) + static_cast<diff_t>(before), Titer{});
    };

    auto do_insert_serum = [&do_insert_serum_sparse, &do_insert_serum_dense](auto& titers) {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>)
            do_insert_serum_dense(titers);
        else
            do_insert_serum_sparse(titers);
    };

    std::visit(do_insert_serum, titers_);
    ++number_of_sera_;

} // TitersModify::insert_serum

// ----------------------------------------------------------------------

std::shared_ptr<ProjectionModifyNew> ProjectionModify::clone(ChartModify& chart) const
{
    auto projection = std::make_shared<ProjectionModifyNew>(*this);
    chart.projections_modify().add(projection);
    return projection;

} // ProjectionModify::clone

// ----------------------------------------------------------------------

void ProjectionModify::clone_from(const Projection& aSource)
{
    layout_ = std::make_shared<Layout>(*aSource.layout());
    transformation_ = aSource.transformation();
    transformed_layout_.reset();
    set_forced_column_bases(aSource.forced_column_bases());
    disconnected_ = aSource.disconnected();
    unmovable_ = aSource.unmovable();
    unmovable_in_the_last_dimension_ = aSource.unmovable_in_the_last_dimension();
    comment_ = aSource.comment();

} // ProjectionModify::clone_from

// ----------------------------------------------------------------------

void ProjectionsModify::add(std::shared_ptr<ProjectionModify> projection)
{
    projections_.push_back(projection);
    projections_.back()->set_projection_no(projections_.size() - 1);

} // ProjectionsModify::add

// ----------------------------------------------------------------------

std::shared_ptr<ProjectionModifyNew> ProjectionsModify::new_from_scratch(number_of_dimensions_t number_of_dimensions, MinimumColumnBasis minimum_column_basis)
{
    auto projection = std::make_shared<ProjectionModifyNew>(chart(), number_of_dimensions, minimum_column_basis);
    add(projection);
    return projection;

} // ProjectionsModify::new_from_scratch

// ----------------------------------------------------------------------

std::shared_ptr<ProjectionModifyNew> ProjectionsModify::new_by_cloning(const ProjectionModify& source, bool add_to_chart)
{
    auto projection = std::make_shared<ProjectionModifyNew>(source);
    if (add_to_chart)
        add(projection);
    return projection;

} // ProjectionsModify::new_by_cloning

// ----------------------------------------------------------------------

std::shared_ptr<ProjectionModifyNew> ProjectionsModify::new_by_cloning(const ProjectionModify& source, Chart& chart)
{
    auto projection = std::make_shared<ProjectionModifyNew>(source, chart);
    add(projection);
    return projection;

} // ProjectionsModify::new_by_cloning

// ----------------------------------------------------------------------

void ProjectionsModify::remove_all()
{
    projections_.erase(projections_.begin(), projections_.end());

} // ProjectionsModify::remove_all

// ----------------------------------------------------------------------

void ProjectionsModify::remove(size_t projection_no)
{
    if (projection_no >= projections_.size())
        throw invalid_data{AD_FORMAT("invalid projection number: {}", projection_no)};
    projections_.erase(projections_.begin() + static_cast<decltype(projections_)::difference_type>(projection_no));
    set_projection_no();

} // ProjectionsModify::remove

// ----------------------------------------------------------------------

void ProjectionsModify::remove_all_except(size_t projection_no)
{
    using diff_t = decltype(projections_)::difference_type;
    if (projection_no >= projections_.size())
        throw invalid_data{AD_FORMAT("invalid projection number: {}", projection_no)};
    projections_.erase(projections_.begin() + static_cast<diff_t>(projection_no + 1), projections_.end());
    projections_.erase(projections_.begin(), projections_.begin() + static_cast<diff_t>(projection_no));
    set_projection_no();

} // ProjectionsModify::remove_all_except

// ----------------------------------------------------------------------

void ProjectionsModify::remove_except(size_t number_of_initial_projections_to_keep, ProjectionP projection_to_keep)
{
    using diff_t = decltype(projections_)::difference_type;
    const auto projection_no = projection_to_keep ? projection_to_keep->projection_no() : 0;
    if (projection_no >= number_of_initial_projections_to_keep) {
        projections_.erase(projections_.begin() + static_cast<diff_t>(projection_no + 1), projections_.end());
        projections_.erase(projections_.begin() + static_cast<diff_t>(std::min(number_of_initial_projections_to_keep, projections_.size())), projections_.begin() + static_cast<diff_t>(projection_no));
    }
    else {
        projections_.erase(projections_.begin() + static_cast<diff_t>(std::min(number_of_initial_projections_to_keep, projections_.size())), projections_.end());
    }
    set_projection_no();

} // ProjectionsModify::remove_except

// ----------------------------------------------------------------------

PointIndexList ProjectionModify::non_nan_points() const // for relax_incremental and enum unmovable_non_nan_points
{
    auto layt = layout();
    PointIndexList non_nan;
    for (size_t point_no{0}; point_no < layt->number_of_points(); ++point_no) {
        if (layt->point_has_coordinates(point_no))
            non_nan.insert(point_no);
    }
    return non_nan;

} // ProjectionModify::non_nan_points

// ----------------------------------------------------------------------

static inline void remove_from(PointIndexList& list, const ReverseSortedIndexes& indexes, size_t base)
{
    // if (!list.empty() && !indexes.empty())
    //     fmt::print(stderr, "DEBUG: remove_from BEGIN {} {} {}\n", list, indexes, base);
    for (const auto to_remove_base : indexes) {
        const auto to_remove = to_remove_base + base;
        for (auto listp = list.rbegin(); listp != list.rend(); ++listp) {
            if (*listp > to_remove) {
                --*listp;
            }
            else if (*listp == to_remove) {
                list.erase(listp.base() - 1);
                break;
            }
            else
                break;
        }
    }
    // if (!indexes.empty())
    //     fmt::print(stderr, "DEBUG: remove_from END {}\n", list);
}

void ProjectionModify::remove_antigens(const ReverseSortedIndexes& indexes)
{
    modify();
    layout_modified()->remove_points(indexes, 0);
    ::remove_from(disconnected_, indexes, 0);
    ::remove_from(unmovable_, indexes, 0);
    ::remove_from(unmovable_in_the_last_dimension_, indexes, 0);

} // ProjectionModify::remove_antigens

// ----------------------------------------------------------------------

void ProjectionModify::remove_sera(const ReverseSortedIndexes& indexes, size_t number_of_antigens)
{
    modify();
    layout_modified()->remove_points(indexes, number_of_antigens);
    if (forced_column_bases_)
        forced_column_bases_->remove(indexes);
    ::remove_from(disconnected_, indexes, number_of_antigens);
    ::remove_from(unmovable_, indexes, number_of_antigens);
    ::remove_from(unmovable_in_the_last_dimension_, indexes, number_of_antigens);

} // ProjectionModify::remove_sera

// ----------------------------------------------------------------------

static inline void index_inserted(PointIndexList& list, size_t before, size_t base)
{
    for (auto listp = std::lower_bound(list.begin(), list.end(), before + base); listp != list.end(); ++listp)
        ++*listp;
}

void ProjectionModify::insert_antigen(size_t before)
{
    modify();
    layout_modified()->insert_point(before, 0);
    ::index_inserted(disconnected_, before, 0);
    ::index_inserted(unmovable_, before, 0);
    ::index_inserted(unmovable_in_the_last_dimension_, before, 0);

} // ProjectionModify::insert_antigen

// ----------------------------------------------------------------------

void ProjectionModify::insert_serum(size_t before, size_t number_of_antigens)
{
    modify();
    layout_modified()->insert_point(before, number_of_antigens);
    if (forced_column_bases_)
        forced_column_bases_->insert(before - number_of_antigens, 7.0);
    ::index_inserted(disconnected_, before, number_of_antigens);
    ::index_inserted(unmovable_, before, number_of_antigens);
    ::index_inserted(unmovable_in_the_last_dimension_, before, number_of_antigens);

} // ProjectionModify::insert_serum

// ----------------------------------------------------------------------

void ProjectionModify::set_forced_column_basis(size_t serum_no, double column_basis)
{
    modify();
    if (!forced_column_bases_)
        forced_column_bases_ = std::make_shared<ColumnBasesModify>(chart().column_bases(minimum_column_basis()));
    forced_column_bases_->set(serum_no, column_basis);

} // ProjectionModify::set_forced_column_basis

// ----------------------------------------------------------------------

std::shared_ptr<Layout> ProjectionModify::randomize_layout(ProjectionModify::randomizer rnd, double diameter_multiplier, LayoutRandomizer::seed_t seed)
{
    AD_DEBUG("randomize_layout");
    std::shared_ptr<LayoutRandomizer> rnd_v;
    switch (rnd) {
      case randomizer::plain_with_table_max_distance:
          rnd_v = randomizer_plain_with_table_max_distance(*this, seed);
          break;
      case randomizer::plain_with_current_layout_area:
          rnd_v = randomizer_plain_with_current_layout_area(*this, diameter_multiplier, seed);
          break;
      case randomizer::plain_from_sample_optimization:
      {
          auto stress = stress_factory(*this, multiply_antigen_titer_until_column_adjust::yes);
          rnd_v = randomizer_plain_from_sample_optimization(*this, stress, diameter_multiplier, seed);
      }
          break;
    }
    return randomize_layout(rnd_v);

} // ProjectionModify::randomize_layout

// ----------------------------------------------------------------------

std::shared_ptr<Layout> ProjectionModify::randomize_layout(std::shared_ptr<LayoutRandomizer> a_randomizer)
{
    modify();
    auto layout = layout_modified();
    const auto number_of_dimensions = layout->number_of_dimensions();
    for (const auto point_no : range_from_0_to(layout->number_of_points())) {
        const auto point{a_randomizer->get(number_of_dimensions)};
        layout->update(point_no, point);
    }
    // AD_DEBUG("randomize_layout {}", *layout);
    return layout;

} // ProjectionModify::randomize_layout

// ----------------------------------------------------------------------

std::shared_ptr<Layout> ProjectionModify::randomize_layout(const PointIndexList& to_randomize, std::shared_ptr<LayoutRandomizer> a_randomizer)
{
    modify();
    auto layout = layout_modified();
    const auto number_of_dimensions = layout->number_of_dimensions();
    for (auto point_no : to_randomize)
        layout->update(point_no, a_randomizer->get(number_of_dimensions));
    return layout;

} // ProjectionModify::randomize_layout

// ----------------------------------------------------------------------

void ProjectionModify::set_layout(const Layout& layout, bool allow_size_change)
{
    auto target_layout = layout_modified();
    if (!allow_size_change && layout.size() != target_layout->size())
        throw invalid_data("ProjectionModify::set_layout(const Layout&): wrong layout size");
    *target_layout = layout;

} // ProjectionModify::set_layout

// ----------------------------------------------------------------------

void ProjectionModify::disconnect_having_too_few_numeric_titers(optimization_options options, const Titers& titers)
{
    if (options.disconnect_too_few_numeric_titers == disconnect_few_numeric_titers::yes) {
        if (const auto having_too_few_numeric_titers = titers.having_too_few_numeric_titers(); !having_too_few_numeric_titers.empty()) {
            modify();
            disconnected_.extend(having_too_few_numeric_titers);
        }
    }

} // ProjectionModify::disconnect_having_too_few_numeric_titers

// ----------------------------------------------------------------------

optimization_status ProjectionModify::relax(optimization_options options)
{
    const auto status = optimize(*this, options);
    stress_ = status.final_stress;
    if (transformation_.number_of_dimensions != layout_->number_of_dimensions())
        transformation_.reset(layout_->number_of_dimensions());
    return status;

} // ProjectionModify::relax

// ----------------------------------------------------------------------

optimization_status ProjectionModify::relax(optimization_options options, IntermediateLayouts& intermediate_layouts)
{
    const auto status = optimize(*this, intermediate_layouts, options);
    stress_ = status.final_stress;
    if (transformation_.number_of_dimensions != layout_->number_of_dimensions())
        transformation_.reset(layout_->number_of_dimensions());
    return status;

} // ProjectionModify::relax

// ----------------------------------------------------------------------

ProcrustesData ProjectionModify::orient_to(const Projection& master)
{
    CommonAntigensSera common(master.chart(), chart(), CommonAntigensSera::match_level_t::automatic);
    const auto procrustes_data = procrustes(master, *this, common.points(), procrustes_scaling_t::no);
    transformation(procrustes_data.transformation);
    return procrustes_data;

} // ProjectionModify::orient_to

// ----------------------------------------------------------------------

void ProjectionModifyNew::connect(const PointIndexList& to_connect)
{
    for (auto point_no: to_connect) {
        auto& disconnected = get_disconnected();
        const auto found = std::find(disconnected.begin(), disconnected.end(), point_no);
        if (found == disconnected.end())
            throw invalid_data{AD_FORMAT("Point was not disconnected: {}: cannot connect it", point_no)};
        disconnected.erase(found);
    }

} // ProjectionModifyNew::connect

// ----------------------------------------------------------------------

PlotSpecModify::PlotSpecModify(size_t number_of_antigens, size_t number_of_sera)
    : main_{nullptr}, number_of_antigens_(number_of_antigens), modified_{true}, styles_(number_of_antigens + number_of_sera, acmacs::PointStyle{})
{
    // AD_DEBUG("PlotSpecModify number_of_antigens:{}", number_of_antigens);
    for (size_t point_no : range_from_0_to(number_of_antigens)) {
        styles_[point_no].fill(GREEN);
    }
    for (size_t point_no : range_from_to(number_of_antigens, number_of_antigens + number_of_sera)) {
        styles_[point_no].shape(acmacs::PointShape::Box);
    }
    drawing_order_.fill_if_empty(number_of_antigens + number_of_sera);

} // PlotSpecModify::PlotSpecModify

// ----------------------------------------------------------------------

void PlotSpecModify::remove_antigens(const ReverseSortedIndexes& indexes)
{
    modify();
    remove(indexes, styles_);
    drawing_order_.remove_points(indexes);
    number_of_antigens_ -= indexes.size();

} // PlotSpecModify::remove_antigens

// ----------------------------------------------------------------------

void PlotSpecModify::remove_sera(const ReverseSortedIndexes& indexes)
{
    modify();
    remove(indexes, styles_, static_cast<ReverseSortedIndexes::difference_type>(number_of_antigens_));
    drawing_order_.remove_points(indexes, number_of_antigens_);

} // PlotSpecModify::remove_sera

// ----------------------------------------------------------------------

void PlotSpecModify::insert_antigen(size_t before)
{
    modify();
    styles_.insert(styles_.begin() + static_cast<decltype(styles_)::difference_type>(before), acmacs::PointStyle{});
    drawing_order_.insert(before);
    ++number_of_antigens_;

} // PlotSpecModify::insert_antigen

// ----------------------------------------------------------------------

void PlotSpecModify::insert_serum(size_t before)
{
    modify();
    styles_.insert(styles_.begin() + static_cast<decltype(styles_)::difference_type>(before + number_of_antigens_), acmacs::PointStyle{});
    drawing_order_.insert(before + number_of_antigens_);

} // PlotSpecModify::insert_serum

// ----------------------------------------------------------------------
