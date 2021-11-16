#include "py/module.hh"
#include "utils/messages.hh"
#include "virus/name-parse.hh"
#include "virus/passage-parse.hh"

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

    struct PassageParsingResult
    {
        ae::virus::passage::passage_deconstructed_t deconstructed;
        ae::Messages messages;

        PassageParsingResult(ae::virus::passage::passage_deconstructed_t&& a_deconstructed, ae::Messages&& a_messages) : deconstructed{std::move(a_deconstructed)}, messages{std::move(a_messages)} {}
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
            "mark_extra"_a = false)                                                                 //
        .def("host_location_isolation_year", &ae::virus::name::Parts::host_location_isolation_year) //
        .def_readonly("host", &ae::virus::name::Parts::host)                                        //
        .def_readonly("year", &ae::virus::name::Parts::year)                                        //
        .def_readonly("reassortant", &ae::virus::name::Parts::reassortant)                          //
        .def_readonly("extra", &ae::virus::name::Parts::extra)                                      //
        .def_readonly("continent", &ae::virus::name::Parts::continent)                              //
        .def_readonly("country", &ae::virus::name::Parts::country)                                  //
        ;

    // ----------------------------------------------------------------------

    mdl.def(
        "passage_parse",
        [](pybind11::object source, pybind11::object filename, size_t line_no) {
            ae::virus::passage::parse_settings settings;
            ae::Messages messages;
            auto deconstructed_passage = ae::virus::passage::parse(std::string{pybind11::str(source)}, settings, messages, ae::MessageLocation{std::string{pybind11::str(filename)}, line_no});
            return PassageParsingResult{std::move(deconstructed_passage), std::move(messages)};
        },
        "source"_a, "filename"_a = "", "line_no"_a = 0);

    pybind11::class_<PassageParsingResult>(mdl, "PassageParsingResult")                                      //
        .def("good", &PassageParsingResult::good)                                                            //
        .def("passage", [](const PassageParsingResult& result) { return result.deconstructed.construct(); }) //
        .def_readonly("messages", &PassageParsingResult::messages)                                           //
        ;
}

// ======================================================================
