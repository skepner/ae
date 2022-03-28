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
    template <typename Storage> class point_coordinates_with_storage
    {
      public:
        using storage_t = Storage;
        using diff_t = typename Storage::difference_type;
        constexpr static const double nan = std::numeric_limits<double>::quiet_NaN();

        point_coordinates_with_storage() = default;
        point_coordinates_with_storage(const point_coordinates_with_storage&) = default;
        point_coordinates_with_storage(point_coordinates_with_storage&&) = default;
        point_coordinates_with_storage& operator=(const point_coordinates_with_storage&) = default;
        point_coordinates_with_storage& operator=(point_coordinates_with_storage&&) = default;

        number_of_dimensions_t number_of_dimensions() const { return number_of_dimensions_t{size()}; }

        double operator[](number_of_dimensions_t dim) const { return data_[*dim]; }
        double& operator[](number_of_dimensions_t dim) { return data_[*dim]; }

        bool empty() const
        {
            return size() == 0 || std::any_of(begin(), end(), [](auto val) { return std::isnan(val); });
        }
        bool exists() const { return !empty(); }

        auto begin() { return data_.begin(); }
        auto begin() const { return data_.begin(); }
        auto end() { return data_.end(); }
        auto end() const { return data_.end(); }

        double back() const { return data_.back(); }

        template <typename S2> point_coordinates_with_storage& operator+=(const point_coordinates_with_storage<S2>& rhs)
        {
            std::transform(begin(), end(), rhs.begin(), begin(), [](double v1, double v2) { return v1 + v2; });
            return *this;
        }

        point_coordinates_with_storage& operator+=(double val)
        {
            std::transform(begin(), end(), begin(), [val](double v1) { return v1 + val; });
            return *this;
        }

        template <typename S2> point_coordinates_with_storage& operator-=(const point_coordinates_with_storage<S2>& rhs)
        {
            std::transform(begin(), end(), rhs.begin(), begin(), [](double v1, double v2) { return v1 - v2; });
            return *this;
        }

        point_coordinates_with_storage& operator*=(double val)
        {
            std::transform(begin(), end(), begin(), [val](double v1) { return v1 * val; });
            return *this;
        }

        point_coordinates_with_storage& operator/=(double val)
        {
            std::transform(begin(), end(), begin(), [val](double v1) { return v1 / val; });
            return *this;
        }

        void set_nan() { std::fill(data_.begin(), data_.end(), nan); }

      protected:
        point_coordinates_with_storage(number_of_dimensions_t dims) : data_(*dims, nan) {}
        template <typename It>
        point_coordinates_with_storage(It first, size_t dims) : data_(&*first, dims) {} // do not use number_of_dimensions_t here to avoid conflict with point_coordinates_ref constructor
        template <typename It> point_coordinates_with_storage(It first, It last) : data_(first, last) {}

        auto size() const { return data_.size(); }
        Storage& storage() { return data_; }
        // const Storage& storage() const { return data_; }

      private:
        Storage data_{};
    };

    // ----------------------------------------------------------------------

    class point_coordinates_ref : public point_coordinates_with_storage<std::span<double, std::dynamic_extent>>
    {
      public:
        using point_coordinates_with_storage<storage_t>::point_coordinates_with_storage;
        template <typename It> point_coordinates_ref(It first, number_of_dimensions_t dims) : point_coordinates_with_storage<storage_t>(first, *dims) {}
    };

    class point_coordinates_ref_const : public point_coordinates_with_storage<std::span<const double, std::dynamic_extent>>
    {
      public:
        using point_coordinates_with_storage<storage_t>::point_coordinates_with_storage;
        template <typename It> point_coordinates_ref_const(It first, number_of_dimensions_t dims) : point_coordinates_with_storage<storage_t>(first, *dims) {}
        point_coordinates_ref_const(const point_coordinates_ref& ref) : point_coordinates_with_storage<storage_t>(&*ref.begin(), &*ref.end()) {}
    };

    class point_coordinates : public point_coordinates_with_storage<std::vector<double>>
    {
      public:
        using point_coordinates_with_storage<storage_t>::point_coordinates_with_storage;
        point_coordinates(number_of_dimensions_t num_dim) : point_coordinates_with_storage<storage_t>(num_dim) {}
        point_coordinates(std::initializer_list<double> vals) : point_coordinates_with_storage<storage_t>(vals.begin(), vals.end()) {}
        template <typename Storage> explicit point_coordinates(const point_coordinates_with_storage<Storage>& src) : point_coordinates_with_storage<storage_t>(src.begin(), src.end()) {}
        template <typename Storage> point_coordinates& operator=(const point_coordinates_with_storage<Storage>& src)
        {
            storage().assign(src.begin(), src.end());
            return *this;
        }
    };

    // ----------------------------------------------------------------------

    template <typename S1, typename S2> inline double distance2(const point_coordinates_with_storage<S1>& p1, const point_coordinates_with_storage<S2>& p2)
    {
        std::vector<double> diff(*p1.number_of_dimensions());
        std::transform(p1.begin(), p1.end(), p2.begin(), diff.begin(), [](double v1, double v2) { return v1 - v2; });
        return std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0);
    }

    template <typename S1, typename S2> inline double distance(const point_coordinates_with_storage<S1>& p1, const point_coordinates_with_storage<S2>& p2) { return std::sqrt(distance2(p1, p2)); }

    template <typename S1, typename S2> inline point_coordinates middle(const point_coordinates_with_storage<S1>& p1, const point_coordinates_with_storage<S2>& p2)
    {
        point_coordinates result(p1.number_of_dimensions());
        std::transform(p1.begin(), p1.end(), p2.begin(), result.begin(), [](double v1, double v2) { return (v1 + v2) / 2.0; });
        return result;
    }

    // ----------------------------------------------------------------------

    template <typename Storage> inline point_coordinates operator-(const point_coordinates_with_storage<Storage>& p1)
    {
        point_coordinates result{p1};
        std::transform(result.begin(), result.end(), result.begin(), [](double val) { return -val; });
        return result;
    }

    template <typename S1, typename S2> inline point_coordinates operator+(const point_coordinates_with_storage<S1>& p1, const point_coordinates_with_storage<S2>& p2)
    {
        point_coordinates result{p1};
        result += p2;
        return result;
    }

    template <typename Storage> inline point_coordinates operator+(const point_coordinates_with_storage<Storage>& p1, double val)
    {
        point_coordinates result{p1};
        result += val;
        return result;
    }

    template <typename S1, typename S2> inline point_coordinates operator-(const point_coordinates_with_storage<S1>& p1, const point_coordinates_with_storage<S2>& p2)
    {
        point_coordinates result{p1};
        result -= p2;
        return result;
    }

    template <typename Storage> inline point_coordinates operator-(const point_coordinates_with_storage<Storage>& p1, double val)
    {
        point_coordinates result{p1};
        result += -val;
        return result;
    }

    template <typename Storage> inline point_coordinates operator*(const point_coordinates_with_storage<Storage>& p1, double val)
    {
        point_coordinates result{p1};
        result *= val;
        return result;
    }

    template <typename Storage> inline point_coordinates operator/(const point_coordinates_with_storage<Storage>& p1, double val)
    {
        point_coordinates result{p1};
        result /= val;
        return result;
    }

} // namespace ae::chart::v3

// // ----------------------------------------------------------------------

template <typename Storage> struct fmt::formatter<ae::chart::v3::point_coordinates_with_storage<Storage>> : public fmt::formatter<ae::fmt_helper::float_formatter>
{
    template <typename FormatContext> auto format(const ae::chart::v3::point_coordinates_with_storage<Storage>& coord, FormatContext& ctx)
    {
        format_to(ctx.out(), "[");
        for (const auto dim : coord.number_of_dimensions()) {
            if (dim != ae::number_of_dimensions_t{0})
                format_to(ctx.out(), ", ");
            format_val(coord[dim], ctx);
        }
        return format_to(ctx.out(), "]");
    }
};

// ----------------------------------------------------------------------
