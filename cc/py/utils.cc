#include "py/module.hh"
#include "utils/messages.hh"

// ======================================================================

void ae::py::utils(pybind11::module_& mdl)
{
    using namespace pybind11::literals;

    pybind11::class_<ae::Message>(mdl, "Message")       //
        .def_readonly("message", &ae::Message::message) //
        .def_readonly("context", &ae::Message::context) //
        ;

    pybind11::class_<ae::Messages>(mdl, "Messages") //
        .def("empty", &ae::Messages::empty)                                        //
        .def("unrecognized_locations", &ae::Messages::unrecognized_locations)      //
        .def("messages", &ae::Messages::messages)                                  //
        .def(
            "__iter__", [](const ae::Messages& messages) { return pybind11::make_iterator(messages.messages().begin(), messages.messages().end()); }, pybind11::keep_alive<0, 1>()) //
        ;
}

// ======================================================================
