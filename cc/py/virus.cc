#include "py/module.hh"
#include "utils/messages.hh"
#include "virus/name-parse.hh"
#include "virus/passage.hh"

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
        ae::virus::passage::deconstructed_t deconstructed;
        ae::Messages messages;

        PassageParsingResult(ae::virus::passage::deconstructed_t&& a_deconstructed, ae::Messages&& a_messages) : deconstructed{std::move(a_deconstructed)}, messages{std::move(a_messages)} {}
        bool good() const { return messages.empty(); }
    };

} // namespace ae::py

// ======================================================================

void ae::py::virus(pybind11::module_& mdl)
{
    using namespace pybind11::literals;

    auto virus_submodule = mdl.def_submodule("virus", "virus api");

    virus_submodule.def(
        "name_parse",
        [](pybind11::object source, std::string_view type_subtype, bool trace, pybind11::object filename, size_t line_no) {
            ae::virus::name::parse_settings settings{trace ? ae::virus::name::parse_settings::tracing::yes : ae::virus::name::parse_settings::tracing::no, ae::virus::name::parse_settings::report::no,
                                                     type_subtype};
            ae::Messages messages;
            auto parts = ae::virus::name::parse(std::string{pybind11::str(source)}, settings, messages, ae::MessageLocation{std::string{pybind11::str(filename)}, line_no});
            return VirusNameParsingResult{std::move(parts), std::move(messages)};
        },
        "source"_a, "type_subtype"_a = "", "trace"_a = false, "filename"_a = "", "line_no"_a = 0);

    pybind11::class_<VirusNameParsingResult>(virus_submodule, "VirusNameParsingResult") //
        .def("__bool__", &VirusNameParsingResult::good)                                 //
        .def("good", &VirusNameParsingResult::good)                                     //
        .def_readonly("parts", &VirusNameParsingResult::parts)                          //
        .def_readonly("messages", &VirusNameParsingResult::messages)                    //
        ;

    pybind11::class_<ae::virus::name::Parts>(virus_submodule, "VirusNameParts") //
        .def(
            "name", [](const ae::virus::name::Parts& parts, bool mark_extra) { return parts.name(mark_extra ? ae::virus::name::Parts::mark_extra::yes : ae::virus::name::Parts::mark_extra::no); },
            "mark_extra"_a = false)                                                                 //
        .def("host_location_isolation_year", &ae::virus::name::Parts::host_location_isolation_year) //
        .def_readonly("host", &ae::virus::name::Parts::host)                                        //
        .def_readonly("location", &ae::virus::name::Parts::location)                                //
        .def_readonly("isolation", &ae::virus::name::Parts::isolation)                              //
        .def_readonly("year", &ae::virus::name::Parts::year)                                        //
        .def_readonly("reassortant", &ae::virus::name::Parts::reassortant)                          //
        .def_readonly("extra", &ae::virus::name::Parts::extra)                                      //
        .def_readonly("continent", &ae::virus::name::Parts::continent)                              //
        .def_readonly("country", &ae::virus::name::Parts::country)                                  //
        ;

    // ----------------------------------------------------------------------

    virus_submodule.def(
        "passage_parse",
        [](pybind11::object source, bool trace, const std::filesystem::path& filename, size_t line_no) {
            ae::virus::passage::parse_settings settings{trace ? ae::virus::passage::parse_settings::tracing::yes : ae::virus::passage::parse_settings::tracing::no};
            ae::Messages messages;
            auto deconstructed_passage = ae::virus::passage::parse(std::string{pybind11::str(source)}, settings, messages, ae::MessageLocation{filename, line_no});
            return PassageParsingResult{std::move(deconstructed_passage), std::move(messages)};
        },
        "source"_a, "trace"_a = false, "filename"_a = "", "line_no"_a = 0);

    pybind11::class_<PassageParsingResult>(virus_submodule, "PassageParsingResult")                          //
        .def("__bool__", &PassageParsingResult::good)                                                        //
        .def("good", &PassageParsingResult::good)                                                            //
        .def("passage", [](const PassageParsingResult& result) { return result.deconstructed.construct(); }) //
        .def_readonly("messages", &PassageParsingResult::messages)                                           //
        ;

    pybind11::class_<ae::virus::Passage>(virus_submodule, "Passage")        //
        .def("__str__", &ae::virus::Passage::to_string)                     //
        .def("good", &ae::virus::Passage::good)                             //
        .def("empty", &ae::virus::Passage::empty)                           //
        .def("number_of_elements", &ae::virus::Passage::number_of_elements) //
        .def("is_egg", &ae::virus::Passage::is_egg)                         //
        .def("is_cell", &ae::virus::Passage::is_cell)                       //
        .def("without_date", &ae::virus::Passage::without_date)             //
        ;
}

// ======================================================================
