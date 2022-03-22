#include "chart/v3/procrustes.hh"
#include "chart/v3/chart.hh"
#include "chart/v3/common.hh"
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

    // ----------------------------------------------------------------------

    inline bool has_nan(const alglib::real_2d_array& data)
    {
        for (int row = 0; row < data.rows(); ++row)
            for (int col = 0; col < data.cols(); ++col)
                if (std::isnan(data(row, col)))
                    return true;
        return false;
    }

    template <typename T, typename Func> inline double accumulate(T range, Func func)
    {
        double sum{0.0};
        const aint_t range_i = cint(range);
        for (aint_t ind{0}; ind < range_i; ++ind)
            sum += func(ind);
        return sum;
    }

    // ----------------------------------------------------------------------

    class MatrixJ
    {
      public:
        template <typename S> MatrixJ(S size) : size_(cint(size)) {}
        virtual ~MatrixJ() {}

        constexpr aint_t rows() const { return size_; }
        constexpr aint_t cols() const { return size_; }
        virtual double operator()(aint_t row, aint_t col) const = 0;

      private:
        const aint_t size_;
    };

    class MatrixJProcrustes : public MatrixJ
    {
      public:
        template <typename S> MatrixJProcrustes(S size) : MatrixJ(size), diagonal_(1.0 - 1.0 / static_cast<double>(size)), non_diagonal_(-1.0 / static_cast<double>(size)) {}
        double operator()(aint_t row, aint_t column) const override { return row == column ? diagonal_ : non_diagonal_; }

      private:
        const double diagonal_, non_diagonal_;

    }; // class MatrixJProcrustes

    class MatrixJProcrustesScaling : public MatrixJ
    {
      public:
        template <typename S>
        MatrixJProcrustesScaling(S size)
            : MatrixJ(size), diagonal_0_(1.0 - 1.0 / static_cast<double>(size)), non_diagonal_0_(-1.0 / static_cast<double>(size)),
              diagonal_(non_diagonal_0_ * non_diagonal_0_ * static_cast<double>(size - 1) + diagonal_0_ * diagonal_0_),
              non_diagonal_(non_diagonal_0_ * non_diagonal_0_ * static_cast<double>(size - 2) + non_diagonal_0_ * diagonal_0_ * 2)
        {
        }
        double operator()(aint_t row, aint_t column) const override { return row == column ? diagonal_ : non_diagonal_; }

      private:
        const double diagonal_0_, non_diagonal_0_;
        const double diagonal_, non_diagonal_;

    }; // class MatrixJProcrustesScaling

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

ae::chart::v3::procrustes_data_t ae::chart::v3::procrustes(const Projection& primary, const Projection& secondary, const common_antigens_sera_t& common, procrustes_scaling_t scaling)
{
    const auto number_of_dimensions = primary.number_of_dimensions();
    const Layout primary_layout = primary.number_of_dimensions() == number_of_dimensions_t{2} ? primary.transformed_layout() : primary.layout();
    const Layout& secondary_layout = secondary.layout();
    if (number_of_dimensions != secondary_layout.number_of_dimensions())
        throw Error{fmt::format("[procrustes] projections have different number of dimensions: {} and {}", number_of_dimensions, secondary_layout.number_of_dimensions())};

    auto common_without_disconnected = common.points();
    // const auto common_size = common_without_disconnected.size();
    common_without_disconnected.erase(
        std::remove_if(std::begin(common_without_disconnected), std::end(common_without_disconnected),
                       [&primary_layout, &secondary_layout](const auto& en) { return !primary_layout.point_has_coordinates(en.first) || !secondary_layout.point_has_coordinates(en.second); }),
        std::end(common_without_disconnected));
    // AD_DEBUG("common: {} common_without_disconnected: {}", common_size, common_without_disconnected.size());

    real_2d_array x, y;
    x.setlength(cint(common_without_disconnected.size()), cint(number_of_dimensions));
    y.setlength(cint(common_without_disconnected.size()), cint(number_of_dimensions));
    for (size_t point_no = 0; point_no < common_without_disconnected.size(); ++point_no) {
        for (const auto dim : number_of_dimensions) {
            x(cint(point_no), cint(dim)) = primary_layout(common_without_disconnected[point_no].first, dim);
            y(cint(point_no), cint(dim)) = secondary_layout(common_without_disconnected[point_no].second, dim);
        }
    }

    procrustes_data_t procrustes_data(number_of_dimensions);

    auto set_transformation = [&procrustes_data, number_of_dimensions](const auto& source) {
        for (const auto row : number_of_dimensions)
            for (const auto col : number_of_dimensions)
                procrustes_data.transformation(*row, *col) = source(*row, *col);
        if (!procrustes_data.transformation.valid())
            std::cerr << "WARNING: procrustes: invalid transformation\n";
    };

    real_2d_array transformation;
    if (scaling == procrustes_scaling_t::no) {
        const MatrixJProcrustes j(common_without_disconnected.size());
        auto m4 = transpose(multiply_left_transposed(multiply(j, y), multiply(j, x)));
        real_2d_array u, vt;
        singular_value_decomposition(m4, u, vt);
        if (has_nan(u))
            AD_WARNING("[procrustes] invalid u after svd (no scaling)");
        if (has_nan(vt))
            AD_WARNING("[procrustes] invalid vt after svd (no scaling)");
        transformation = multiply_both_transposed(vt, u);
        if (has_nan(transformation))
            AD_WARNING("[procrustes] invalid transformation after svd (no scaling)");
    }
    else {
        const MatrixJProcrustesScaling j(common_without_disconnected.size());
        const auto m1 = multiply(j, y);
        const auto m2 = multiply_left_transposed(x, m1);
        real_2d_array u, vt;
        singular_value_decomposition(m2, u, vt);
        transformation = multiply_both_transposed(vt, u);
        if (has_nan(transformation))
            AD_WARNING("[procrustes] invalid transformation after svd (with scaling)");
        // std::cerr << "transformation0: " << transformation.tostring(8) << '\n';

        // calculate optimal scale parameter
        const auto denominator = multiply_left_transposed(y, m1);
        const auto trace_denominator = accumulate(number_of_dimensions, [&denominator](aint_t i) { return denominator(i, i); });
        const auto m3 = multiply(y, transformation);
        const auto m4 = multiply(j, m3);
        const auto numerator = multiply_left_transposed(x, m4);
        const auto trace_numerator = accumulate(number_of_dimensions, [&numerator](aint_t i) { return numerator(i, i); });
        const auto scale = trace_numerator / trace_denominator;
        multiply(transformation, scale);
        procrustes_data.scale = scale;
    }
    set_transformation(transformation);

    // translation
    auto m5 = multiply(y, transformation);
    multiply_add(m5, -1, x);
    for (const auto dim : number_of_dimensions) {
        const auto t_i = accumulate(common_without_disconnected.size(), [&m5, dim = cint(dim)](aint_t row) { return m5(row, dim); });
        procrustes_data.transformation.translation(dim) = t_i / static_cast<double>(common_without_disconnected.size());
    }

    AD_DEBUG("procrustes");

    // rms
    procrustes_data.secondary_transformed = procrustes_data.apply(secondary_layout);
    procrustes_data.rms = 0.0;
    size_t num_rows = 0;
    for (const auto& cp : common_without_disconnected) {
        if (const auto pc = primary_layout.at(cp.first), sc = procrustes_data.secondary_transformed.at(cp.second); pc.exists() && sc.exists()) {
            ++num_rows;
            procrustes_data.rms += accumulate(number_of_dimensions, [&pc, &sc](aint_t dim) { return square(pc[number_of_dimensions_t{dim}] - sc[number_of_dimensions_t{dim}]); });
            // std::cerr << cp.primary << ' ' << cp.secondary << ' ' << procrustes_data.rms << '\n';
        }
    }
    procrustes_data.rms = std::sqrt(procrustes_data.rms / static_cast<double>(num_rows));

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
    alglib::real_2d_array result;
    result.setlength(left.rows(), right.cols());
    for (aint_t row = 0; row < left.rows(); ++row) {
        for (aint_t column = 0; column < right.cols(); ++column)
            result(row, column) = accumulate(left.cols(), [&left,&right,row,column](aint_t i) { return left(row, i) * right(i, column); });
    }
    return result;

} // ae::chart::v3::multiply

// ----------------------------------------------------------------------

alglib::real_2d_array ae::chart::v3::multiply(const real_2d_array& left, const real_2d_array& right)
{
    alglib::real_2d_array result;
    result.setlength(left.rows(), right.cols());
    // std::cerr << "multiply left " << left.rows() << 'x' << left.cols() << "  right " << right.rows() << 'x' << right.cols() << "  result " << result.rows() << 'x' << result.cols() << '\n';
    alglib::rmatrixgemm(left.rows(), right.cols(), right.rows(),
                        1.0 /*alpha*/, left, 0 /*i-left*/, 0 /*j-left*/, 0 /*left-no-transform*/,
                        right, 0 /*i-right*/, 0 /*j-right*/, 0 /*right-no-transform*/,
                        0.0 /*beta*/, result, 0 /*i-result*/, 0 /*j-result*/);
    return result;

} // ae::chart::v3::multiply

// ----------------------------------------------------------------------

void ae::chart::v3::multiply(real_2d_array& matrix, double scale)
{
    for (aint_t row = 0; row < matrix.rows(); ++row)
        alglib::vmul(&matrix[row][0], 1, matrix.cols(), scale);

} // ae::chart::v3::multiply

// ----------------------------------------------------------------------

void ae::chart::v3::multiply_add(real_2d_array& matrix, double scale, const real_2d_array& to_add)
{
    for (aint_t row = 0; row < matrix.rows(); ++row) {
        alglib::vmul(&matrix[row][0], 1, matrix.cols(), scale);
        alglib::vadd(&matrix[row][0], 1, &to_add[row][0], 1, matrix.cols());
    }

} // ae::chart::v3::multiply_add

// ----------------------------------------------------------------------

alglib::real_2d_array ae::chart::v3::multiply_left_transposed(const real_2d_array& left, const real_2d_array& right)
{
    alglib::real_2d_array result;
    result.setlength(left.cols(), right.cols());
    alglib::rmatrixgemm(left.cols(), right.cols(), right.rows(),
                        1.0 /*alpha*/, left, 0 /*i-left*/, 0 /*j-left*/, 1 /*left-transpose*/,
                        right, 0 /*i-right*/, 0 /*j-right*/, 0 /*right-no-transform*/,
                        0.0 /*beta*/, result, 0 /*i-result*/, 0 /*j-result*/);
    return result;

} // ae::chart::v3::multiply_left_transposed

// ----------------------------------------------------------------------

alglib::real_2d_array ae::chart::v3::multiply_both_transposed(const real_2d_array& left, const real_2d_array& right)
{
    alglib::real_2d_array result;
    result.setlength(left.cols(), right.rows());
    alglib::rmatrixgemm(left.cols(), right.rows(), left.rows(),
                        1.0 /*alpha*/, left, 0 /*i-left*/, 0 /*j-left*/, 1 /*left-transpose*/,
                        right, 0 /*i-right*/, 0 /*j-right*/, 1 /*right-transpose*/,
                        0.0 /*beta*/, result, 0 /*i-result*/, 0 /*j-result*/);
    return result;

} // ae::chart::v3::multiply_both_transposed

// ----------------------------------------------------------------------

alglib::real_2d_array ae::chart::v3::transpose(const real_2d_array& source)
{
    alglib::real_2d_array result;
    result.setlength(source.cols(), source.rows());
    alglib::rmatrixtranspose(source.rows(), source.cols(), source, 0/*i-source*/, 0/*j-source*/, result, 0/*i-result*/, 0/*j-result*/);
      // std::cerr << "transposed: " << source.rows() << 'x' << source.cols() << " --> " << source.cols() << 'x' << source.rows() << '\n';
    return result;

} // ae::chart::v3::transpose

// ----------------------------------------------------------------------

void ae::chart::v3::singular_value_decomposition(const real_2d_array& matrix, real_2d_array& u, real_2d_array& vt)
{
    vt.setlength(matrix.cols(), matrix.cols());
    u.setlength(matrix.rows(), matrix.rows());
    alglib::real_1d_array w;
    w.setlength(matrix.cols());
    alglib::rmatrixsvd(matrix, matrix.rows(), matrix.cols(), 2/*u-needed*/, 2/*vt-needed*/, 2 /*additionalmemory -> max performance*/, w, u, vt);

} // ae::chart::v3::singular_value_decomposition

// ----------------------------------------------------------------------
