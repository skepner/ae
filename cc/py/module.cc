#include "py/module.hh"
#include "ext/date.hh"

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
    ae::py::virus(mdl);

    // ----------------------------------------------------------------------

    mdl.def(
        "date_format",
        [](const std::string& source, bool allow_incomplete, bool throw_on_error, bool month_first) {
            return ae::date::parse_and_format(source, allow_incomplete ? ae::date::allow_incomplete::yes : ae::date::allow_incomplete::no,
                                              throw_on_error ? ae::date::throw_on_error::yes : ae::date::throw_on_error::no, month_first ? ae::date::month_first::yes : ae::date::month_first::no);
        },
        "source"_a, "allow_incomplete"_a = false, "throw_on_error"_a = true, "month_first"_a = false);

    // ----------------------------------------------------------------------

    pybind11::class_<PybTest>(mdl, "PybTest")          //
        .def(pybind11::init())                         //
        .def_property_readonly("test", &PybTest::test) //
        ;
}

// ======================================================================
