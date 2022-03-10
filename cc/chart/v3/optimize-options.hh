#pragma once

#include <stdexcept>
#include <vector>
#include <algorithm>

#include "chart/v3/index.hh"
#include "chart/v3/optimization-precision.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    enum class optimization_method {
        alglib_lbfgs_pca,
        alglib_cg_pca,
        // optimlib_bfgs_pca,
        // optimlib_differential_evolution,
    };
    enum class multiply_antigen_titer_until_column_adjust { no, yes };
    enum class dodgy_titer_is_regular_e { no, yes };
    enum class disconnect_few_numeric_titers { no, yes };

    using number_of_optimizations_t = index_tt<struct number_of_optimizations_tag>;

    enum class use_dimension_annealing { no, yes };
    constexpr inline use_dimension_annealing use_dimension_annealing_from_bool(bool use) { return use ? use_dimension_annealing::yes : use_dimension_annealing::no; }
    enum class remove_source_projection { no, yes }; // for relax_incremental
    enum class unmovable_non_nan_points { no, yes }; // for relax_incremental, points that have coordinates (not NaN) are marked as unmovable

    struct optimization_options
    {
        // optimization_options() = default;
        // optimization_options(optimization_precision a_precision, double a_randomization_diameter_multiplier = 2.0)
        //     : precision{a_precision}, randomization_diameter_multiplier{a_randomization_diameter_multiplier}
        // {
        // }
        // optimization_options(optimization_method a_method, optimization_precision a_precision = optimization_precision::fine, double a_randomization_diameter_multiplier = 2.0)
        //     : method{a_method}, precision{a_precision}, randomization_diameter_multiplier{a_randomization_diameter_multiplier}
        // {
        // }

        optimization_method method{optimization_method::alglib_cg_pca};
        optimization_precision precision{optimization_precision::fine};
        disconnect_few_numeric_titers disconnect_too_few_numeric_titers{disconnect_few_numeric_titers::yes};
        multiply_antigen_titer_until_column_adjust mult{multiply_antigen_titer_until_column_adjust::yes};
        double randomization_diameter_multiplier{2.0}; // for layout randomizations
        int num_threads{0};                            // 0 - omp_get_max_threads()
        use_dimension_annealing dimension_annealing{use_dimension_annealing::no};
        remove_source_projection rsp{remove_source_projection::yes};
        unmovable_non_nan_points unnp{unmovable_non_nan_points::no};

    }; // struct optimization_options

    struct dimension_schedule
    {
        dimension_schedule(number_of_dimensions_t target_number_of_dimensions = number_of_dimensions_t{2}) : schedule{5, target_number_of_dimensions} {}
        dimension_schedule(std::initializer_list<number_of_dimensions_t> arg) : schedule(arg) {}
        dimension_schedule(const std::vector<number_of_dimensions_t>& arg) : schedule(arg) {}
        dimension_schedule(const std::vector<size_t>& arg) : schedule(arg.size(), number_of_dimensions_t{0}) { std::transform(std::begin(arg), std::end(arg), std::begin(schedule), [](size_t src) { return number_of_dimensions_t{src}; }); }

        size_t size() const { return schedule.size(); }
        number_of_dimensions_t initial() const { return schedule.front(); }
        number_of_dimensions_t final() const { return schedule.back(); }

          // using const_iterator = std::vector<size_t>::const_iterator;
        auto begin() const { return schedule.begin(); }
        auto end() const { return schedule.end(); }

        std::vector<number_of_dimensions_t> schedule;

    }; // struct dimension_schedule

    optimization_method optimization_method_from_string(std::string_view source); // optimize.cc

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v3::optimization_method> : public fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatContext> auto format(const ae::chart::v3::optimization_method& method, FormatContext& ctx)
    {
        using namespace ae::chart::v3;
        switch (method) {
          case optimization_method::alglib_lbfgs_pca:
              return format_to(ctx.out(), "alglib_lbfgs_pca");
          case optimization_method::alglib_cg_pca:
              return format_to(ctx.out(), "alglib_cg_pca");
          // case optimization_method::optimlib_bfgs_pca:
          //     return format_to(ctx.out(), "optimlib_bfgs_pca");
          // case optimization_method::optimlib_differential_evolution:
          //     return format_to(ctx.out(), "optimlib_differential_evolution");
        }
        return format_to(ctx.out(), "unknown"); // g++9
    }
};

template <> struct fmt::formatter<ae::chart::v3::optimization_precision> : public fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatContext> auto format(const ae::chart::v3::optimization_precision& precision, FormatContext& ctx)
    {
        using namespace ae::chart::v3;
        switch (precision) {
          case optimization_precision::rough:
              return format_to(ctx.out(), "rough");
          case optimization_precision::very_rough:
              return format_to(ctx.out(), "very_rough");
          case optimization_precision::fine:
              return format_to(ctx.out(), "fine");
        }
        return format_to(ctx.out(), "unknown"); // g++9
    }
};

template <> struct fmt::formatter<ae::chart::v3::multiply_antigen_titer_until_column_adjust> : public fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatContext> auto format(const ae::chart::v3::multiply_antigen_titer_until_column_adjust& mul, FormatContext& ctx)
    {
        using namespace ae::chart::v3;
        switch (mul) {
          case multiply_antigen_titer_until_column_adjust::no:
              return format_to(ctx.out(), "no");
          case multiply_antigen_titer_until_column_adjust::yes:
              return format_to(ctx.out(), "yes");
        }
        return format_to(ctx.out(), "unknown"); // g++9
    }
};

template <> struct fmt::formatter<ae::chart::v3::dodgy_titer_is_regular_e> : public fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatContext> auto format(const ae::chart::v3::dodgy_titer_is_regular_e& dod, FormatContext& ctx)
    {
        using namespace ae::chart::v3;
        switch (dod) {
          case dodgy_titer_is_regular_e::no:
              return format_to(ctx.out(), "no");
          case dodgy_titer_is_regular_e::yes:
              return format_to(ctx.out(), "yes");
        }
        return format_to(ctx.out(), "unknown"); // g++9
    }
};

// ----------------------------------------------------------------------
