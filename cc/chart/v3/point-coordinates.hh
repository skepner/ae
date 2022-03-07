#pragma once

#include <limits>
#include <variant>
#include <vector>
#include <span>
#include <numeric>

#include "utils/float.hh"
#include "utils/log.hh"
#include "chart/v3/index.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class point_coordinates
    {
      public:
        constexpr static const double nan = std::numeric_limits<double>::quiet_NaN();
        using store_t = std::vector<double>;
        using ref_t = std::span<double>;

        point_coordinates() = default;
        point_coordinates(const point_coordinates&) = default;
        point_coordinates(point_coordinates&&) = default;
        explicit point_coordinates(number_of_dimensions_t number_of_dimensions, double init = nan) : data_{store_t(*number_of_dimensions, init)} {}
        point_coordinates(double x, double y) : data_{store_t{x, y}} {}
        point_coordinates(double x, double y, double z) : data_{store_t{x, y, z}} {}
        point_coordinates(const ref_t& ref) : data_{ref} {}
        point_coordinates(const store_t& data) : data_{data} {}

        // stored <- stored, ref <- ref
        point_coordinates& operator=(const point_coordinates&) = default;
        point_coordinates& operator=(point_coordinates&&) = default;

        // stored <- stored, stored <- ref
        point_coordinates copy() const
            {
                return point_coordinates{std::visit([](const auto& content) { return store_t(content.begin(), content.end()); }, data_)};
            }

        number_of_dimensions_t number_of_dimensions() const { return std::visit([](const auto& content) { return number_of_dimensions_t{content.size()}; }, data_); }

        double operator[](number_of_dimensions_t dim) const { return std::visit([dim](const auto& content) { return content[*dim]; }, data_); }
        double& operator[](number_of_dimensions_t dim) { return std::visit([dim](auto& content) -> double& { return content[*dim]; }, data_); }

        const double* begin() const { return std::visit([](const auto& content) -> const double* { return &*content.begin(); }, data_); }
        const double* end() const { return std::visit([](const auto& content) -> const double* { return &*content.end(); }, data_); }
        double* begin() { return std::visit([](auto& content) -> double* { return &*content.begin(); }, data_); }
        double* end() { return std::visit([](auto& content) -> double* { return &*content.end(); }, data_); }

        double x() const { return operator[](number_of_dimensions_t{0}); }
        double y() const { return operator[](number_of_dimensions_t{1}); }
        double z() const { return operator[](number_of_dimensions_t{2}); }

        void x(double val) { operator[](number_of_dimensions_t{0}) = val; }
        void y(double val) { operator[](number_of_dimensions_t{1}) = val; }
        void z(double val) { operator[](number_of_dimensions_t{2}) = val; }

        point_coordinates& operator+=(const point_coordinates& rhs) { std::transform(begin(), end(), rhs.begin(), begin(), [](double v1, double v2) { return v1 + v2; }); return *this; }
        point_coordinates& operator+=(double val) { std::transform(begin(), end(), begin(), [val](double v1) { return v1 + val; }); return *this; }
        point_coordinates& operator-=(const point_coordinates& rhs) { std::transform(begin(), end(), rhs.begin(), begin(), [](double v1, double v2) { return v1 - v2; }); return *this; }
        point_coordinates& operator*=(double val) { std::transform(begin(), end(), begin(), [val](double v1) { return v1 * val; }); return *this; }
        point_coordinates& operator/=(double val) { std::transform(begin(), end(), begin(), [val](double v1) { return v1 / val; }); return *this; }

        bool empty() const { return std::any_of(begin(), end(), [](auto val) { return std::isnan(val); }); }
        bool exists() const { return !empty(); }

        std::vector<double> as_vector() const { return {begin(), end()}; }

      private:
        std::variant<store_t, ref_t> data_;
    };

    inline double distance2(const point_coordinates& p1, const point_coordinates& p2)
    {
        std::vector<double> diff(*p1.number_of_dimensions());
        std::transform(p1.begin(), p1.end(), p2.begin(), diff.begin(), [](double v1, double v2) { return v1 - v2; });
        return std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
    }

    inline double distance(const point_coordinates& p1, const point_coordinates& p2) { return std::sqrt(distance2(p1, p2)); }
    inline point_coordinates middle(const point_coordinates& p1, const point_coordinates& p2)
    {
           point_coordinates result(p1.number_of_dimensions());
           std::transform(p1.begin(), p1.end(), p2.begin(), result.begin(), [](double v1, double v2) { return (v1 + v2) / 2.0; });
           return result;
    }

    inline point_coordinates operator-(const point_coordinates& p1) { point_coordinates result(p1); std::transform(result.begin(), result.end(), result.begin(), [](double val) { return -val; }); return result; }
    inline point_coordinates operator+(const point_coordinates& p1, const point_coordinates& p2) { point_coordinates result(p1); result += p2; return result; }
    inline point_coordinates operator+(const point_coordinates& p1, double val) { point_coordinates result(p1); result += val; return result; }
    inline point_coordinates operator-(const point_coordinates& p1, const point_coordinates& p2) { point_coordinates result(p1); result -= p2; return result; }
    inline point_coordinates operator-(const point_coordinates& p1, double val) { point_coordinates result(p1); result += -val; return result; }
    inline point_coordinates operator*(const point_coordinates& p1, double val) { point_coordinates result(p1); result *= val; return result; }
    inline point_coordinates operator/(const point_coordinates& p1, double val) { point_coordinates result(p1); result /= val; return result; }

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v3::point_coordinates> : public fmt::formatter<ae::fmt_helper::float_formatter>
{
    template <typename FormatContext> auto format(const ae::chart::v3::point_coordinates& coord, FormatContext& ctx)
    {
        format_to(ctx.out(), "{{");
        for (auto dim : coord.number_of_dimensions()) {
            if (dim != ae::number_of_dimensions_t{0})
                format_to(ctx.out(), ", ");
            format_val(coord[dim], ctx);
        }
        return format_to(ctx.out(), "}}");
    }
};

// ----------------------------------------------------------------------
