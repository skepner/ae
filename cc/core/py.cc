#include "ext/pybind11.hh"

class PybTest
{
  public:
    constexpr int test() const { return 19671967; }
};

// ======================================================================

PYBIND11_MODULE(acmacs_e, mdl)
{
    using namespace pybind11::literals;

    mdl.doc() = "Acmacs E backend";

    py::class_<PybTest>(mdl, "PybTest") //
        .def(py::init()) //
        .def_property_readonly("test", &PybTest::test) //
        ;
}

// ======================================================================
