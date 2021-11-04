#include "py/module.hh"
#include "sequences/fasta.hh"

// ======================================================================

void ae::py::sequences(pybind11::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace ae::sequences::fasta;

    pybind11::class_<RawSequence, std::shared_ptr<RawSequence>>(mdl, "Fasta_RawSequence") //
        ;

    pybind11::class_<Reader::value_t>(mdl, "Fasta_ReaderValue")                                                   //
        .def_property_readonly("raw_name", [](const Reader::value_t& value) { return value.sequence->raw_name; }) //
        .def_property_readonly("sequence", [](const Reader::value_t& value) { return value.sequence; })           //
        .def_readonly("filename", &Reader::value_t::filename)                                                     //
        .def_readonly("line_no", &Reader::value_t::line_no)                                                       //
        ;

    pybind11::class_<Reader>(mdl, "FastaReader")                                                                           //
        .def(pybind11::init([](pybind11::object path) { return Reader(std::string{pybind11::str(path)}); }), "filename"_a) //
        .def(
            "__iter__", [](Reader& reader) { return pybind11::make_iterator(reader.begin(), reader.end()); }, pybind11::keep_alive<0, 1>()) //
        ;
}

// ======================================================================
