#pragma once

#include <cmath>

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    // http://en.wikipedia.org/wiki/Sigmoid_function
    // g(aValue) = 1/(1 + exp(-aValue * aPower))
    inline double sigmoid(double aValue) { return double{1} / (double{1} + std::exp(double{-1} * aValue)); }

    // https://groups.google.com/forum/#!topic/comp.ai.neural-nets/gqekclNH3No
    inline double sigmoid_fast(double aValue)
    {
        const double absx = std::abs(aValue);
        if (absx > static_cast<double>(8.713655)) {
            return aValue > 0 ? 1 : 0;
        }
        else {
            if (const double xx = aValue * aValue; absx > double{4.5})
                return aValue > 0 ? (((static_cast<double>(3.2e-7) * xx - static_cast<double>(8.544e-5)) * xx + static_cast<double>(9.99869e-3)) * aValue + static_cast<double>(0.953157))
                                  : (((static_cast<double>(3.2e-7) * xx - static_cast<double>(8.544e-5)) * xx + static_cast<double>(9.99869e-3)) * aValue + static_cast<double>(0.046843));
            else
                return (((((static_cast<double>(-5e-8) * xx + static_cast<double>(3.6e-6)) * xx - static_cast<double>(1.0621e-4)) * xx + static_cast<double>(1.75410e-3)) * xx -
                         static_cast<double>(0.02045660)) *
                            xx +
                        static_cast<double>(0.24990936)) *
                           aValue +
                       static_cast<double>(0.499985);
        }
    }

    inline double sigmoid_pseudo(double aValue) { return aValue > 0 ? 1 : 0; }

    // sigmoid derivative
    // Alan uses: dSigmoid = sigmoid*(1-sigmoid)
    // Derek uses: dSigmoid = sigmoid * sigmoid * e^(-x)  [both variants are the same]
    // http://atlas.web.cern.ch/Atlas/GROUPS/SOFTWARE/INFO/Workshops/9905/slides/thu.7/sld007.htm suggests dSigmoid = sigmoid / (1 + e^x) which is slower but more accurate for big x
    inline double d_sigmoid(double aValue)
    {
        const auto s = sigmoid(aValue);
        return s * (double{1} - s);
    }

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
