#include "utils/named-type.hh"
#include "chart/v3/chart.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    using serum_circle_fold = named_double_t<struct serum_circle_fold_tag>;

    class serum_circles_t
    {
    };

    serum_circles_t serum_circles(const Chart& chart, const Projection& projection, serum_circle_fold fold);
}

// ----------------------------------------------------------------------
