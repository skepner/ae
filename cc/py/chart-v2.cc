#include "py/module.hh"
#include "chart/v2/factory-import.hh"
#include "chart/v2/factory-export.hh"
#include "chart/v2/chart-modify.hh"
#include "chart/v2/selected-antigens-sera.hh"
#include "chart/v2/text-export.hh"
#include "chart/v2/grid-test.hh"

// ======================================================================

void ae::py::chart_v2(pybind11::module_& mdl)
{
    using namespace std::string_view_literals;
    using namespace pybind11::literals;
    using namespace ae::chart::v2;

    // ----------------------------------------------------------------------

    auto chart_v2_submodule = mdl.def_submodule("chart_v2", "chart_v2 api");

}

// ======================================================================
