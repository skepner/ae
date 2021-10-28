#include "py/module.hh"
#include "virus/name-parse.hh"

// ----------------------------------------------------------------------

namespace ae::py
{
    struct VirusNameParsingResult
    {
        ae::virus::name::Parts parts;
        // std::string messages;
        ae::Messages messages;
        bool good() const { return messages.empty(); }
    };
} // namespace ae::py

// ======================================================================

void ae::py::virus(pybind11::module_& mdl)
{
    using namespace pybind11::literals;

    mdl.def(
        "virus_name_parse",
        [](pybind11::object source, pybind11::object context) {
            ae::virus::name::parse_settings settings;
            auto parts = ae::virus::name::parse(std::string{pybind11::str(source)}, settings, std::string{pybind11::str(context)});
            return VirusNameParsingResult{std::move(parts), std::move(settings.messages())};
            // return VirusNameParsingResult{ae::virus::name::Parts{}, settings.messages().report()};
        },
        "source"_a, "context"_a = "");

    pybind11::class_<VirusNameParsingResult>(mdl, "VirusNameParsingResult") //
        .def("good", &VirusNameParsingResult::good)                         //
        .def_readonly("parts", &VirusNameParsingResult::parts)              //
        .def_readonly("messages", &VirusNameParsingResult::messages)        //
        ;

    pybind11::class_<ae::virus::name::Parts>(mdl, "VirusNameParts") //
        .def(
            "name", [](const ae::virus::name::Parts& parts, bool mark_extra) { return parts.name(mark_extra ? ae::virus::name::Parts::mark_extra::yes : ae::virus::name::Parts::mark_extra::no); },
            "mark_extra"_a = false) //
        ;
}

// ======================================================================
