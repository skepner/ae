#pragma once

#include <stdexcept>
#include <numeric>

// #include "draw/v1/line.hh"
#include "utils/string.hh"

// ----------------------------------------------------------------------

namespace ae::statistics
{
    class Error : public std::runtime_error { public: using std::runtime_error::runtime_error; };

// ----------------------------------------------------------------------

    inline double sqr(double value) { return value * value; }

    template <typename T> inline T identity(T val) { return val; }

    template <typename ForwardIterator, typename Extractor> inline std::pair<double, size_t> mean_size(ForwardIterator first, ForwardIterator last, Extractor extractor)
    {
        size_t num = 0;
        double sum = 0;
        for (; first != last; ++first, ++num)
            sum += extractor(*first);
        if (num == 0)
            throw Error("mean(): empty range");
        return {sum / static_cast<double>(num), num};
    }

    template <typename ForwardIterator, typename Extractor> inline double mean(ForwardIterator first, ForwardIterator last, Extractor extractor) { return mean_size(first, last, extractor).first; }
    template <typename ForwardIterator> inline double mean(ForwardIterator first, ForwardIterator last) { return mean(first, last, identity<double>); }
    template <typename Container> inline double mean(const Container& cont) { return mean(std::begin(cont), std::end(cont), identity<double>); }

    template <typename ForwardIterator> inline double mean_abs(ForwardIterator first, ForwardIterator last) { return mean(first, last, [](const auto& en) { return std::abs(en); }); }
    template <typename Container> inline double mean_abs(const Container& cont) { return mean_abs(std::begin(cont), std::end(cont)); }

// ----------------------------------------------------------------------

    template <typename ForwardIterator, typename Extractor> inline double varianceN(ForwardIterator first, ForwardIterator last, double mean, Extractor extractor)
    {
        return std::accumulate(first, last, 0.0, [mean,extractor](double sum, const auto& value) { return sum + sqr(extractor(value) - mean); });
    }

    template <typename ForwardIterator, typename Extractor> inline double varianceN(ForwardIterator first, ForwardIterator last, Extractor extractor)
    {
        return varianceN(first, last, mean(first, last, extractor), extractor);
    }

    template <typename XForwardIterator, typename YForwardIterator> inline double covarianceN(XForwardIterator x_first, XForwardIterator x_last, double x_mean, YForwardIterator y_first, double y_mean)
    {
        double sum = 0;
        for (; x_first != x_last; ++x_first, ++y_first)
            sum += (*x_first - x_mean) * (*y_first - y_mean);
        return sum;
    }

// ----------------------------------------------------------------------

    // https://en.wikipedia.org/wiki/Standard_deviation
    class StandardDeviation
    {
     public:
        double mean() const { return mean_; }
        double sample_sd() const { return sample_sd_; } // R sd() function returns sample sd
        double population_sd() const { return population_sd_; }

     private:
        StandardDeviation(double mean, double population_sd, double sample_sd) : mean_{mean}, population_sd_{population_sd}, sample_sd_{sample_sd} {}

        double mean_;
        double population_sd_ = 0;
        double sample_sd_;

        template <typename ForwardIterator, typename Extractor> friend StandardDeviation standard_deviation(ForwardIterator, ForwardIterator, double, Extractor);
    };

    template <typename ForwardIterator, typename Extractor> inline StandardDeviation standard_deviation(ForwardIterator first, ForwardIterator last, double mean, Extractor extractor)
    {
        const auto vari = varianceN(first, last, mean, extractor);
        const auto size = last - first;
        return {mean, std::sqrt(vari / static_cast<double>(size)), std::sqrt(vari / static_cast<double>(size - 1))};
    }

    template <typename ForwardIterator> inline StandardDeviation standard_deviation(ForwardIterator first, ForwardIterator last, double mean) { return standard_deviation(first, last, mean, identity<double>); }

    template <typename ForwardIterator, typename Extractor> inline StandardDeviation standard_deviation(ForwardIterator first, ForwardIterator last, Extractor extactor)
    {
        return standard_deviation(first, last, mean(first, last, extactor), extactor);
    }

    template <typename ForwardIterator> inline StandardDeviation standard_deviation(ForwardIterator first, ForwardIterator last)
    {
        return standard_deviation(first, last, identity<double>);
    }

    template <typename Container> inline StandardDeviation standard_deviation(const Container& cont) { return standard_deviation(std::begin(cont), std::end(cont)); }

// ----------------------------------------------------------------------

    // class SimpleLinearRegression : public ae::draw::v1::LineDefinedByEquation
    // {
    //  public:
    //     SimpleLinearRegression(double a_slope, double a_intercept, double a_r2, double a_rbar2) : LineDefinedByEquation(a_slope, a_intercept), r2_{a_r2}, rbar2_{a_rbar2} {}

    //     double c0() const { return intercept(); }
    //     double c1() const { return slope(); }
    //     double r2() const { return r2_; } // Coefficient of determination http://en.wikipedia.org/wiki/Coefficient_of_determination
    //     double rbar2() const { return rbar2_; }

    //   private:
    //     const double r2_, rbar2_;
    // };

    // // inline std::ostream& operator<<(std::ostream& out, const SimpleLinearRegression& lrg) { return out << "LinearRegression(c0:" << lrg.c0() << ", c1:" << lrg.c1() << ", r2:" << lrg.r2() << ", rbar2:" << lrg.rbar2() << ')'; }

    // // Adopted from GNU Scientific library (gsl_fit_linear)
    // template <typename XForwardIterator, typename YForwardIterator> inline SimpleLinearRegression simple_linear_regression(XForwardIterator x_first, XForwardIterator x_last, YForwardIterator y_first)
    // {
    //     const auto [x_mean, x_size] = mean_size(x_first, x_last, identity<double>);
    //     const auto y_mean = mean(y_first, y_first + static_cast<ssize_t>(x_size));
    //     const auto slope = covarianceN(x_first, x_last, x_mean, y_first, y_mean) / varianceN(x_first, x_last, x_mean, identity<double>);
    //     const auto intercept = y_mean - slope * x_mean;

    //     // SSE: sum of squared residuals
    //     double sse = 0.0;
    //     for (auto [xi, yi] = std::pair(x_first, y_first); xi != x_last; ++xi, ++yi)
    //         sse += sqr(*yi - (intercept + slope * (*xi)));

    //     // SST:  total sum of squares http://en.wikipedia.org/wiki/Total_sum_of_squares
    //     const auto sst = varianceN(y_first, y_first + static_cast<typename XForwardIterator::difference_type>(x_size), y_mean, identity<double>);
    //     if (sst <= 0)
    //         throw std::runtime_error(fmt::format("simple_linear_regression: cannot calculate R2: SST is wrong: {}", sst));
    //     const auto r2 = 1.0 - sse / sst;
    //     const auto rbar2 = 1.0 - (1.0 - r2) * double(x_size - 1) / double(x_size - 2);

    //     return {slope, intercept, r2, rbar2};
    // }

// ----------------------------------------------------------------------

    // http://en.wikipedia.org/wiki/Correlation
    // https://en.wikipedia.org/wiki/Pearson_correlation_coefficient
    template <typename XForwardIterator, typename YForwardIterator> inline double correlation(XForwardIterator x_first, XForwardIterator x_last, YForwardIterator y_first)
    {
        if (x_first == x_last)
            return 0.0;
        const auto size = x_last - x_first;
        const auto x_mean = mean(x_first, x_last), y_mean = mean(y_first, y_first + size);
        return covarianceN(x_first, x_last, x_mean, y_first, y_mean) / static_cast<double>(size - 1) / standard_deviation(x_first, x_last, x_mean).population_sd() / standard_deviation(y_first, y_first + size, y_mean).population_sd();
    }

    template <typename Container> inline double correlation(const Container& first, const Container& second)
    {
        if (first.size() != second.size())
            throw std::invalid_argument("correlation: containers of different sizes");
        return correlation(std::begin(first), std::end(first), std::begin(second));
    }

} // namespace acmacs::statistics

// ----------------------------------------------------------------------
