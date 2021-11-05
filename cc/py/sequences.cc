#include "py/module.hh"
#include "sequences/fasta.hh"
#include "sequences/translate.hh"
#include "sequences/align.hh"

// ======================================================================

void ae::py::sequences(pybind11::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace ae::sequences;

    // ----------------------------------------------------------------------

    auto raw_sequence_submodule = mdl.def_submodule("raw_sequence", "creating and processing raw sequences");

    pybind11::class_<RawSequence, std::shared_ptr<RawSequence>>(raw_sequence_submodule, "Sequence") //
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

    pybind11::class_<fasta::Reader::value_t>(raw_sequence_submodule, "ReaderValue")                                                   //
        .def_property_readonly("raw_name", [](const fasta::Reader::value_t& value) { return value.sequence->raw_name; }) //
        .def_property_readonly("sequence", [](const fasta::Reader::value_t& value) { return value.sequence; })           //
        .def_readonly("filename", &fasta::Reader::value_t::filename)                                                     //
        .def_readonly("line_no", &fasta::Reader::value_t::line_no)                                                       //
        ;

    pybind11::class_<fasta::Reader>(raw_sequence_submodule, "FastaReader")                                                                           //
        .def(pybind11::init([](pybind11::object path) { return fasta::Reader(std::string{pybind11::str(path)}); }), "filename"_a) //
        .def(
            "__iter__", [](fasta::Reader& reader) { return pybind11::make_iterator(reader.begin(), reader.end()); }, pybind11::keep_alive<0, 1>()) //
        ;

    raw_sequence_submodule.def("translate", &translate, "sequence"_a);
    raw_sequence_submodule.def("align", &align, "sequence"_a);

    // ----------------------------------------------------------------------

}

// ======================================================================
