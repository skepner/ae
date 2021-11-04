#include "py/module.hh"
#include "sequences/fasta.hh"

// ======================================================================

void ae::py::sequences(pybind11::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace ae::sequences::fasta;

    pybind11::class_<RawSequence, std::shared_ptr<RawSequence>>(mdl, "Fasta_RawSequence") //
        .def_readwrite("name", &RawSequence::name)
        .def_readwrite("date", &RawSequence::date)
        .def_readwrite("accession_number", &RawSequence::accession_number)
        .def_readwrite("type_subtype", &RawSequence::type_subtype)
        .def_readwrite("lab", &RawSequence::lab)
        .def_readwrite("lab_id", &RawSequence::lab_id)
        .def_readwrite("lineage", &RawSequence::lineage)
        .def_readwrite("passage", &RawSequence::passage)
        .def_readwrite("gisaid_dna_accession_no", &RawSequence::gisaid_dna_accession_no)
        .def_readwrite("gisaid_dna_insdc", &RawSequence::gisaid_dna_insdc)
        .def_readwrite("gisaid_identifier", &RawSequence::gisaid_identifier)
        .def_readwrite("gisaid_last_modified", &RawSequence::gisaid_last_modified)
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
