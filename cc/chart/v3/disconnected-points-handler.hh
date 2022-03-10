#pragma once

#include "chart/v3/stress.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    // optimization engines do not like NaNs in layout, set disconnected point coordinates to 0 before calling an engine and restore them afterwards
    class DisconnectedPointsHandler
    {
      public:
        DisconnectedPointsHandler(const Stress& stress, double* arg_first, size_t num_args) : stress_{stress}, arg_first_{arg_first}, num_args_{num_args}
        {
            stress_.set_coordinates_of_disconnected(arg_first_, num_args_, 0.0, stress_.number_of_dimensions());
        }
        DisconnectedPointsHandler(const DisconnectedPointsHandler&) = delete;
        DisconnectedPointsHandler& operator=(const DisconnectedPointsHandler&) = delete;

        ~DisconnectedPointsHandler() { stress_.set_coordinates_of_disconnected(arg_first_, 0ul, std::numeric_limits<double>::quiet_NaN(), stress_.number_of_dimensions()); }

      private:
        const Stress& stress_;
        double* arg_first_{nullptr};
        size_t num_args_{0};
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
