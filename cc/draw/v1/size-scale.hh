#pragma once

#include "utils/named-type.hh"

// ----------------------------------------------------------------------

namespace ae::draw::v1
{

    class Pixels : public named_double_t<struct Pixels_tag>
    {
      public:
        using named_double_t<struct Pixels_tag>::named_double_t;

        constexpr auto value() const { return get(); }
    };

    class Scaled : public named_double_t<struct Scaled_tag>
    {
      public:
        using named_double_t<struct Scaled_tag>::named_double_t;

        constexpr auto value() const { return get(); }
    };

    // ----------------------------------------------------------------------

    class Rotation : public named_double_t<struct Rotation_tag>
    {
      public:
        using named_double_t<struct Rotation_tag>::named_double_t;
    };

    constexpr inline Rotation RotationDegrees(double aAngle) { return Rotation{aAngle * M_PI / 180.0}; }

    inline Rotation RotationRadiansOrDegrees(double aAngle)
    {
        if (std::abs(aAngle) < 3.15)
            return Rotation{aAngle};
        else
            return RotationDegrees(aAngle);
    }

    constexpr inline const Rotation NoRotation{0.0};
    constexpr inline const Rotation RotationReassortant{0.5};
    constexpr inline const Rotation Rotation90DegreesClockwise{RotationDegrees(90)};
    constexpr inline const Rotation Rotation90DegreesAnticlockwise{RotationDegrees(-90)};

    // ----------------------------------------------------------------------

    class Aspect : public named_double_t<struct Aspect_tag>
    {
      public:
        using named_double_t<struct Aspect_tag>::named_double_t;

        constexpr auto value() const { return get(); }
    };

    constexpr inline const Aspect AspectNormal{1.0};
    constexpr inline const Aspect AspectEgg{0.75};

    // ----------------------------------------------------------------------

    constexpr Pixels operator-(Pixels rhs) noexcept { return Pixels{-rhs.value()}; }
    constexpr Pixels operator/(Pixels lhs, double rhs) noexcept { return Pixels{lhs.value() / rhs}; }
    constexpr Pixels operator*(Pixels lhs, double rhs) noexcept { return Pixels{lhs.value() * rhs}; }
    constexpr Pixels operator*(Pixels lhs, Aspect rhs) noexcept { return Pixels{lhs.value() * rhs.value()}; }

    constexpr Scaled operator-(Scaled rhs) noexcept { return Scaled{-rhs.value()}; }
    constexpr Scaled operator/(Scaled lhs, double rhs) noexcept { return Scaled{lhs.value() / rhs}; }
    constexpr Scaled operator*(Scaled lhs, double rhs) noexcept { return Scaled{lhs.value() * rhs}; }
    constexpr Scaled operator*(Scaled lhs, Aspect rhs) noexcept { return Scaled{lhs.value() * rhs.value()}; }

} // namespace ae::draw::v1

// ----------------------------------------------------------------------
