#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <cassert>
#include <limits>
#include <array>
#include <variant>

#include "utils/float.hh"
#include "utils/string.hh"
#include "utils/log.hh"
#include "chart/v2/number-of-dimensions.hh"

// ----------------------------------------------------------------------

namespace ae::draw::v1
{
    using number_of_dimensions_t = ae::chart::v2::number_of_dimensions_t;

    class PointCoordinates
    {
      public:
        constexpr static const double nan = std::numeric_limits<double>::quiet_NaN();
        enum zero2D { zero2D };
        enum zero3D { zero3D };
        enum nan2D { nan2D };
        enum nan3D { nan3D };

        explicit PointCoordinates(number_of_dimensions_t number_of_dimensions, double init = nan)
        {
            switch (*number_of_dimensions) {
              case 2:
                  data_ = Store2D{init, init};
                  break;
              case 3:
                  data_ = Store3D{init, init, init};
                  break;
              default:
                  data_ = StoreXD(*number_of_dimensions);
                  std::fill(begin(), end(), init);
                  break;
            }
        }
        constexpr PointCoordinates(double x, double y) : data_(Store2D{x, y}) {}
        constexpr PointCoordinates(const std::pair<double, double>& xy) : data_(Store2D{xy.first, xy.second}) {}
        constexpr PointCoordinates(double x, double y, double z) : data_(Store3D{x, y, z}) {}
        constexpr PointCoordinates(enum zero2D) : PointCoordinates(0.0, 0.0) {}
        constexpr PointCoordinates(enum zero3D) : PointCoordinates(0.0, 0.0, 0.0) {}
        constexpr PointCoordinates(enum nan2D) : PointCoordinates(nan, nan) {}
        constexpr PointCoordinates(enum nan3D) : PointCoordinates(nan, nan, nan) {}
        PointCoordinates(const double* first, const double* last) : data_(ConstRef{first, static_cast<size_t>(last - first)}) {}
        PointCoordinates(double* first, double* last) : data_(Ref{first, static_cast<size_t>(last - first)}) {}
        PointCoordinates(std::vector<double>::const_iterator first, std::vector<double>::const_iterator last) : PointCoordinates(&*first, &*last) {}
        PointCoordinates(std::vector<double>::iterator first, std::vector<double>::iterator last) : PointCoordinates(&*first, &*last) {}

        PointCoordinates(const PointCoordinates& rhs)
        {
            switch (*rhs.number_of_dimensions()) {
              case 2:
                  data_ = Store2D{rhs.x(), rhs.y()};
                  break;
              case 3:
                  data_ = Store3D{rhs.x(), rhs.y(), rhs.z()};
                  break;
              default:
                  data_ = StoreXD(std::begin(rhs), std::end(rhs));
                  break;
            }
        }

        PointCoordinates& operator=(const PointCoordinates& rhs)
        {
            assert(number_of_dimensions() == rhs.number_of_dimensions());
            std::copy(rhs.begin(), rhs.end(), begin());
            return *this;
        }

        PointCoordinates copy() const noexcept { return *this; }

        bool operator==(const PointCoordinates& rhs) const { return std::equal(begin(), end(), rhs.begin(), rhs.end(), [](double x, double y) { return float_equal_or_both_nan(x, y); }); }
        bool operator!=(const PointCoordinates& rhs) const { return !operator==(rhs); }

        constexpr number_of_dimensions_t number_of_dimensions() const
        {
            return std::visit(
                [](auto&& data) -> number_of_dimensions_t {
                    using T = std::decay_t<decltype(data)>;
                    if constexpr (std::is_same_v<T, Store2D>)
                        return number_of_dimensions_t{2};
                    else if constexpr (std::is_same_v<T, Store3D>)
                        return number_of_dimensions_t{3};
                    else if constexpr (std::is_same_v<T, StoreXD>)
                        return number_of_dimensions_t{data.size()};
                    else
                        return number_of_dimensions_t{data.size};
                },
                data_);
        }

        constexpr double operator[](number_of_dimensions_t dim) const
        { /* assert(dim < number_of_dimensions()); */
            return std::visit(
                [dim](auto&& data) -> double {
                    using T = std::decay_t<decltype(data)>;
                    if constexpr (std::is_same_v<T, Store2D> || std::is_same_v<T, Store3D> || std::is_same_v<T, StoreXD>)
                        return data[*dim];
                    else
                        return data.begin[*dim];
                },
                data_);
        }

        constexpr double& operator[](number_of_dimensions_t dim)
        { /* assert(dim < number_of_dimensions()); */
            return std::visit(
                [dim](auto&& data) -> double& {
                    using T = std::decay_t<decltype(data)>;
                    if constexpr (std::is_same_v<T, Store2D> || std::is_same_v<T, Store3D> || std::is_same_v<T, StoreXD>)
                        return data[*dim];
                    else if constexpr (std::is_same_v<T, ConstRef>) {
                        AD_ERROR("cannot update const PointCoordinates in PointCoordinates::operator[] (abort)");
                        abort();
                    }
                    else
                        return data.begin[*dim];
                },
                data_);
        }

        constexpr const double* begin() const
        {
            return std::visit(
                [](auto&& data) -> const double* {
                    using T = std::decay_t<decltype(data)>;
                    if constexpr (std::is_same_v<T, Store2D> || std::is_same_v<T, Store3D> || std::is_same_v<T, StoreXD>)
                        return &*data.cbegin();
                    else
                        return data.begin;
                },
                data_);
        }
        constexpr const double* end() const
        {
            return std::visit(
                [](auto&& data) -> const double* {
                    using T = std::decay_t<decltype(data)>;
                    if constexpr (std::is_same_v<T, Store2D> || std::is_same_v<T, Store3D> || std::is_same_v<T, StoreXD>)
                        return &*data.cend();
                    else
                        return data.begin + data.size;
                },
                data_);
        }

        constexpr double* begin()
        {
            return std::visit(
                [](auto&& data) -> double* {
                    using T = std::decay_t<decltype(data)>;
                    if constexpr (std::is_same_v<T, Store2D> || std::is_same_v<T, Store3D> || std::is_same_v<T, StoreXD>)
                        return &*data.begin();
                    else if constexpr (std::is_same_v<T, ConstRef>) {
                        AD_ERROR("cannot update const PointCoordinates in PointCoordinates::begin() (abort)");
                        abort();
                    }
                    else
                        return data.begin;
                },
                data_);
        }
        constexpr double* end()
        {
            return std::visit(
                [](auto&& data) -> double* {
                    using T = std::decay_t<decltype(data)>;
                    if constexpr (std::is_same_v<T, Store2D> || std::is_same_v<T, Store3D> || std::is_same_v<T, StoreXD>)
                        return &*data.end();
                    else if constexpr (std::is_same_v<T, ConstRef>) {
                        AD_ERROR("cannot update const PointCoordinates in PointCoordinates::end() (abort)");
                        abort();
                    }
                    else
                        return data.begin + data.size;
                },
                data_);
        }

        constexpr double x() const { return operator[](number_of_dimensions_t{0}); }
        constexpr double y() const { return operator[](number_of_dimensions_t{1}); }
        constexpr double z() const { return operator[](number_of_dimensions_t{2}); }

        constexpr void x(double val) { operator[](number_of_dimensions_t{0}) = val; }
        constexpr void y(double val) { operator[](number_of_dimensions_t{1}) = val; }
        constexpr void z(double val) { operator[](number_of_dimensions_t{2}) = val; }

        PointCoordinates& operator+=(const PointCoordinates& rhs) { std::transform(begin(), end(), rhs.begin(), begin(), [](double v1, double v2) { return v1 + v2; }); return *this; }
        PointCoordinates& operator+=(double val) { std::transform(begin(), end(), begin(), [val](double v1) { return v1 + val; }); return *this; }
        PointCoordinates& operator-=(const PointCoordinates& rhs) { std::transform(begin(), end(), rhs.begin(), begin(), [](double v1, double v2) { return v1 - v2; }); return *this; }
        PointCoordinates& operator*=(double val) { std::transform(begin(), end(), begin(), [val](double v1) { return v1 * val; }); return *this; }
        PointCoordinates& operator/=(double val) { std::transform(begin(), end(), begin(), [val](double v1) { return v1 / val; }); return *this; }

        bool empty() const { return std::any_of(begin(), end(), [](auto val) { return std::isnan(val); }); }
        bool exists() const { return !empty(); }

        std::vector<double> as_vector() const { return {begin(), end()}; }

      private:
        using Store2D = std::array<double, 2>;
        using Store3D = std::array<double, 3>;
        using StoreXD = std::vector<double>;
        struct Ref
        {
            double* begin;
            size_t size; };
        struct ConstRef { const double* const begin; size_t size; };

        std::variant<Store2D, Store3D, StoreXD, Ref, ConstRef> data_;

    }; // class PointCoordinates

    inline double distance2(const PointCoordinates& p1, const PointCoordinates& p2)
    {
        std::vector<double> diff(*p1.number_of_dimensions());
        std::transform(p1.begin(), p1.end(), p2.begin(), diff.begin(), [](double v1, double v2) { return v1 - v2; });
        return std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
    }

    inline double distance(const PointCoordinates& p1, const PointCoordinates& p2) { return std::sqrt(distance2(p1, p2)); }
    inline PointCoordinates middle(const PointCoordinates& p1, const PointCoordinates& p2)
    {
           PointCoordinates result(p1.number_of_dimensions());
           std::transform(p1.begin(), p1.end(), p2.begin(), result.begin(), [](double v1, double v2) { return (v1 + v2) / 2.0; });
           return result;
    }

    inline PointCoordinates operator-(const PointCoordinates& p1) { PointCoordinates result(p1); std::transform(result.begin(), result.end(), result.begin(), [](double val) { return -val; }); return result; }
    inline PointCoordinates operator+(const PointCoordinates& p1, const PointCoordinates& p2) { PointCoordinates result(p1); result += p2; return result; }
    inline PointCoordinates operator+(const PointCoordinates& p1, double val) { PointCoordinates result(p1); result += val; return result; }
    inline PointCoordinates operator-(const PointCoordinates& p1, const PointCoordinates& p2) { PointCoordinates result(p1); result -= p2; return result; }
    inline PointCoordinates operator-(const PointCoordinates& p1, double val) { PointCoordinates result(p1); result += -val; return result; }
    inline PointCoordinates operator*(const PointCoordinates& p1, double val) { PointCoordinates result(p1); result *= val; return result; }
    inline PointCoordinates operator/(const PointCoordinates& p1, double val) { PointCoordinates result(p1); result /= val; return result; }

} // namespace ae::draw::v1

// ----------------------------------------------------------------------

// format for PointCoordinates is format for double of each element, e.g. :.8f

template <> struct fmt::formatter<ae::draw::v1::PointCoordinates> : public fmt::formatter<ae::fmt_helper::float_formatter>
{
    template <typename FormatContext> auto format(const ae::draw::v1::PointCoordinates& coord, FormatContext& ctx)
    {
        ae::chart::v2::number_of_dimensions_t dim{0};
        format_to(ctx.out(), "{{");
        format_val(coord[dim], ctx);
        for (++dim; dim < coord.number_of_dimensions(); ++dim) {
            format_to(ctx.out(), ", ");
            format_val(coord[dim], ctx);
        }
        return format_to(ctx.out(), "}}");
    }
};

// ----------------------------------------------------------------------
