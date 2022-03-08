#pragma once

#include "utils/named-type.hh"

// ----------------------------------------------------------------------

namespace ae::draw::v2
{
    using Rotation = named_double_t<struct Rotation_tag>;
    using Aspect = named_double_t<struct Aspect_tag>;

    // ----------------------------------------------------------------------

    inline const Rotation NoRotation{0.0};

    inline Rotation rotation(double aAngle) {
        if (std::abs(aAngle) < 3.15)
            return Rotation{aAngle};
        else
            return Rotation{aAngle * M_PI / 180.0};
    }

    // ----------------------------------------------------------------------

    inline const Aspect AspectNormal{1.0};
    inline const Aspect AspectEgg{0.75};

} // namespace ae::draw::v2

// ----------------------------------------------------------------------
