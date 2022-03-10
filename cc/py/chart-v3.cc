#include "py/module.hh"
#include "chart/v3/chart.hh"

// ======================================================================

namespace ae::py
{
}

// ======================================================================

void ae::py::chart_v3(pybind11::module_& mdl)
{
    using namespace std::string_view_literals;
    using namespace pybind11::literals;
    using namespace ae::chart::v3;

    // ----------------------------------------------------------------------

    auto chart_v3_submodule = mdl.def_submodule("chart_v3", "chart_v3 api");

    pybind11::class_<Chart, std::shared_ptr<Chart>>(chart_v3_submodule, "Chart") //
        // .def(pybind11::init([]() { return std::make_shared<Chart>(); }), pybind11::doc("creates an empty chart"))
        // .def(pybind11::init([](pybind11::object path) { return std::make_shared<Chart>(pybind11::str(path)); }), "filename"_a, pybind11::doc("imports chart from a file"))

        .def(pybind11::init<>(), pybind11::doc("creates an empty chart"))
        .def(pybind11::init<const std::filesystem::path&>(), "filename"_a, pybind11::doc("imports chart from a file"))
        .def("write", &Chart::write, "filename"_a, pybind11::doc("exports chart into a file"))
        ;
}

// ----------------------------------------------------------------------
