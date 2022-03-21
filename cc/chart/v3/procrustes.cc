#include "chart/v3/procrustes.hh"
#include "chart/v3/chart.hh"
// #include "chart/v3/alglib.hh"

#pragma GCC diagnostic push

#if defined(__clang__)

#pragma GCC diagnostic ignored "-Wreserved-id-macro"
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma GCC diagnostic ignored "-Wsuggest-destructor-override"

#elif defined(__GNUG__)

#pragma GCC diagnostic ignored "-Wvolatile"

#endif

#define AE_COMPILE_MINLBFGS
#define AE_COMPILE_MINCG
#include "linalg.h"
#undef AE_COMPILE_MINLBFGS
#undef AE_COMPILE_MINCG

// #define AE_COMPILE_PCA
// #include "dataanalysis.h"
// #undef AE_COMPILE_PCA

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class MatrixJ;
    using real_2d_array = ::alglib::real_2d_array;
    using aint_t = alglib::ae_int_t;

    template <typename T> inline aint_t cint(T src) { return static_cast<aint_t>(src); };
    inline aint_t cint(number_of_dimensions_t src) { return static_cast<aint_t>(*src); };

    static real_2d_array multiply(const MatrixJ& left, const real_2d_array& right);
    static real_2d_array multiply(const real_2d_array& left, const real_2d_array& right);
    static void multiply(real_2d_array& matrix, double scale);
    static void multiply_add(real_2d_array& matrix, double scale, const real_2d_array& to_add);
    static real_2d_array multiply_left_transposed(const real_2d_array& left, const real_2d_array& right);
    static real_2d_array multiply_both_transposed(const real_2d_array& left, const real_2d_array& right);
    static real_2d_array transpose(const real_2d_array& matrix);
    static void singular_value_decomposition(const real_2d_array& matrix, real_2d_array& u, real_2d_array& vt);

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

ae::chart::v3::procrustes_data_t ae::chart::v3::procrustes(const Projection& primary, const Projection& secondary, const common_antigens_sera_t& common, procrustes_scaling_t scaling)
{
    const auto number_of_dimensions = primary.number_of_dimensions();
    // auto& primary_layout = primary.number_of_dimensions() == number_of_dimensions_t{2} ? primary.transformed_layout() : primary.layout();
    // const auto& secondary_layout = secondary.layout();
    // if (number_of_dimensions != secondary_layout.number_of_dimensions())
    //     throw Error{fmt::format("[procrustes] projections have different number of dimensions: {} and {}", number_of_dimensions, secondary_layout.number_of_dimensions())};

    // auto common_without_disconnected = common;
    // common_without_disconnected.erase(std::remove_if(std::begin(common_without_disconnected), std::end(common_without_disconnected),
    //                                                  [&primary_layout, &secondary_layout](const auto& en) {
    //                                                      return std::isnan(primary_layout.coordinate(en.primary, number_of_dimensions_t{0})) ||
    //                                                             std::isnan(secondary_layout.coordinate(en.secondary, number_of_dimensions_t{0}));
    //                                                  }),
    //                                   std::end(common_without_disconnected));
    // // std::cerr << "common: " << common.size() << " common_without_disconnected: " << common_without_disconnected.size() << '\n';

    // real_2d_array x, y;
    // x.setlength(cint(common_without_disconnected.size()), cint(number_of_dimensions));
    // y.setlength(cint(common_without_disconnected.size()), cint(number_of_dimensions));
    // for (size_t point_no = 0; point_no < common_without_disconnected.size(); ++point_no) {
    //     for (const auto dim : number_of_dimensions) {
    //         x(cint(point_no), cint(dim)) = primary_layout.coordinate(common_without_disconnected[point_no].primary, dim);
    //         y(cint(point_no), cint(dim)) = secondary_layout.coordinate(common_without_disconnected[point_no].secondary, dim);
    //     }
    // }

    procrustes_data_t procrustes_data(number_of_dimensions);

    // auto set_transformation = [&procrustes_data, number_of_dimensions = cint(number_of_dimensions)](const auto& source) {
    //     for (aint_t row = 0; row < number_of_dimensions; ++row)
    //         for (aint_t col = 0; col < number_of_dimensions; ++col)
    //             procrustes_data.transformation(row, col) = source(row, col);
    //     if (!procrustes_data.transformation.valid())
    //         std::cerr << "WARNING: procrustes: invalid transformation\n";
    // };

    // real_2d_array transformation;
    // if (scaling == procrustes_scaling_t::no) {
    //     const MatrixJProcrustes j(common_without_disconnected.size());
    //     auto m4 = transpose(multiply_left_transposed(multiply(j, y), multiply(j, x)));
    //     real_2d_array u, vt;
    //     singular_value_decomposition(m4, u, vt);
    //     if (has_nan(u))
    //         std::cerr << "WARNING: procrustes: invalid u after svd (no scaling)\n";
    //     if (has_nan(vt))
    //         std::cerr << "WARNING: procrustes: invalid vt after svd (no scaling)\n";
    //     transformation = multiply_both_transposed(vt, u);
    //     if (has_nan(transformation))
    //         std::cerr << "WARNING: procrustes: invalid transformation after svd (no scaling)\n";
    // }
    // else {
    //     const MatrixJProcrustesScaling j(common_without_disconnected.size());
    //     const auto m1 = multiply(j, y);
    //     const auto m2 = multiply_left_transposed(x, m1);
    //     real_2d_array u, vt;
    //     singular_value_decomposition(m2, u, vt);
    //     transformation = multiply_both_transposed(vt, u);
    //     if (has_nan(transformation))
    //         std::cerr << "WARNING: procrustes: invalid transformation after svd (with scaling)\n";
    //     // std::cerr << "transformation0: " << transformation.tostring(8) << '\n';

    //     // calculate optimal scale parameter
    //     const auto denominator = multiply_left_transposed(y, m1);
    //     const auto trace_denominator =
    //         std::accumulate(acmacs::index_iterator<aint_t>(0), acmacs::index_iterator(cint(number_of_dimensions)), 0.0, [&denominator](double sum, auto i) { return sum + denominator(i, i); });
    //     const auto m3 = multiply(y, transformation);
    //     const auto m4 = multiply(j, m3);
    //     const auto numerator = multiply_left_transposed(x, m4);
    //     const auto trace_numerator =
    //         std::accumulate(acmacs::index_iterator<aint_t>(0), acmacs::index_iterator(cint(number_of_dimensions)), 0.0, [&numerator](double sum, auto i) { return sum + numerator(i, i); });
    //     const auto scale = trace_numerator / trace_denominator;
    //     multiply(transformation, scale);
    //     procrustes_data.scale = scale;
    // }
    // set_transformation(transformation);

    // // translation
    // auto m5 = multiply(y, transformation);
    // multiply_add(m5, -1, x);
    // for (auto dim : acmacs::range(number_of_dimensions)) {
    //     const auto t_i = std::accumulate(acmacs::index_iterator<aint_t>(0), acmacs::index_iterator(cint(common_without_disconnected.size())), 0.0,
    //                                      [&m5, dim = cint(dim)](auto sum, auto row) { return sum + m5(row, dim); });
    //     procrustes_data.transformation.translation(dim) = t_i / static_cast<double>(common_without_disconnected.size());
    // }

    // // rms
    // procrustes_data.secondary_transformed = procrustes_data.apply(*secondary_layout);
    // procrustes_data.rms = 0.0;
    // size_t num_rows = 0;
    // for (const auto& cp : common_without_disconnected) {
    //     if (const auto pc = primary_layout.at(cp.primary), sc = procrustes_data.secondary_transformed->at(cp.secondary); pc.exists() && sc.exists()) {
    //         ++num_rows;
    //         const auto make_rms_inc = [&pc, &sc](auto sum, auto dim) { return sum + square(pc[dim] - sc[dim]); };
    //         procrustes_data.rms = std::accumulate(acmacs::index_iterator<number_of_dimensions_t>(0UL), acmacs::index_iterator(number_of_dimensions), procrustes_data.rms, make_rms_inc);
    //         // std::cerr << cp.primary << ' ' << cp.secondary << ' ' << procrustes_data.rms << '\n';
    //     }
    // }
    // procrustes_data.rms = std::sqrt(procrustes_data.rms / static_cast<double>(num_rows));

    return procrustes_data;

} // ae::chart::v3::procrustes

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
}

// ----------------------------------------------------------------------

ae::chart::v3::Layout ae::chart::v3::procrustes_data_t::apply(const Layout& source) const
{

} // ae::chart::v3::procrustes_data_t::apply

// ----------------------------------------------------------------------

alglib::real_2d_array ae::chart::v3::multiply(const MatrixJ& left, const real_2d_array& right)
{

} // ae::chart::v3::multiply

// ----------------------------------------------------------------------

alglib::real_2d_array ae::chart::v3::multiply(const real_2d_array& left, const real_2d_array& right)
{

} // ae::chart::v3::multiply

// ----------------------------------------------------------------------

void ae::chart::v3::multiply(real_2d_array& matrix, double scale)
{

} // ae::chart::v3::multiply

// ----------------------------------------------------------------------

void ae::chart::v3::multiply_add(real_2d_array& matrix, double scale, const real_2d_array& to_add)
{

} // ae::chart::v3::multiply_add

// ----------------------------------------------------------------------

alglib::real_2d_array ae::chart::v3::multiply_left_transposed(const real_2d_array& left, const real_2d_array& right)
{

} // ae::chart::v3::multiply_left_transposed

// ----------------------------------------------------------------------

alglib::real_2d_array ae::chart::v3::multiply_both_transposed(const real_2d_array& left, const real_2d_array& right)
{

} // ae::chart::v3::multiply_both_transposed

// ----------------------------------------------------------------------

alglib::real_2d_array ae::chart::v3::transpose(const real_2d_array& matrix)
{

} // ae::chart::v3::transpose

// ----------------------------------------------------------------------

void ae::chart::v3::singular_value_decomposition(const real_2d_array& matrix, real_2d_array& u, real_2d_array& vt)
{

} // ae::chart::v3::singular_value_decomposition

// ----------------------------------------------------------------------
