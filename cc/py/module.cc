#include "py/module.hh"

// ======================================================================

class PybTest
{
  public:
    constexpr int test() const { return 19671967; }
};

// ======================================================================

PYBIND11_MODULE(ae_backend, mdl)
{
    using namespace pybind11::literals;

    mdl.doc() = "AE backend";

    ae::py::sequences(mdl);

    pybind11::class_<PybTest>(mdl, "PybTest") //
        .def(pybind11::init()) //
        .def_property_readonly("test", &PybTest::test) //
        ;
}

// ======================================================================
