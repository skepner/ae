#pragma once

#include "utils/named-type.hh"

// ----------------------------------------------------------------------

namespace ae::draw::v1
{

    class Pixels : public named_double_t<struct Pixels_tag>
    {
      public:
        using named_double_t<struct Pixels_tag>::named_double_t;

        auto value() const { return get(); }
    };

    class Scaled : public named_double_t<struct Scaled_tag>
    {
      public:
        using named_double_t<struct Scaled_tag>::named_double_t;

        auto value() const { return get(); }
    };

    // ----------------------------------------------------------------------

    class Rotation : public named_double_t<struct Rotation_tag>
    {
      public:
        using named_double_t<struct Rotation_tag>::named_double_t;
    };

    inline Rotation RotationDegrees(double aAngle) { return Rotation{aAngle * M_PI / 180.0}; }

    inline Rotation RotationRadiansOrDegrees(double aAngle)
    {
        if (std::abs(aAngle) < 3.15)
            return Rotation{aAngle};
        else
            return RotationDegrees(aAngle);
    }

    inline const Rotation NoRotation{0.0};
    inline const Rotation RotationReassortant{0.5};
    inline const Rotation Rotation90DegreesClockwise{RotationDegrees(90)};
    inline const Rotation Rotation90DegreesAnticlockwise{RotationDegrees(-90)};

    // ----------------------------------------------------------------------

    class Aspect : public named_double_t<struct Aspect_tag>
    {
      public:
        using named_double_t<struct Aspect_tag>::named_double_t;

        auto value() const { return get(); }
    };

    inline const Aspect AspectNormal{1.0};
    inline const Aspect AspectEgg{0.75};

    // ----------------------------------------------------------------------

    inline Pixels operator-(Pixels rhs) noexcept { return Pixels{-rhs.value()}; }
    inline Pixels operator/(Pixels lhs, double rhs) noexcept { return Pixels{lhs.value() / rhs}; }
    inline Pixels operator*(Pixels lhs, double rhs) noexcept { return Pixels{lhs.value() * rhs}; }
    inline Pixels operator*(Pixels lhs, Aspect rhs) noexcept { return Pixels{lhs.value() * rhs.value()}; }

    inline Scaled operator-(Scaled rhs) noexcept { return Scaled{-rhs.value()}; }
    inline Scaled operator/(Scaled lhs, double rhs) noexcept { return Scaled{lhs.value() / rhs}; }
    inline Scaled operator*(Scaled lhs, double rhs) noexcept { return Scaled{lhs.value() * rhs}; }
    inline Scaled operator*(Scaled lhs, Aspect rhs) noexcept { return Scaled{lhs.value() * rhs.value()}; }

} // namespace ae::draw::v1

// ----------------------------------------------------------------------
