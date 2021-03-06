#include "py/module.hh"
#include "utils/messages.hh"
#include "utils/file.hh"

// ======================================================================

void ae::py::utils(pybind11::module_& mdl)
{
    using namespace pybind11::literals;

    pybind11::class_<ae::Message>(mdl, "Message")                                                                          //
        .def("__str__", [](const ae::Message& msg) { return fmt::format("{} \"{}\" {}", ae::Message::format_long(msg.type), msg.value, msg.context); })        //
        .def_property_readonly("type", [](const ae::Message& msg) { return fmt::format("{}", msg.type); })                 //
        .def_property_readonly("type_subtype", [](const ae::Message& msg) { return fmt::format("{}", msg.type_subtype); }) //
        .def_readonly("value", &ae::Message::value)                                                                        //
        .def_readonly("context", &ae::Message::context)                                                                    //
        .def_property_readonly("filename", [](const ae::Message& msg) { return msg.location.filename; })                   //
        .def_property_readonly("line_no", [](const ae::Message& msg) { return msg.location.line_no; })                     //
        .def("type_short", [](const ae::Message& msg) { return std::string{ae::Message::format_short(msg.type)}; })        //
        ;

    pybind11::class_<ae::Messages>(mdl, "Messages")                                              //
        .def(pybind11::init())                                                                   //
        .def("empty", &ae::Messages::empty)                                                      //
        .def("unrecognized_locations", &ae::Messages::unrecognized_locations)                    //
        .def("messages", &ae::Messages::messages)                                                //
        .def("__len__", [](const ae::Messages& messages) { return messages.messages().size(); }) //
        .def(
            "__iter__", [](const ae::Messages& messages) { return pybind11::make_iterator(messages.messages().begin(), messages.messages().end()); }, pybind11::keep_alive<0, 1>()) //
        ;

    mdl.def(
        "read_file", [](pybind11::object filename) { return ae::file::read(std::string{pybind11::str(filename)}); }, "filename"_a);
}

// ======================================================================
