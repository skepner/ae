#include "py/module.hh"
#include "sequences/fasta.hh"

// ======================================================================

void ae::py::sequences(pybind11::module_& mdl)
{
    using namespace pybind11::literals;

    pybind11::class_<ae::sequences::fasta::Reader::value_t>(mdl, "FastaReaderValue")                                                                           //
        .def_readonly("name", &ae::sequences::fasta::Reader::value_t::name)
        .def_readonly("sequence", &ae::sequences::fasta::Reader::value_t::sequence)
        ;

    pybind11::class_<ae::sequences::fasta::Reader>(mdl, "FastaReader")                                                                           //
        .def(pybind11::init([](pybind11::object path) { return ae::sequences::fasta::Reader(std::string{pybind11::str(path)}); }), "filename"_a) //
        .def(
            "__iter__", [](ae::sequences::fasta::Reader& reader) { return pybind11::make_iterator(reader.begin(), reader.end()); }, pybind11::keep_alive<0, 1>()) //
        ;
}

// ======================================================================
