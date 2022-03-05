#pragma once

#include "chart/v3/index.hh"
#include "chart/v3/area.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Layout
    {
      public:
        Layout() = default;

      private:
        std::vector<double> data_;
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
