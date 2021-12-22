#include "ext/range-v3.hh"
#include "chart/v2/vector-math.hh"
#include "chart/v2/sigmoid.hh"
#include "chart/v2/stress.hh"
#include "chart/v2/chart.hh"

// ----------------------------------------------------------------------

// std::transform_reduce is not in g++-7.2 (nor in g++-8, see Parallelism TS in https://gcc.gnu.org/onlinedocs/libstdc++/manual/status.html#status.iso.2017)
#if !defined(__clang__) && __GNUC__ < 9
namespace std
{
      // extracted from clang5 lib: /usr/local/opt/llvm/include/c++/v1/numeric

    template <class _InputIterator, class _Tp, class _BinaryOp, class _UnaryOp>
        inline _Tp transform_reduce(_InputIterator __first, _InputIterator __last, _Tp __init,  _BinaryOp __b, _UnaryOp __u)
    {
        for (; __first != __last; ++__first)
            __init = __b(__init, __u(*__first));
        return __init;
    }

    template <class _InputIterator1, class _InputIterator2, class _Tp, class _BinaryOp1, class _BinaryOp2>
        inline _Tp transform_reduce(_InputIterator1 __first1, _InputIterator1 __last1, _InputIterator2 __first2, _Tp __init,  _BinaryOp1 __b1, _BinaryOp2 __b2)
    {
        for (; __first1 != __last1; ++__first1, (void) ++__first2)
            __init = __b1(__init, __b2(*__first1, *__first2));
        return __init;
    }
}
#endif

// ----------------------------------------------------------------------

ae::chart::v2::Stress ae::chart::v2::stress_factory(const Projection& projection, multiply_antigen_titer_until_column_adjust mult)
{
    Stress stress(projection, mult);
    auto cb = projection.forced_column_bases();
    if (!cb)
        cb = projection.chart().column_bases(projection.minimum_column_basis());
    projection.chart().titers()->update(stress.table_distances(), *cb, stress.parameters());
    return stress;

} // ae::chart::v2::stress_factory

// ----------------------------------------------------------------------

ae::chart::v2::Stress ae::chart::v2::stress_factory(const Projection& projection, size_t antigen_no, double logged_avidity_adjust, multiply_antigen_titer_until_column_adjust mult)
{
    Stress stress(projection, mult);
    stress.parameters().avidity_adjusts.set_logged(antigen_no, logged_avidity_adjust);
    auto cb = projection.forced_column_bases();
    if (!cb)
        cb = projection.chart().column_bases(projection.minimum_column_basis());
    projection.chart().titers()->update(stress.table_distances(), *cb, stress.parameters());
    return stress;

} // ae::chart::v2::stress_factory

// ----------------------------------------------------------------------

ae::chart::v2::Stress ae::chart::v2::stress_factory(const Chart& chart, number_of_dimensions_t number_of_dimensions, MinimumColumnBasis minimum_column_basis, multiply_antigen_titer_until_column_adjust mult, dodgy_titer_is_regular a_dodgy_titer_is_regular)
{
    Stress stress(number_of_dimensions, chart.number_of_points(), mult, a_dodgy_titer_is_regular);
    auto cb = chart.forced_column_bases(minimum_column_basis);
    if (!cb)
        cb = chart.column_bases(minimum_column_basis);
    chart.titers()->update(stress.table_distances(), *cb, stress.parameters());
    return stress;

} // ae::chart::v2::stress_factory

// ----------------------------------------------------------------------

ae::chart::v2::TableDistances ae::chart::v2::table_distances(const Chart& chart, MinimumColumnBasis minimum_column_basis, dodgy_titer_is_regular a_dodgy_titer_is_regular)
{
    Stress stress(number_of_dimensions_t{2}, chart.number_of_points(), multiply_antigen_titer_until_column_adjust::yes, a_dodgy_titer_is_regular);
    auto cb = chart.forced_column_bases(minimum_column_basis);
    if (!cb)
        cb = chart.column_bases(minimum_column_basis);
    return chart.titers()->table_distances(*cb, stress.parameters());

} // ae::chart::v2::table_distances

// ----------------------------------------------------------------------

constexpr inline double non_zero(double value) { return float_zero(value) ? 1e-5 : value; };

// ----------------------------------------------------------------------

static inline double map_distance(const double* first, size_t point_1, size_t point_2, ae::chart::v2::number_of_dimensions_t number_of_dimensions)
{
    using diff_t = typename std::vector<double>::difference_type;
    return ae::chart::v2::vector_math::distance(
        first + static_cast<diff_t>(static_cast<size_t>(number_of_dimensions) * point_1),
        first + static_cast<diff_t>(static_cast<size_t>(number_of_dimensions) * (point_1 + 1)),
        first + static_cast<diff_t>(static_cast<size_t>(number_of_dimensions) * point_2));

} // map_distance

// ----------------------------------------------------------------------

static inline double map_distance(const double* first, const typename ae::chart::v2::TableDistances::Entry& entry, ae::chart::v2::number_of_dimensions_t number_of_dimensions)
{
    return map_distance(first, entry.point_1, entry.point_2, number_of_dimensions);

} // map_distance

// ----------------------------------------------------------------------

ae::chart::v2::Stress::Stress(const Projection& projection, multiply_antigen_titer_until_column_adjust mult)
    : number_of_dimensions_(projection.number_of_dimensions()),
      parameters_(projection.number_of_points(), projection.unmovable(), projection.disconnected(), projection.unmovable_in_the_last_dimension(),
                  mult, projection.avidity_adjusts(), projection.dodgy_titer_is_regular())
{
} // ae::chart::v2::Stress::Stress

// ----------------------------------------------------------------------

ae::chart::v2::Stress::Stress(number_of_dimensions_t number_of_dimensions, size_t number_of_points, multiply_antigen_titer_until_column_adjust mult, dodgy_titer_is_regular a_dodgy_titer_is_regular)
    : number_of_dimensions_(number_of_dimensions),
      parameters_(number_of_points, mult, a_dodgy_titer_is_regular)
{

} // ae::chart::v2::Stress::Stress

// ----------------------------------------------------------------------

ae::chart::v2::Stress::Stress(number_of_dimensions_t number_of_dimensions, size_t number_of_points)
    : number_of_dimensions_(number_of_dimensions),
      parameters_(number_of_points)
{

} // ae::chart::v2::Stress::Stress

// ----------------------------------------------------------------------

inline double contribution_regular(size_t point_1, size_t point_2, double table_distance, const double* first, ae::chart::v2::number_of_dimensions_t num_dim)
{
    const double diff = table_distance - map_distance(first, point_1, point_2, num_dim);
    return diff * diff;
}

inline double contribution_less_than(size_t point_1, size_t point_2, double table_distance, const double* first, ae::chart::v2::number_of_dimensions_t num_dim)
{
    const double diff = table_distance - map_distance(first, point_1, point_2, num_dim) + 1;
    return diff * diff * ae::chart::v2::sigmoid(diff * ae::chart::v2::SigmoidMutiplier());
}

// inline double contribution_regular(const typename ae::chart::v2::TableDistances::Entry& entry, const double* first, ae::chart::v2::number_of_dimensions_t num_dim)
// {
//     const double diff = entry.table_distance - map_distance(first, entry, num_dim);
//     return diff * diff;
// }

// inline double contribution_less_than(const typename ae::chart::v2::TableDistances::Entry& entry, const double* first, ae::chart::v2::number_of_dimensions_t num_dim)
// {
//     const double diff = entry.table_distance - map_distance(first, entry, num_dim) + 1;
//     return diff * diff * ae::chart::v2::sigmoid(diff * SigmoidMutiplier());
// }

// ----------------------------------------------------------------------

double ae::chart::v2::Stress::value(const double* first, const double*) const
{
    return std::transform_reduce(table_distances().regular().begin(), table_distances().regular().end(), double{0}, std::plus<>(),
                                 [first, num_dim = number_of_dimensions_](const auto& entry) { return contribution_regular(entry.point_1, entry.point_2, entry.distance, first, num_dim); }) +
           std::transform_reduce(table_distances().less_than().begin(), table_distances().less_than().end(), double{0}, std::plus<>(),
                                 [first, num_dim = number_of_dimensions_](const auto& entry) { return contribution_less_than(entry.point_1, entry.point_2, entry.distance, first, num_dim); });

} // ae::chart::v2::Stress::value

// ----------------------------------------------------------------------

double ae::chart::v2::Stress::value(const Layout& aLayout) const
{
    return value(aLayout.as_flat_vector_double().data());

} // ae::chart::v2::Stress::value

// ----------------------------------------------------------------------

double ae::chart::v2::Stress::contribution(size_t point_no, const double* first) const
{
    return std::transform_reduce(table_distances().begin_regular_for(point_no), table_distances().end_regular_for(point_no), double{0}, std::plus<>(),
                                 [first, num_dim = number_of_dimensions_](const auto& entry) { return contribution_regular(entry.point_1, entry.point_2, entry.distance, first, num_dim); }) +
           std::transform_reduce(table_distances().begin_less_than_for(point_no), table_distances().end_less_than_for(point_no), double{0}, std::plus<>(),
                                 [first, num_dim = number_of_dimensions_](const auto& entry) { return contribution_less_than(entry.point_1, entry.point_2, entry.distance, first, num_dim); });

} // ae::chart::v2::Stress::contribution

// ----------------------------------------------------------------------

double ae::chart::v2::Stress::contribution(size_t point_no, const Layout& aLayout) const
{
    return contribution(point_no, aLayout.as_flat_vector_double().data());

} // ae::chart::v2::Stress::contribution

// ----------------------------------------------------------------------

double ae::chart::v2::Stress::contribution(size_t point_no, const TableDistancesForPoint& table_distances_for_point, const double* first) const
{
    return std::transform_reduce(
               table_distances_for_point.regular.begin(), table_distances_for_point.regular.end(), double{0}, std::plus<>(),
               [point_no, first, num_dim = number_of_dimensions_](const auto& entry) { return contribution_regular(point_no, entry.another_point, entry.distance, first, num_dim); }) +
           std::transform_reduce(
               table_distances_for_point.less_than.begin(), table_distances_for_point.less_than.end(), double{0}, std::plus<>(),
               [point_no, first, num_dim = number_of_dimensions_](const auto& entry) { return contribution_less_than(point_no, entry.another_point, entry.distance, first, num_dim); });

} // ae::chart::v2::Stress::contribution

// ----------------------------------------------------------------------

double ae::chart::v2::Stress::contribution(size_t point_no, const TableDistancesForPoint& table_distances_for_point, const Layout& aLayout) const
{
    return contribution(point_no, table_distances_for_point, aLayout.as_flat_vector_double().data());

} // ae::chart::v2::Stress::contribution

// ----------------------------------------------------------------------

std::vector<double> ae::chart::v2::Stress::gradient(const double* first, const double* last) const
{
    std::vector<double> result(static_cast<size_t>(last - first), 0);
    gradient(first, last, result.data());
    return result;

} // ae::chart::v2::Stress::gradient

// ----------------------------------------------------------------------

std::vector<double> ae::chart::v2::Stress::gradient(const Layout& aLayout) const
{
    const auto& arg = aLayout.as_flat_vector_double();
    return gradient(arg.data(), arg.data() + arg.size());

} // ae::chart::v2::Stress::gradient

// ----------------------------------------------------------------------

void ae::chart::v2::Stress::gradient(const double* first, const double* last, double* gradient_first) const
{
    if (parameters_.unmovable->empty() && parameters_.unmovable_in_the_last_dimension->empty())
        gradient_plain(first, last, gradient_first);
    else
        gradient_with_unmovable(first, last, gradient_first);

} // ae::chart::v2::Stress::gradient

// ----------------------------------------------------------------------

double ae::chart::v2::Stress::value_gradient(const double* first, const double* last, double* gradient_first) const
{
    gradient(first, last, gradient_first);
    return value(first, last);

} // ae::chart::v2::Stress::value_gradient

// ----------------------------------------------------------------------

void ae::chart::v2::Stress::gradient_plain(const double* first, const double* last, double* gradient_first) const
{
    std::for_each(gradient_first, gradient_first + (last - first), [](double& val) { val = 0; });

    auto update = [first,gradient_first,num_dim=static_cast<size_t>(number_of_dimensions_)](const auto& entry, double inc_base) {
        using diff_t = typename std::vector<double>::difference_type;
        auto p1 = first + static_cast<diff_t>(entry.point_1 * num_dim),
                p2 = first + static_cast<diff_t>(entry.point_2 * num_dim);
        auto r1 = gradient_first + static_cast<diff_t>(entry.point_1 * num_dim),
                r2 = gradient_first + static_cast<diff_t>(entry.point_2 * num_dim);
        for (size_t dim = 0; dim < num_dim; ++dim, ++p1, ++p2, ++r1, ++r2) {
            const double inc = inc_base * (*p1 - *p2);
            *r1 -= inc;
            *r2 += inc;
        }
    };

    auto contribution_regular = [first,num_dim=number_of_dimensions_,update](const auto& entry) {
        const double map_dist = ::map_distance(first, entry, num_dim);
        const double inc_base = (entry.distance - map_dist) * 2 / non_zero(map_dist);
        update(entry, inc_base);
    };
    auto contribution_less_than = [first,num_dim=number_of_dimensions_,update](const auto& entry) {
        const double map_dist = ::map_distance(first, entry, num_dim);
        const double diff = entry.distance - map_dist + 1;
        const double inc_base = (diff * 2 * ae::chart::v2::sigmoid(diff * SigmoidMutiplier())
                                + diff * diff * ae::chart::v2::d_sigmoid(diff * SigmoidMutiplier()) * SigmoidMutiplier()) / non_zero(map_dist);
        update(entry, inc_base);
    };

    std::for_each(table_distances().regular().begin(), table_distances().regular().end(), contribution_regular);
    std::for_each(table_distances().less_than().begin(), table_distances().less_than().end(), contribution_less_than);

} // ae::chart::v2::Stress::gradient_plain

// ----------------------------------------------------------------------

void ae::chart::v2::Stress::gradient_with_unmovable(const double* first, const double* last, double* gradient_first) const
{
    std::vector<bool> unmovable(parameters_.number_of_points, false);
    for (const auto p_no: parameters_.unmovable)
        unmovable[p_no] = true;
    std::vector<bool> unmovable_in_the_last_dimension(parameters_.number_of_points, false);
    for (const auto p_no: parameters_.unmovable_in_the_last_dimension)
        unmovable_in_the_last_dimension[p_no] = true;

    std::for_each(gradient_first, gradient_first + (last - first), [](double& val) { val = 0; });

    auto update = [first,gradient_first,num_dim=static_cast<size_t>(number_of_dimensions_),&unmovable,&unmovable_in_the_last_dimension](const auto& entry, double inc_base) {
        using diff_t = typename std::vector<double>::difference_type;
        auto p1f = [p=static_cast<diff_t>(entry.point_1 * num_dim)] (auto b) { return b + p; };
        auto p2f = [p=static_cast<diff_t>(entry.point_2 * num_dim)] (auto b) { return b + p; };
        auto p1 = p1f(first);
        auto r1 = p1f(gradient_first);
        auto p2 = p2f(first);
        auto r2 = p2f(gradient_first);
        for (size_t dim = 0; dim < num_dim; ++dim, ++p1, ++p2, ++r1, ++r2) {
            const double inc = inc_base * (*p1 - *p2);
            if (!unmovable[entry.point_1] && (!unmovable_in_the_last_dimension[entry.point_1] || (dim + 1) < num_dim))
                *r1 -= inc;
            if (!unmovable[entry.point_2] && (!unmovable_in_the_last_dimension[entry.point_2] || (dim + 1) < num_dim))
                *r2 += inc;
        }
    };

    auto contribution_regular = [first,num_dim=number_of_dimensions_,update](const auto& entry) {
        const double map_dist = ::map_distance(first, entry, num_dim);
        const double inc_base = (entry.distance - map_dist) * 2 / non_zero(map_dist);
        update(entry, inc_base);
    };
    auto contribution_less_than = [first,num_dim=number_of_dimensions_,update](const auto& entry) {
        const double map_dist = ::map_distance(first, entry, num_dim);
        const double diff = entry.distance - map_dist + 1;
        const double inc_base = (diff * 2 * ae::chart::v2::sigmoid(diff * SigmoidMutiplier())
                                + diff * diff * ae::chart::v2::d_sigmoid(diff * SigmoidMutiplier()) * SigmoidMutiplier()) / non_zero(map_dist);
        update(entry, inc_base);
    };

    std::for_each(table_distances().regular().begin(), table_distances().regular().end(), contribution_regular);
    std::for_each(table_distances().less_than().begin(), table_distances().less_than().end(), contribution_less_than);

} // ae::chart::v2::Stress::gradient_with_unmovable

// ----------------------------------------------------------------------

void ae::chart::v2::Stress::set_coordinates_of_disconnected(double* first, [[maybe_unused]] size_t num_args, double value, number_of_dimensions_t number_of_dimensions) const
{
    // do not use number_of_dimensions_! after pca its value is wrong!
    for (auto p_no : parameters_.disconnected) {
        for (auto dim : range_from_0_to(number_of_dimensions))
            *(first + p_no * static_cast<size_t>(number_of_dimensions) + static_cast<size_t>(dim)) = value;
    }

    // if (float_zero(value)) {
    //     for (const auto arg_no : range_from_0_to(num_args)) {
    //         if (std::isnan(first[arg_no]) || std::isinf(first[arg_no]))
    //             AD_WARNING("Stress::set_coordinates_of_disconnected: coordinates of point {} ({}) are {} but point is not disconnected", arg_no / static_cast<size_t>(number_of_dimensions), arg_no, first[arg_no]);
    //     }
    // }

} // ae::chart::v2::Stress::set_coordinates_of_disconnected

// ----------------------------------------------------------------------
