#include "ext/range-v3.hh"
#include "utils/float.hh"
#include "chart/v2/procrustes.hh"
#include "chart/v2/chart.hh"

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wreserved-id-macro"
#pragma GCC diagnostic ignored "-Wsuggest-destructor-override"
#endif
#define AE_COMPILE_SVD
#include "linalg.h"
#undef AE_COMPILE_SVD

#pragma GCC diagnostic pop

using namespace acmacs::chart;

using aint_t = alglib::ae_int_t;
template <typename T> constexpr inline aint_t cint(T src) { return static_cast<aint_t>(src); };
constexpr inline aint_t cint(ae::chart::v2::number_of_dimensions_t src) { return static_cast<aint_t>(*src); };

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
    template <typename S> MatrixJProcrustesScaling(S size)
        : MatrixJ(size),
          diagonal_0_(1.0 - 1.0 / static_cast<double>(size)), non_diagonal_0_(-1.0 / static_cast<double>(size)),
          diagonal_(non_diagonal_0_ * non_diagonal_0_ * static_cast<double>(size - 1) + diagonal_0_ * diagonal_0_),
          non_diagonal_(non_diagonal_0_ * non_diagonal_0_ * static_cast<double>(size - 2) + non_diagonal_0_ * diagonal_0_ * 2)
        {}
    double operator()(aint_t row, aint_t column) const override { return row == column ? diagonal_ : non_diagonal_; }

 private:
    const double diagonal_0_, non_diagonal_0_;
    const double diagonal_, non_diagonal_;

}; // class MatrixJProcrustesScaling

// ----------------------------------------------------------------------

static alglib::real_2d_array multiply(const MatrixJ& left, const alglib::real_2d_array& right);
static alglib::real_2d_array multiply(const alglib::real_2d_array& left, const alglib::real_2d_array& right);
static void multiply(alglib::real_2d_array& matrix, double scale);
static void multiply_add(alglib::real_2d_array& matrix, double scale, const alglib::real_2d_array& to_add);
static alglib::real_2d_array multiply_left_transposed(const alglib::real_2d_array& left, const alglib::real_2d_array& right);
static alglib::real_2d_array multiply_both_transposed(const alglib::real_2d_array& left, const alglib::real_2d_array& right);
static alglib::real_2d_array transpose(const alglib::real_2d_array& matrix);
static void singular_value_decomposition(const alglib::real_2d_array& matrix, alglib::real_2d_array& u, alglib::real_2d_array& vt);

inline bool has_nan(const alglib::real_2d_array& data)
{
    for (int row = 0; row < data.rows(); ++row)
        for (int col = 0; col < data.cols(); ++col)
            if (std::isnan(data(row, col)))
                return true;
    return false;
}

// inline bool has_nan(const alglib::real_2d_array& data)
// {
//     bool has_nan = false;
//     for (int row = 0; row < data.rows(); ++row) {
//         for (int col = 0; col < data.cols(); ++col) {
//             const auto val = data(row, col);
//             has_nan |= std::isnan(val);
//             std::cout << ' ' << val;
//         }
//         std::cout << '\n';
//     }
//     return has_nan;
// }

// ----------------------------------------------------------------------

// Code for this function was extracted from Procrustes3-for-lisp.c from lispmds

ae::chart::v2::ProcrustesData ae::chart::v2::procrustes(const Projection& primary, const Projection& secondary, const std::vector<CommonAntigensSera::common_t>& common, procrustes_scaling_t scaling)
{
    auto primary_layout = primary.number_of_dimensions() == number_of_dimensions_t{2} ? primary.transformed_layout() : primary.layout();
    auto secondary_layout = secondary.layout();
    const auto number_of_dimensions = primary_layout->number_of_dimensions();
    if (number_of_dimensions != secondary_layout->number_of_dimensions())
        throw invalid_data("procrustes: projections have different number of dimensions");

    auto common_without_disconnected = common;
    common_without_disconnected.erase(std::remove_if(std::begin(common_without_disconnected), std::end(common_without_disconnected),
                                                     [&primary_layout, &secondary_layout](const auto& en) {
                                                         return std::isnan(primary_layout->coordinate(en.primary, number_of_dimensions_t{0})) ||
                                                                std::isnan(secondary_layout->coordinate(en.secondary, number_of_dimensions_t{0}));
                                                     }),
                                      std::end(common_without_disconnected));
    // std::cerr << "common: " << common.size() << " common_without_disconnected: " << common_without_disconnected.size() << '\n';

    alglib::real_2d_array x, y;
    x.setlength(cint(common_without_disconnected.size()), cint(number_of_dimensions));
    y.setlength(cint(common_without_disconnected.size()), cint(number_of_dimensions));
    for (size_t point_no = 0; point_no < common_without_disconnected.size(); ++point_no) {
        for (auto dim : acmacs::range(number_of_dimensions)) {
            x(cint(point_no), cint(dim)) = primary_layout->coordinate(common_without_disconnected[point_no].primary, dim);
            y(cint(point_no), cint(dim)) = secondary_layout->coordinate(common_without_disconnected[point_no].secondary, dim);
        }
    }

    ProcrustesData result(number_of_dimensions);
    auto set_transformation = [&result, number_of_dimensions = cint(number_of_dimensions)](const auto& source) {
        for (aint_t row = 0; row < number_of_dimensions; ++row)
            for (aint_t col = 0; col < number_of_dimensions; ++col)
                result.transformation(row, col) = source(row, col);
        if (!result.transformation.valid())
            std::cerr << "WARNING: procrustes: invalid transformation\n";
    };

    alglib::real_2d_array transformation;
    if (scaling == procrustes_scaling_t::no) {
        const MatrixJProcrustes j(common_without_disconnected.size());
        auto m4 = transpose(multiply_left_transposed(multiply(j, y), multiply(j, x)));
        alglib::real_2d_array u, vt;
        singular_value_decomposition(m4, u, vt);
        if (has_nan(u))
            std::cerr << "WARNING: procrustes: invalid u after svd (no scaling)\n";
        if (has_nan(vt))
            std::cerr << "WARNING: procrustes: invalid vt after svd (no scaling)\n";
        transformation = multiply_both_transposed(vt, u);
        if (has_nan(transformation))
            std::cerr << "WARNING: procrustes: invalid transformation after svd (no scaling)\n";
    }
    else {
        const MatrixJProcrustesScaling j(common_without_disconnected.size());
        const auto m1 = multiply(j, y);
        const auto m2 = multiply_left_transposed(x, m1);
        alglib::real_2d_array u, vt;
        singular_value_decomposition(m2, u, vt);
        transformation = multiply_both_transposed(vt, u);
        if (has_nan(transformation))
            std::cerr << "WARNING: procrustes: invalid transformation after svd (with scaling)\n";
        // std::cerr << "transformation0: " << transformation.tostring(8) << '\n';

        // calculate optimal scale parameter
        const auto denominator = multiply_left_transposed(y, m1);
        const auto trace_denominator =
            std::accumulate(acmacs::index_iterator<aint_t>(0), acmacs::index_iterator(cint(number_of_dimensions)), 0.0, [&denominator](double sum, auto i) { return sum + denominator(i, i); });
        const auto m3 = multiply(y, transformation);
        const auto m4 = multiply(j, m3);
        const auto numerator = multiply_left_transposed(x, m4);
        const auto trace_numerator =
            std::accumulate(acmacs::index_iterator<aint_t>(0), acmacs::index_iterator(cint(number_of_dimensions)), 0.0, [&numerator](double sum, auto i) { return sum + numerator(i, i); });
        const auto scale = trace_numerator / trace_denominator;
        multiply(transformation, scale);
        result.scale = scale;
    }
    set_transformation(transformation);

    // translation
    auto m5 = multiply(y, transformation);
    multiply_add(m5, -1, x);
    for (auto dim : acmacs::range(number_of_dimensions)) {
        const auto t_i = std::accumulate(acmacs::index_iterator<aint_t>(0), acmacs::index_iterator(cint(common_without_disconnected.size())), 0.0,
                                         [&m5, dim = cint(dim)](auto sum, auto row) { return sum + m5(row, dim); });
        result.transformation.translation(dim) = t_i / static_cast<double>(common_without_disconnected.size());
    }

    // rms
    result.secondary_transformed = result.apply(*secondary_layout);
    result.rms = 0.0;
    size_t num_rows = 0;
    for (const auto& cp : common_without_disconnected) {
        if (const auto pc = primary_layout->at(cp.primary), sc = result.secondary_transformed->at(cp.secondary); pc.exists() && sc.exists()) {
            ++num_rows;
            const auto make_rms_inc = [&pc, &sc](auto sum, auto dim) { return sum + square(pc[dim] - sc[dim]); };
            result.rms = std::accumulate(acmacs::index_iterator<number_of_dimensions_t>(0UL), acmacs::index_iterator(number_of_dimensions), result.rms, make_rms_inc);
            // std::cerr << cp.primary << ' ' << cp.secondary << ' ' << result.rms << '\n';
        }
    }
    result.rms = std::sqrt(result.rms / static_cast<double>(num_rows));

    // std::cerr << "common points (without disconnected): " << common_without_disconnected.size() << '\n';
    // std::cerr << "transformation: " << acmacs::to_string(result.transformation) << '\n';
    // std::cerr << "rms: " << acmacs::to_string(result.rms) << '\n';

    return result;

} // ae::chart::v2::procrustes

// ----------------------------------------------------------------------

std::shared_ptr<acmacs::Layout> ae::chart::v2::ProcrustesData::apply(const acmacs::Layout& source) const
{
    assert(source.number_of_dimensions() == transformation.number_of_dimensions);
    auto result = std::make_shared<acmacs::Layout>(source.number_of_points(), source.number_of_dimensions());

    // multiply source by transformation
    for (size_t row_no = 0; row_no < source.number_of_points(); ++row_no) {
        if (const auto row = source[row_no]; row.exists()) {
            for (auto dim : acmacs::range(transformation.number_of_dimensions)) {
                auto sum_squares = [&source, this, row_no, dim](auto sum, auto index) { return sum + source(row_no, index) * this->transformation(index, dim); };
                result->coordinate(row_no, dim) = std::accumulate(acmacs::index_iterator<number_of_dimensions_t>(0UL), acmacs::index_iterator(source.number_of_dimensions()), 0.0, sum_squares) + transformation.translation(dim);
            }
        }
        else {
            result->set_nan(row_no);
        }
    }

    return result;

} // ae::chart::v2::ProcrustesData::apply

// ----------------------------------------------------------------------

alglib::real_2d_array multiply(const MatrixJ& left, const alglib::real_2d_array& right)
{
    // std::cerr << "multiply left " << left.rows() << 'x' << left.cols() << "  right " << right.rows() << 'x' << right.cols() << '\n';
    alglib::real_2d_array result;
    result.setlength(left.rows(), right.cols());
    for (aint_t row = 0; row < left.rows(); ++row) {
        for (aint_t column = 0; column < right.cols(); ++column) {
            result(row, column) = std::accumulate(acmacs::index_iterator<aint_t>(0), acmacs::index_iterator(left.cols()), 0.0,
                                                  [&left,&right,row,column](double sum, auto i) { return sum + left(row, i) * right(i, column); });
        }
    }
    return result;

} // multiply

// ----------------------------------------------------------------------

alglib::real_2d_array multiply(const alglib::real_2d_array& left, const alglib::real_2d_array& right)
{
    alglib::real_2d_array result;
    result.setlength(left.rows(), right.cols());
    // std::cerr << "multiply left " << left.rows() << 'x' << left.cols() << "  right " << right.rows() << 'x' << right.cols() << "  result " << result.rows() << 'x' << result.cols() << '\n';
    alglib::rmatrixgemm(left.rows(), right.cols(), right.rows(),
                        1.0 /*alpha*/, left, 0 /*i-left*/, 0 /*j-left*/, 0 /*left-no-transform*/,
                        right, 0 /*i-right*/, 0 /*j-right*/, 0 /*right-no-transform*/,
                        0.0 /*beta*/, result, 0 /*i-result*/, 0 /*j-result*/);
    return result;

} // multiply

// ----------------------------------------------------------------------

void multiply(alglib::real_2d_array& matrix, double scale)
{
    for (aint_t row = 0; row < matrix.rows(); ++row)
        alglib::vmul(&matrix[row][0], 1, matrix.cols(), scale);

} // multiply

// ----------------------------------------------------------------------

void multiply_add(alglib::real_2d_array& matrix, double scale, const alglib::real_2d_array& to_add)
{
    for (aint_t row = 0; row < matrix.rows(); ++row) {
        alglib::vmul(&matrix[row][0], 1, matrix.cols(), scale);
        alglib::vadd(&matrix[row][0], 1, &to_add[row][0], 1, matrix.cols());
    }

} // multiply_add

// ----------------------------------------------------------------------

alglib::real_2d_array multiply_left_transposed(const alglib::real_2d_array& left, const alglib::real_2d_array& right)
{
    alglib::real_2d_array result;
    result.setlength(left.cols(), right.cols());
    alglib::rmatrixgemm(left.cols(), right.cols(), right.rows(),
                        1.0 /*alpha*/, left, 0 /*i-left*/, 0 /*j-left*/, 1 /*left-transpose*/,
                        right, 0 /*i-right*/, 0 /*j-right*/, 0 /*right-no-transform*/,
                        0.0 /*beta*/, result, 0 /*i-result*/, 0 /*j-result*/);
    return result;

} // multiply_left_transposed

// ----------------------------------------------------------------------

alglib::real_2d_array multiply_both_transposed(const alglib::real_2d_array& left, const alglib::real_2d_array& right)
{
    alglib::real_2d_array result;
    result.setlength(left.cols(), right.rows());
    alglib::rmatrixgemm(left.cols(), right.rows(), left.rows(),
                        1.0 /*alpha*/, left, 0 /*i-left*/, 0 /*j-left*/, 1 /*left-transpose*/,
                        right, 0 /*i-right*/, 0 /*j-right*/, 1 /*right-transpose*/,
                        0.0 /*beta*/, result, 0 /*i-result*/, 0 /*j-result*/);
    return result;

} // multiply_both_transposed

// ----------------------------------------------------------------------

alglib::real_2d_array transpose(const alglib::real_2d_array& source)
{
    alglib::real_2d_array result;
    result.setlength(source.cols(), source.rows());
    alglib::rmatrixtranspose(source.rows(), source.cols(), source, 0/*i-source*/, 0/*j-source*/, result, 0/*i-result*/, 0/*j-result*/);
      // std::cerr << "transposed: " << source.rows() << 'x' << source.cols() << " --> " << source.cols() << 'x' << source.rows() << '\n';
    return result;

} // transpose

// ----------------------------------------------------------------------

void singular_value_decomposition(const alglib::real_2d_array& matrix, alglib::real_2d_array& u, alglib::real_2d_array& vt)
{
    vt.setlength(matrix.cols(), matrix.cols());
    u.setlength(matrix.rows(), matrix.rows());
    alglib::real_1d_array w;
    w.setlength(matrix.cols());
    alglib::rmatrixsvd(matrix, matrix.rows(), matrix.cols(), 2/*u-needed*/, 2/*vt-needed*/, 2 /*additionalmemory -> max performance*/, w, u, vt);

} // singular_value_decomposition

// ----------------------------------------------------------------------

ae::chart::v2::ProcrustesSummary ae::chart::v2::procrustes_summary(const acmacs::Layout& primary, const acmacs::Layout& transformed_secondary, const ProcrustesSummaryParameters& parameters)
{
    ProcrustesSummary results{parameters.number_of_antigens, primary.number_of_points() - parameters.number_of_antigens};
    double sum_distance = 0;
    for (const auto ag_no : range_from_0_to(parameters.number_of_antigens)) {
        const double dist = distance(primary[ag_no], transformed_secondary[ag_no]);
        // AD_DEBUG("    distance AG {:2d}: {:7.4f}", ag_no, dist);
        results.antigen_distances[ag_no] = dist;
        sum_distance += dist;
        results.longest_distance = std::max(results.longest_distance, dist);
    }
    if (parameters.number_of_antigens > 1)
        results.average_distance = (sum_distance - results.antigen_distances[parameters.antigen_being_tested]) / static_cast<double>(parameters.number_of_antigens - 1);
    else
        results.average_distance = 0;
    // AD_DEBUG("average_distance (without AG {}): {:7.4f}", parameters.antigen_being_tested, results.average_distance);

    for (const auto sr_no : range_from_0_to(results.serum_distances.size())) {
        const double dist = distance(primary[sr_no + parameters.number_of_antigens], transformed_secondary[sr_no + parameters.number_of_antigens]);
        results.serum_distances[sr_no] = dist;
        results.longest_distance = std::max(results.longest_distance, dist);
    }

    if (parameters.number_of_antigens > 0) {
        for (const auto ag_no : range_from_0_to(parameters.number_of_antigens))
            results.antigens_by_distance[ag_no] = ag_no;
        std::sort(results.antigens_by_distance.begin(), results.antigens_by_distance.end(), [&results](auto ag1, auto ag2) { return results.antigen_distances[ag2] < results.antigen_distances[ag1]; });

        if (const double x_diff = transformed_secondary(parameters.antigen_being_tested, number_of_dimensions_t{0}) - primary(parameters.antigen_being_tested, number_of_dimensions_t{0});
            !float_zero(x_diff)) {
            results.test_antigen_angle =
                std::atan((transformed_secondary(parameters.antigen_being_tested, number_of_dimensions_t{1}) - primary(parameters.antigen_being_tested, number_of_dimensions_t{1})) / x_diff);
        }

        if (parameters.vaccine_antigen.has_value()) { // compute only if vaccine antigen is valid
            for (const auto dim : acmacs::range(primary.number_of_dimensions()))
                results.distance_vaccine_to_test_antigen += square(transformed_secondary(parameters.antigen_being_tested, dim) - transformed_secondary(*parameters.vaccine_antigen, dim));
            results.distance_vaccine_to_test_antigen = std::sqrt(results.distance_vaccine_to_test_antigen);

            if (const double xv_diff =
                    transformed_secondary(parameters.antigen_being_tested, number_of_dimensions_t{0}) - transformed_secondary(*parameters.vaccine_antigen, number_of_dimensions_t{0});
                !float_zero(xv_diff)) {
                results.angle_vaccine_to_test_antigen = std::atan(
                    (transformed_secondary(parameters.antigen_being_tested, number_of_dimensions_t{1}) - transformed_secondary(*parameters.vaccine_antigen, number_of_dimensions_t{1})) / xv_diff);
            }
        }
    }

    return results;

} // ae::chart::v2::summary

// ----------------------------------------------------------------------
