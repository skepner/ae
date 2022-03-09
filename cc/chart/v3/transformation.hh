#pragma once

#include <array>

#include "utils/named-type.hh"
#include "utils/log.hh"
#include "chart/v3/point-coordinates.hh"
#include "draw/v2/rotation.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    inline double sqr(double value) { return value * value; }

    // ----------------------------------------------------------------------

    namespace detail
    {
        using TransformationBase = std::array<double, 16>;
        constexpr size_t transformation_size = 4;
        constexpr std::array<size_t, transformation_size> transformation_row_base{0, transformation_size, transformation_size * 2, transformation_size * 3};

    } // namespace detail

    // (N+1)xN matrix handling transformation in N-dimensional space. The last row is for translation
    // handles transformation and translation in 2D and 3D
    class Transformation : public detail::TransformationBase
    {
      public:
        Transformation(const Transformation&) = default;
        Transformation(number_of_dimensions_t num_dim = number_of_dimensions_t{2})
            : detail::TransformationBase{1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0}, number_of_dimensions{num_dim}
        {
        }
        Transformation(double a11, double a12, double a21, double a22)
            : detail::TransformationBase{a11, a12, 0.0, 0.0, a21, a22, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0}, number_of_dimensions{2}
        {
        }
        Transformation& operator=(const Transformation&) = default;
        Transformation& operator=(Transformation&&) = default;
        bool operator ==(const Transformation& rhs) const = default;

        void reset(number_of_dimensions_t num_dim) { operator=(Transformation(num_dim)); }

        constexpr double _x(size_t offset) const { return operator[](offset); }
        constexpr double& _x(size_t offset) { return operator[](offset); }
        constexpr double _x(size_t row, size_t column) const { return operator[](detail::transformation_row_base[row] + column); }
        constexpr double& _x(size_t row, size_t column) { return operator[](detail::transformation_row_base[row] + column); }
        template <typename S> constexpr double& operator()(S row, S column) { return _x(static_cast<size_t>(row), static_cast<size_t>(column)); }
        template <typename S> constexpr double operator()(S row, S column) const { return _x(static_cast<size_t>(row), static_cast<size_t>(column)); }

        template <typename S> constexpr double& translation(S dimension) { return _x(detail::transformation_size - 1, static_cast<size_t>(dimension)); }
        template <typename S> constexpr double translation(S dimension) const { return _x(detail::transformation_size - 1, static_cast<size_t>(dimension)); }

        std::vector<double> as_vector() const
        {
            switch (*number_of_dimensions) {
                case 2:
                    return {_x(0, 0), _x(0, 1), _x(1, 0), _x(1, 1)};
                case 3:
                    return {_x(0, 0), _x(0, 1), _x(0, 2), _x(1, 0), _x(1, 1), _x(1, 2), _x(2, 0), _x(2, 1), _x(2, 2)};
            }
            return {};
        }

        template <typename Iterator> Transformation& set(Iterator first, size_t size)
        {
            switch (size) {
                case 4:
                    _x(0, 0) = *first++;
                    _x(0, 1) = *first++;
                    _x(1, 0) = *first++;
                    _x(1, 1) = *first++;
                    break;
                case 9:
                    _x(0, 0) = *first++;
                    _x(0, 1) = *first++;
                    _x(0, 2) = *first++;
                    _x(1, 0) = *first++;
                    _x(1, 1) = *first++;
                    _x(1, 2) = *first++;
                    _x(2, 0) = *first++;
                    _x(2, 1) = *first++;
                    _x(2, 2) = *first++;
            }
            return *this;
        }

        // 2D --------------------------------------------------

        constexpr double a() const { return _x(0, 0); }
        constexpr double& a() { return _x(0, 0); }
        constexpr double b() const { return _x(0, 1); }
        constexpr double& b() { return _x(0, 1); }
        constexpr double c() const { return _x(1, 0); }
        constexpr double& c() { return _x(1, 0); }
        constexpr double d() const { return _x(1, 1); }
        constexpr double& d() { return _x(1, 1); }

        Transformation& set(double a11, double a12, double a21, double a22)
        {
            a() = a11;
            b() = a12;
            c() = a21;
            d() = a22;
            return *this;
        }

        void rotate(ae::draw::v2::Rotation aAngle) {
            const double cos = std::cos(*aAngle);
            const double sin = std::sin(*aAngle);
            const double r0 = cos * a() + -sin * c();
            const double r1 = cos * b() + -sin * d();
            c() = sin * a() + cos * c();
            d() = sin * b() + cos * d();
            a() = r0;
            b() = r1;
        }

        void flip_transformed(double x, double y)
        {
            const double x2y2 = x * x - y * y, xy = 2 * x * y;
            const double r0 = x2y2 * a() + xy * c();
            const double r1 = x2y2 * b() + xy * d();
            c() = xy * a() + -x2y2 * c();
            d() = xy * b() + -x2y2 * d();
            a() = r0;
            b() = r1;
        }

        // reflect about a line specified with vector [aX, aY]
        void flip(double aX, double aY)
        {
            // vector [aX, aY] must be first transformed using inversion of this
            const auto inv = inverse();
            const double x = aX * inv.a() + aY * inv.c();
            const double y = aX * inv.b() + aY * inv.d();
            flip_transformed(x, y);
        }

        void multiply_by(const Transformation& t)
        {
            const auto r0 = a() * t.a() + b() * t.c();
            const auto r1 = a() * t.b() + b() * t.d();
            const auto r2 = c() * t.a() + d() * t.c();
            const auto r3 = c() * t.b() + d() * t.d();
            a() = r0;
            b() = r1;
            c() = r2;
            d() = r3;
        }

        // LineDefinedByEquation transform(LineDefinedByEquation source) const
        // {
        //     const auto p1 = transform(PointCoordinates{0, source.intercept()});
        //     const auto p2 = transform(PointCoordinates{1, source.slope() + source.intercept()});
        //     const auto slope = (p2.y() - p1.y()) / (p2.x() - p1.x());
        //     return {slope, p1.y() - slope * p1.x()};
        // }

        double determinant() const { return a() * d() - b() * c(); }

        class singular : public std::exception
        {
        };

        Transformation inverse() const
        {
            const auto deter = determinant();
            if (float_zero(deter))
                throw singular{};
            return {d() / deter, -b() / deter, -c() / deter, a() / deter};
        }

        double difference(const Transformation& another) const
        {
            if (number_of_dimensions != another.number_of_dimensions)
                throw std::runtime_error{AD_FORMAT("cannot compare transformations with different number_of_dimensions")};
            double sum{0.0};
            for (const auto i1 : number_of_dimensions)
                for (const auto i2 : number_of_dimensions)
                    sum += sqr(_x(*i1, *i2) - another._x(*i1, *i2));
            return sum;
        }

        // 2D and 3D --------------------------------------------------

        point_coordinates transform(const point_coordinates& source) const
        {
            switch (*source.number_of_dimensions()) {
                case 2:
                    return point_coordinates{source.x() * a() + source.y() * c(), source.x() * b() + source.y() * d()};
                case 3:
                    return point_coordinates{
                        source.x() * _x(0, 0) + source.y() * _x(1, 0) + source.z() * _x(2, 0),
                        source.x() * _x(0, 1) + source.y() * _x(1, 1) + source.z() * _x(2, 1),
                        source.x() * _x(0, 2) + source.y() * _x(1, 2) + source.z() * _x(2, 2)
                    };
            }
            throw std::runtime_error{AD_FORMAT("invalid number_of_dimensions in point_coordinates")};
        }

        template <typename Iter> void transform(Iter first, Iter last) const
        {
            for (; first != last; ++first)
                *first = transform(*first);
        }

        // 3D --------------------------------------------------

        number_of_dimensions_t number_of_dimensions{2};

        bool valid() const
        {
            return std::none_of(begin(), end(), [](auto val) { return std::isnan(val); });
        }

    }; // class Transformation

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v3::Transformation> : public fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatContext> auto format(const ae::chart::v3::Transformation& transformation, FormatContext& ctx)
    {
        switch (*transformation.number_of_dimensions) {
            case 2:
                return format_to(ctx.out(), "[{}, {}, {}, {}]", transformation.a(), transformation.b(), transformation.c(), transformation.d());
            case 3:
                return format_to(ctx.out(), "[[{}, {}, {}], [{}, {}, {}], [{}, {}, {}]]", transformation._x(0, 0), transformation._x(0, 1), transformation._x(0, 2), transformation._x(1, 0),
                                 transformation._x(1, 1), transformation._x(1, 2), transformation._x(2, 0), transformation._x(2, 1), transformation._x(2, 2));
        }
        return format_to(ctx.out(), "");
    }
};

// ----------------------------------------------------------------------
