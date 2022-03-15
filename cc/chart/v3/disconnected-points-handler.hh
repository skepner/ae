#pragma once

#include <span>

#include "chart/v3/stress.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    // optimization engines do not like NaNs in layout, set disconnected point coordinates to 0 before calling an engine and restore them afterwards
    class DisconnectedPointsHandler
    {
      public:
        DisconnectedPointsHandler(const Stress& stress, std::span<double> args) : stress_{stress}, args_{args} { stress_.set_coordinates_of_disconnected(args_, 0.0, stress_.number_of_dimensions()); }
        DisconnectedPointsHandler(const DisconnectedPointsHandler&) = delete;
        DisconnectedPointsHandler& operator=(const DisconnectedPointsHandler&) = delete;

        ~DisconnectedPointsHandler() { stress_.set_coordinates_of_disconnected(args_, std::numeric_limits<double>::quiet_NaN(), stress_.number_of_dimensions()); }

      private:
        const Stress& stress_;
        std::span<double> args_;
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
