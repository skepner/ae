#include <optional>

// #include "utils/string.hh"
#include "chart/v2/factory-import.hh"
#include "chart/v2/factory-export.hh"
#include "chart/v2/chart-modify.hh"
// #include "chart/v2/grid-test.hh"
#include "utils/log.hh"
// #include "acmacs-chart-2/command-helper.hh"

// ----------------------------------------------------------------------

// using namespace acmacs::argv;

// struct Options : public argv
// {
//     Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

//     option<size_t> number_of_optimizations{*this, 'n', dflt{1UL}, desc{"number of optimizations"}};
//     option<size_t> number_of_dimensions{*this, 'd', dflt{2UL}, desc{"number of dimensions"}};
//     option<str>    minimum_column_basis{*this, 'm', dflt{"none"}, desc{"minimum column basis"}};
//     option<bool>   rough{*this, "rough"};
//     option<size_t> fine{*this, "fine", dflt{0UL}, desc{"relax roughly, then relax finely N best projections"}};
//     option<bool>   incremental{*this, "incremental", desc{"only randomize points having NaN coordinates"}};
//     option<bool>   unmovable_non_nan_points{*this, "unmovable-non-nan-points", desc{"requires --incremental, keep ag/sr of primary chart frozen (unmovable)"}};
//     option<bool>   grid{*this, "grid", desc{"perform grid test after optimization until no trapped points left"}};
//     option<str>    grid_json{*this, "grid-json", desc{"export grid test results into json"}};
//     option<double> grid_step{*this, "grid-step", dflt{0.1}};
//     // option<bool>   export_pre_grid{*this, "export-pre-grid", desc{"export chart before running grid test (to help debugging crashes)"}};
//     option<bool>   no_dimension_annealing{*this, "no-dimension-annealing"};
//     option<bool>   dimension_annealing{*this, "dimension-annealing"};
//     option<str>    method{*this, "method", dflt{"alglib-cg"}, desc{"method: alglib-lbfgs, alglib-cg, optim-bfgs, optim-differential-evolution"}};
//     option<double> randomization_diameter_multiplier{*this, "md", dflt{2.0}, desc{"randomization diameter multiplier"}};
//     option<bool>   remove_original_projections{*this, "remove-original-projections", desc{"remove projections found in the source chart"}};
//     option<size_t> keep_projections{*this, "keep-projections", dflt{0UL}, desc{"number of projections to keep, 0 - keep all"}};
//     option<bool>   no_disconnect_having_few_titers{*this, "no-disconnect-having-few-titers"};
//     option<str>    disconnect_antigens{*this, "disconnect-antigens", dflt{""}, desc{"comma or space separated list of antigen/point indexes (0-based) to disconnect for the new projections"}};
//     option<str>    disconnect_sera{*this, "disconnect-sera", dflt{""}, desc{"comma or space separated list of serum indexes (0-based) to disconnect for the new projections"}};
//     option<int>    threads{*this, "threads", dflt{0}, desc{"number of threads to use for optimization (omp): 0 - autodetect, 1 - sequential"}};
//     option<str_array> verbose{*this, 'v', "verbose", desc{"comma separated list (or multiple switches) of enablers"}};
//     option<unsigned> seed{*this, "seed", desc{"seed for randomization, -n 1 implied"}};

//     argument<str>  source_chart{*this, arg_name{"source-chart"}, mandatory};
//     argument<str>  output_chart{*this, arg_name{"output-chart"}};
// };

int main(int argc, char* const argv[])
{
    using namespace std::string_view_literals;
    int exit_code = 0;
    try {
        std::string_view source_chart{argv[1]};
        std::string_view output_chart{argv[2]};
        size_t number_of_optimizations{100};
        ae::chart::v2::number_of_dimensions_t number_of_dimensions{2};
        std::string minimum_column_basis{"none"};
        bool grid{true};
        size_t keep_projections{0};
        int threads{0};
        bool remove_original_projections{false};
        bool incremental{false};
        bool unmovable_non_nan_points{false};
        bool rough{false};
        std::string method{"alglib-cg"}; // alglib-lbfgs, alglib-cg, optim-bfgs, optim-differential-evolution
        std::optional<unsigned> seed{std::nullopt};

        // Options opt(argc, argv);
        // acmacs::log::enable(opt.verbose);
        ae::log::enable(ae::log::relax);

        ae::chart::v2::ChartModify chart{ae::chart::v2::import_from_file(source_chart, ae::chart::v2::Verify::None)};
        auto& projections = chart.projections_modify();
        if (remove_original_projections && !incremental)
            projections.remove_all();
        const auto precision = rough ? ae::chart::v2::optimization_precision::rough : ae::chart::v2::optimization_precision::fine;
        const auto method{ae::chart::v2::optimization_method_from_string(method)};
        // auto disconnected{ae::chart::v2::get_disconnected(opt.disconnect_antigens, opt.disconnect_sera, chart.number_of_antigens(), chart.number_of_sera())};
        const size_t incremental_source_projection_no = 0;

        ae::chart::v2::optimization_options options(method, precision, opt.randomization_diameter_multiplier);
        options.disconnect_too_few_numeric_titers = opt.no_disconnect_having_few_titers ? ae::chart::v2::disconnect_few_numeric_titers::no : ae::chart::v2::disconnect_few_numeric_titers::yes;

        // if (opt.no_dimension_annealing)
        //     AD_WARNING("option --no-dimension-annealing is deprectaed, dimension annealing is disabled by default, use --dimension-annealing to enable");
        const auto dimension_annealing = ae::chart::v2::use_dimension_annealing_from_bool(false);

        if (seed.has_value()) {
            // --- seeded optimization ---
            if (number_of_optimizations != 1ul)
                fmt::print(stderr, "WARNING: can only perform one optimization when seed is used\n");
            if (incremental)
                chart.relax_incremental(incremental_source_projection_no, ae::chart::v2::number_of_optimizations_t{1}, options,
                                        remove_original_projections ? ae::chart::v2::remove_source_projection::yes : ae::chart::v2::remove_source_projection::no,
                                        unmovable_non_nan_points ? ae::chart::v2::unmovable_non_nan_points::yes : ae::chart::v2::unmovable_non_nan_points::no);
            else
                chart.relax(minimum_column_basis, number_of_dimensions, dimension_annealing, options, seed); // , disconnected);
        }
        else {
            options.num_threads = threads;
            if (opt.incremental)
                chart.relax_incremental(incremental_source_projection_no, ae::chart::v2::number_of_optimizations_t{*opt.number_of_optimizations}, options,
                                        remove_original_projections ? ae::chart::v2::remove_source_projection::yes : ae::chart::v2::remove_source_projection::no,
                                        unmovable_non_nan_points ? ae::chart::v2::unmovable_non_nan_points::yes : ae::chart::v2::unmovable_non_nan_points::no);
            else
                chart.relax(ae::chart::v2::number_of_optimizations_t{*opt.number_of_optimizations}, *opt.minimum_column_basis, acmacs::number_of_dimensions_t{*opt.number_of_dimensions},
                            dimension_annealing, options, disconnected);

            // if (grid) {
            //     const size_t projection_no_to_test = 0, relax_attempts = 20;
            //     const auto [grid_results, grid_projections] = ae::chart::v2::grid_test(chart, projection_no_to_test, opt.grid_step, opt.threads, relax_attempts, opt.grid_json);
            // }
        }

        projections.sort();
        for (size_t p_no = 0; p_no < opt.fine; ++p_no)
            chart.projection_modify(p_no)->relax(ae::chart::v2::optimization_options(method, ae::chart::v2::optimization_precision::fine));
        if (keep_projections > 0 && projections.size() > keep_projections)
            projections.keep_just(keep_projections);
        fmt::print("{}\n", chart.make_info());
        // if (output_chart)
        ae::chart::v2::export_factory(chart, output_chart, argv[0]);
    }
    catch (std::exception& err) {
        AD_ERROR("{}", err);
        exit_code = 2;
    }
    return exit_code;
}

// ----------------------------------------------------------------------
