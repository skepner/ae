#pragma once

#include <stdexcept>
#include <chrono>

#include "chart/v3/layout.hh"
#include "chart/v3/optimize-options.hh"
#include "chart/v3/column-bases.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Stress;

    class optimization_error : public std::runtime_error
    {
      public:
        optimization_error(const std::string& msg, const char* file = __builtin_FILE(), int line = __builtin_LINE(), const char* function = __builtin_FUNCTION())
            : std::runtime_error(fmt::format("invalid_data: {} @@ {}:{}: {}", msg, file, line, function))
        {
        }
        optimization_error(const std::string& msg1, const std::string& msg2, const char* file = __builtin_FILE(), int line = __builtin_LINE(), const char* function = __builtin_FUNCTION())
            : std::runtime_error(fmt::format("invalid_data: {} {} @@ {}:{}: {}", msg1, msg2, file, line, function))
        {
        }
    };

    class Projection;
    class Chart;

    // ----------------------------------------------------------------------

    struct optimization_status
    {
        optimization_status(optimization_method a_method) : method{a_method} {}
        optimization_method method;
        size_t number_of_iterations{0};
        size_t number_of_stress_calculations{0};
        std::string termination_report{};
        std::chrono::microseconds time{0};
        double initial_stress{0.0};
        double final_stress{0.0};

    }; // struct optimization_status

    struct DimensionAnnelingStatus
    {
        std::chrono::microseconds time;
    };

    struct IntermediateLayout
    {
        IntermediateLayout(number_of_dimensions_t number_of_dimensions, const double* first, long layout_size, double a_stress)
            : layout(number_of_dimensions, first, first + layout_size), stress{a_stress}
        {
        }
        const Layout layout;
        const double stress;
    };

    using IntermediateLayouts = std::vector<IntermediateLayout>;

    // ----------------------------------------------------------------------

    // optimizes existing projection without dimension annealing
    optimization_status optimize(Chart& chart, Projection& projection, optimization_options options = optimization_options{});
    // optimizes existing projection without dimension annealing and saves intermediate layouts
    optimization_status optimize(Chart& chart, Projection& projection, IntermediateLayouts& intermediate_layouts, optimization_options options = optimization_options{});
    // optimizes existing projection with dimension annealing
    optimization_status optimize(Chart& chart, Projection& projection, const dimension_schedule& schedule, optimization_options options = optimization_options{});
    // creates new projection and optimizes it with or without dimension annealing
    optimization_status optimize(Chart& chart, minimum_column_basis mcb, const dimension_schedule& schedule, optimization_options options = optimization_options{});

    optimization_status optimize(optimization_method method, const Stress& stress, double* arg_first, double* arg_last, optimization_precision precision = optimization_precision::fine);
    inline optimization_status optimize(optimization_method method, const Stress& stress, double* arg_first, size_t arg_size, optimization_precision precision = optimization_precision::fine)
    {
        return optimize(method, stress, arg_first, arg_first + arg_size, precision);
    }

    DimensionAnnelingStatus do_dimension_annealing(optimization_method optimization_method, const Stress& stress, number_of_dimensions_t source_number_of_dimensions,
                                                number_of_dimensions_t target_number_of_dimensions, double* arg_first, double* arg_last);

    // replaces layout in (arg_first, arg_last)
    void pca(const Stress& stress, number_of_dimensions_t number_of_dimensions, double* arg_first, double* arg_last);

    // ----------------------------------------------------------------------

    struct ErrorLine
    {
        ErrorLine(size_t p1, size_t p2, double el) : point_1{p1}, point_2{p2}, error_line{el} {}
        size_t point_1, point_2;
        double error_line; // positive: table_dist > map_dist, i.e. draw line in direction opposite to corresponding point

    }; // struct ErrorLine

    using ErrorLines = std::vector<ErrorLine>;

    ErrorLines error_lines(const Projection& projection);

    // ----------------------------------------------------------------------

    struct OptimiserCallbackData
    {
        OptimiserCallbackData(const Stress& a_stress) : stress{a_stress}, intermediate_layouts{nullptr} {}
        OptimiserCallbackData(const Stress& a_stress, IntermediateLayouts& a_intermediate_layouts) : stress{a_stress}, intermediate_layouts{&a_intermediate_layouts} {}
        const Stress& stress;
        IntermediateLayouts* intermediate_layouts{nullptr};
        size_t iteration_no{0};
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v3::optimization_status> : fmt::formatter<ae::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const ae::chart::v3::optimization_status& status, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "{} {:.12f} <- {:.12f}\n time: {}\n iter: {}\n nstress: {}", status.method, status.final_stress, status.initial_stress, status.time, status.number_of_iterations, status.number_of_stress_calculations);
    }
};


// ----------------------------------------------------------------------
