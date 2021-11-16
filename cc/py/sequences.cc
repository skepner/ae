#include "utils/enum.hh"
#include "utils/messages.hh"
#include "sequences/fasta.hh"
#include "sequences/translate.hh"
#include "sequences/align.hh"
#include "sequences/seqdb.hh"

#include "py/module.hh"

// ======================================================================

void ae::py::sequences(pybind11::module_& mdl)
{
    using namespace pybind11::literals;
    using namespace ae::sequences;
    using namespace std::string_view_literals;

    // ----------------------------------------------------------------------

    auto seqdb_submodule = mdl.def_submodule("seqdb", "seqdb access");

    seqdb_submodule.def(
        "for_subtype", [](std::string_view subtype, bool verb) -> Seqdb& { return seqdb_for_subtype(subtype, verb ? verbose::yes : verbose::no); }, "subtype"_a, "verbose"_a = false,
        pybind11::return_value_policy::reference);
    seqdb_submodule.def("save", &seqdb_save);

    pybind11::class_<Seqdb>(seqdb_submodule, "SeqdbForSubtype") //
        .def("add", &Seqdb::add, "raw_sequence"_a)              //
        .def(
            "save", [](const Seqdb& seqdb, pybind11::object filename) { seqdb.save(std::string{pybind11::str(filename)}); }, "filename"_a) //
        .def("select_all", &Seqdb::select_all)                                                                                             //
        ;

    pybind11::class_<SeqdbSelected, std::shared_ptr<SeqdbSelected>>(seqdb_submodule, "Selected") //
        .def("__len__", [](const SeqdbSelected& selected) { return selected.size(); })           //
        .def(
            "__iter__", [](const SeqdbSelected& selected) { return pybind11::make_iterator(selected.begin(), selected.end()); }, pybind11::keep_alive<0, 1>()) //
        .def("exclude_with_issue", &SeqdbSelected::exclude_with_issue, "exclude"_a = true)                                                                     //
        .def(
            "filter_dates", [](SeqdbSelected& selected, std::string_view first, std::string_view last) -> SeqdbSelected& { return selected.filter_dates(first, last); }, "first"_a = std::string_view{},
            "last"_a = std::string_view{})                         //
        .def("human", &SeqdbSelected::filter_human)                //
        .def("filter_host", &SeqdbSelected::filter_host, "host"_a) //
        .def(
            "filter_name", [](SeqdbSelected& selected, const std::vector<std::string>& names) -> SeqdbSelected& { return selected.filter_name(names); }, "names"_a,
            pybind11::doc("if name starts with ~ use regex matching (~ removed), if name contains _ use seq_id matching")) //
        .def(
            "filter_name", [](SeqdbSelected& selected, const std::string& name) -> SeqdbSelected& { return selected.filter_name(std::vector<std::string>{name}); }, "names"_a,
            pybind11::doc("if name starts with ~ use regex matching (~ removed), if name contains _ use seq_id matching")) //
        .def(
            "exclude_name", [](SeqdbSelected& selected, const std::vector<std::string>& names) -> SeqdbSelected& { return selected.exclude_name(names); }, "names"_a,
            pybind11::doc("if name starts with ~ use regex matching (~ removed), if name contains _ use seq_id matching")) //
        .def(
            "exclude_name", [](SeqdbSelected& selected, const std::string& name) -> SeqdbSelected& { return selected.exclude_name(std::vector<std::string>{name}); }, "names"_a,
            pybind11::doc("if name starts with ~ use regex matching (~ removed), if name contains _ use seq_id matching")) //
        .def(
            "sort",
            [](SeqdbSelected& selected, std::string_view sorting_order) -> SeqdbSelected& {
                order ord = order::date_ascending;
                if (sorting_order == "date"sv || sorting_order == "+date"sv)
                    ord = order::date_ascending;
                else if (sorting_order == "-date"sv)
                    ord = order::date_descending;
                else if (sorting_order == "name"sv || sorting_order == "+name"sv)
                    ord = order::name_ascending;
                else if (sorting_order == "-name"sv)
                    ord = order::name_descending;
                else
                    fmt::print(">> unrecognized soring order {} (+date assumed)", sorting_order);
                return selected.sort(ord);
            },
            "order"_a = "+date")                                               //
        .def("find_masters", &SeqdbSelected::find_masters)                     //
        .def("remove_hash_duplicates", &SeqdbSelected::remove_hash_duplicates) //
        .def("replace_with_master", &SeqdbSelected::replace_with_master)       //
        ;

    pybind11::class_<SeqdbSeqRef>(seqdb_submodule, "SeqdbSeqRef")                               //
        .def("seq_id", [](const SeqdbSeqRef& ref) { return ref.seq_id().get(); })               //
        .def("date", &SeqdbSeqRef::date)                                                        //
        .def("aa", &SeqdbSeqRef::aa)                                                            //
        .def("nuc", &SeqdbSeqRef::nuc)                                                          //
        .def("has_issues", [](const SeqdbSeqRef& ref) { return ref.seq->issues.has_issues(); }) //
        .def("issues", [](const SeqdbSeqRef& ref) { return ref.seq->issues.to_strings(); })     //
        ;

    // ----------------------------------------------------------------------

    auto raw_sequence_submodule = mdl.def_submodule("raw_sequence", "creating and processing raw sequences");

    pybind11::class_<RawSequence, std::shared_ptr<RawSequence>>(raw_sequence_submodule, "Sequence") //
        .def_readwrite("name", &RawSequence::name)
        .def_readwrite("host", &RawSequence::host)
        .def_readwrite("continent", &RawSequence::continent)
        .def_readwrite("country", &RawSequence::country)
        .def_readwrite("reassortant", &RawSequence::reassortant)
        .def_readwrite("annotations", &RawSequence::annotations)
        .def_readwrite("date", &RawSequence::date)
        .def_readwrite("accession_number", &RawSequence::accession_number)
        .def_readwrite("lab", &RawSequence::lab)
        .def_readwrite("lab_id", &RawSequence::lab_id)
        .def_readwrite("lineage", &RawSequence::lineage)
        .def_readwrite("passage", &RawSequence::passage)
        .def_readwrite("gisaid_dna_accession_no", &RawSequence::gisaid_dna_accession_no)
        .def_readwrite("gisaid_dna_insdc", &RawSequence::gisaid_dna_insdc)
        .def_readwrite("gisaid_identifier", &RawSequence::gisaid_identifier)
        .def_readwrite("gisaid_last_modified", &RawSequence::gisaid_last_modified)
        .def_readwrite("gisaid_submitter", &RawSequence::gisaid_submitter)
        .def_readwrite("gisaid_originating_lab", &RawSequence::gisaid_originating_lab)
        .def_property(
            "type_subtype",                                                                                                                                                         //
            [](const RawSequence& seq) { return *seq.type_subtype; },                                                                                                               //
            [](RawSequence& seq, std::string_view type_subtype) { seq.type_subtype = ae::virus::type_subtype_t{type_subtype}; })                                                    //
        .def_property_readonly("aa", [](const RawSequence& sequence) { return sequence.sequence.aa.get(); })                                                                        //
        .def_property_readonly("nuc", [](const RawSequence& sequence) { return sequence.sequence.nuc.get(); })                                                                      //
        .def("is_aligned", [](const RawSequence& sequence) { return !sequence.issues.is_set(issue::not_aligned); })                                                                 //
        .def("is_translated", [](const RawSequence& sequence) { return !sequence.issues.is_set(issue::not_translated); })                                                           //
        .def("is_translated_not_aligned", [](const RawSequence& sequence) { return !sequence.issues.is_set(issue::not_translated) && sequence.issues.is_set(issue::not_aligned); }) //
        .def("has_issues", [](const RawSequence& sequence) { return sequence.issues.any(); })                                                                                       //
        .def("issues", [](const RawSequence& sequence) { return sequence.issues.to_strings(); })                                                                                    //
        ;

    pybind11::class_<fasta::Reader::value_t>(raw_sequence_submodule, "ReaderValue")                                      //
        .def_property_readonly("raw_name", [](const fasta::Reader::value_t& value) { return value.sequence->raw_name; }) //
        .def_property_readonly("sequence", [](const fasta::Reader::value_t& value) { return value.sequence; })           //
        .def_readonly("filename", &fasta::Reader::value_t::filename)                                                     //
        .def_readonly("line_no", &fasta::Reader::value_t::line_no)                                                       //
        ;

    pybind11::class_<fasta::Reader>(raw_sequence_submodule, "FastaReader")                                                        //
        .def(pybind11::init([](pybind11::object path) { return fasta::Reader(std::string{pybind11::str(path)}); }), "filename"_a) //
        .def(
            "__iter__", [](fasta::Reader& reader) { return pybind11::make_iterator(reader.begin(), reader.end()); }, pybind11::keep_alive<0, 1>()) //
        ;

    raw_sequence_submodule.def("translate", &translate, "sequence"_a, "messages"_a);
    raw_sequence_submodule.def("align", &align, "sequence"_a, "messages"_a);
    raw_sequence_submodule.def("calculate_hash", &calculate_hash, "sequence"_a);

    // ----------------------------------------------------------------------
}

// ======================================================================
