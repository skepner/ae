#include "py/module.hh"
#include "virus/name-parse.hh"

// ======================================================================

void ae::py::virus(pybind11::module_& mdl)
{
    using namespace pybind11::literals;

    mdl.def("virus_name_parse", [](pybind11::object source, pybind11::object context) {
        ae::virus::name::parse_settings settings;
        const auto parts = ae::virus::name::parse(std::string{pybind11::str(source)}, settings, std::string{pybind11::str(context)});
        return std::make_pair(parts, settings.messages());
    }, "source"_a, "context"_a = "");

    pybind11::class_<ae::virus::name::Parts>(mdl, "VirusNameParts")                                                                                                                              //
        .def("name", [](const ae::virus::name::Parts& parts, bool mark_extra) {
            return parts.name(mark_extra ? ae::virus::name::Parts::mark_extra::yes : ae::virus::name::Parts::mark_extra::no); }, "mark_extra"_a = false) //
        ;

}

// ======================================================================
