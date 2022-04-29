#include "py/module.hh"
#include "chart/v3/chart.hh"
#include "chart/v3/styles.hh"

// ----------------------------------------------------------------------

namespace ae::py
{
}

// ----------------------------------------------------------------------

void ae::py::chart_v3_plot_spec(pybind11::module_& chart_v3_submodule)
{
    using namespace pybind11::literals;
    using namespace ae::chart::v3;

    pybind11::class_<Styles>(chart_v3_submodule, "Styles") //
        .def("__len__", &Styles::size) //
        .def("__bool__", [](const Styles& styles) { return !styles.empty(); }) //
        .def(
            "__iter__", [](Styles& styles) { return pybind11::make_iterator(styles.begin(), styles.end()); }, pybind11::keep_alive<0, 1>()) //
        .def(
            "__getitem__", &Styles::find, "name"_a, pybind11::return_value_policy::reference_internal, pybind11::doc("find or add style by name")) //
        ;
}

// ----------------------------------------------------------------------
