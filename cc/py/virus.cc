#include "py/module.hh"
#include "virus/name-parse.hh"
#include "utils/messages.hh"

// ----------------------------------------------------------------------

namespace ae::py
{
    struct VirusNameParsingResult
    {
        ae::virus::name::Parts parts;
        ae::Messages messages;

        VirusNameParsingResult(ae::virus::name::Parts&& a_parts, ae::Messages&& a_messages) : parts{std::move(a_parts)}, messages{std::move(a_messages)} {}
        bool good() const { return messages.empty(); }
    };
} // namespace ae::py

// ======================================================================

void ae::py::virus(pybind11::module_& mdl)
{
    using namespace pybind11::literals;

    mdl.def(
        "virus_name_parse",
        [](pybind11::object source, pybind11::object filename, size_t line_no) {
            ae::virus::name::parse_settings settings;
            ae::Messages messages;
            auto parts = ae::virus::name::parse(std::string{pybind11::str(source)}, settings, messages, ae::MessageLocation{std::string{pybind11::str(filename)}, line_no});
            return VirusNameParsingResult{std::move(parts), std::move(messages)};
        },
        "source"_a, "filename"_a = "", "line_no"_a = 0);

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
