#include "chart/v3/index.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Chart;
    class Projection;

    namespace grid_test
    {
        struct settings_t
        {
            double step{0.1};
        };

        struct result_t
        {
            result_t(const Projection& projection) {}
            size_t size() const { return 0; }
        };

        result_t test(const Chart& chart, projection_index projection_no, const settings_t& settings = {});

    } // namespace grid_test
} // namespace ae::chart::v3

// ----------------------------------------------------------------------
