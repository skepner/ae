#include <cctype>
#include <stdexcept>
#include <limits>

#include "utils/enum.hh"
#include "utils/messages.hh"
#include "sequences/fasta.hh"
#include "sequences/translate.hh"
#include "sequences/align.hh"
#include "sequences/seqdb.hh"
#include "sequences/seqdb-selected.hh"
#include "sequences/hamming-distance.hh"

#include "py/module.hh"

// ======================================================================

void ae::py::sequences(pybind11::module_& mdl)
{
    using namespace std::string_view_literals;
    using namespace pybind11::literals;
    using namespace ae::sequences;

    // ----------------------------------------------------------------------

    auto seqdb_submodule = mdl.def_submodule("seqdb", "seqdb access");

    seqdb_submodule.def(
        "for_subtype", [](std::string_view subtype, bool verb) -> Seqdb& { return seqdb_for_subtype(virus::type_subtype_t{subtype}, verb ? verbose::yes : verbose::no); }, "subtype"_a,
        "verbose"_a = false, pybind11::return_value_policy::reference);
    seqdb_submodule.def("save", &seqdb_save);

    pybind11::class_<Seqdb>(seqdb_submodule, "SeqdbForSubtype")                                                           //
        .def("add", &Seqdb::add, "raw_sequence"_a, pybind11::doc{"returns if sequence was added."})                       //
        .def("save", pybind11::overload_cast<const std::filesystem::path&>(&Seqdb::save, pybind11::const_), "filename"_a) //
        .def("select_all", &Seqdb::select_all)                                                                            //
        .def(
            "find_by_seq_id", [](const Seqdb& seqdb, std::string_view seq_id) { return seqdb.find_by_seq_id(seq_id_t{seq_id}, Seqdb::set_master::no); }, "seq_id"_a) //
        .def(
            "find_by_hash", [](const Seqdb& seqdb, std::string_view hash) { return seqdb.find_by_hash(hash_t{hash}); }, "hash"_a) //
        ;

    pybind11::class_<SeqdbSelected, std::shared_ptr<SeqdbSelected>>(seqdb_submodule, "Selected") //
        .def("__len__", [](const SeqdbSelected& selected) { return selected.size(); })           //
        .def(
            "__iter__", [](const SeqdbSelected& selected) { return pybind11::make_iterator(selected.begin(), selected.end()); }, pybind11::keep_alive<0, 1>()) //
        .def(
            "__getitem__",
            [](const SeqdbSelected& selected, ssize_t index) {
                if (static_cast<size_t>(std::abs(index)) >= selected.size())
                    throw pybind11::index_error{fmt::format("SeqdbSelected index ({}) is out of range -{}..{}", index, selected.size() - 1, selected.size() - 1)};
                if (index >= 0)
                    return selected[static_cast<size_t>(index)];
                else
                    return selected[selected.size() - static_cast<size_t>(index)];
            },
            "index"_a, pybind11::keep_alive<0, 1>())                                                                             //
        .def("exclude_with_issue", &SeqdbSelected::exclude_with_issue, "exclude"_a = true, "do_not_exclude_too_short"_a = false) //
        .def("exclude_too_short", &SeqdbSelected::exclude_too_short, "min_aa_length"_a)                                          //
        .def("keep_masters_only", &SeqdbSelected::keep_masters_only, "keep"_a = true)                                            //
        .def(
            "filter_dates", [](SeqdbSelected& selected, std::string_view first, std::string_view last) -> SeqdbSelected& { return selected.filter_dates(first, last); }, "first"_a = std::string_view{},
            "last"_a = std::string_view{}) //
        .def(
            "lineage",
            [](SeqdbSelected& selected, std::string_view lineage) {
                if (!lineage.empty()) {
                    const char lin{static_cast<char>(std::toupper(lineage[0]))};
                    return selected.lineage(std::string_view{&lin, 1});
                }
                else
                    return selected.lineage(std::string_view{});
            },
            "lineage"_a)                                           //
        .def("human", &SeqdbSelected::filter_human)                //
        .def("filter_host", &SeqdbSelected::filter_host, "host"_a) //
        .def(
            "filter_name", [](SeqdbSelected& selected, const std::vector<std::string>& names) -> SeqdbSelected& { return selected.filter_name(names); }, "names"_a,
            pybind11::doc("if name starts with ~ use regex matching (~ removed), if name contains _ use seq_id matching")) //
        .def(
            "filter_name", [](SeqdbSelected& selected, const std::string& name) -> SeqdbSelected& { return selected.filter_name(std::vector<std::string>{name}); }, "names"_a,
            pybind11::doc("if name starts with ~ use regex matching (~ removed), if name contains _ use seq_id matching")) //
        .def(
            "filter_name",
            [](SeqdbSelected& selected, std::string_view name, std::string_view reassortant, std::string_view passage) -> SeqdbSelected& { return selected.filter_name(name, reassortant, passage); },
            "name"_a, "reassortant"_a, "passage"_a,
            pybind11::doc("")) //
        .def(
            "exclude_name", [](SeqdbSelected& selected, const std::vector<std::string>& names) -> SeqdbSelected& { return selected.exclude_name(names); }, "names"_a,
            pybind11::doc("if name starts with ~ use regex matching (~ removed), if name contains _ use seq_id matching")) //
        .def(
            "exclude_name", [](SeqdbSelected& selected, const std::string& name) -> SeqdbSelected& { return selected.exclude_name(std::vector<std::string>{name}); }, "names"_a,
            pybind11::doc("if name starts with ~ use regex matching (~ removed), if name contains _ use seq_id matching")) //
        .def(
            "include_name",
            [](SeqdbSelected& selected, const std::vector<std::string>& names, bool include_with_issue_too) -> SeqdbSelected& { return selected.include_name(names, include_with_issue_too); },
            "names"_a, "include_with_issue_too"_a = false,
            pybind11::doc("if name starts with ~ use regex matching (~ removed), if name contains _ use seq_id matching")) //
        .def(
            "include_name",
            [](SeqdbSelected& selected, const std::string& name, bool include_with_issue_too) -> SeqdbSelected& {
                return selected.include_name(std::vector<std::string>{name}, include_with_issue_too);
            },
            "names"_a, "include_with_issue_too"_a = false,
            pybind11::doc("if name starts with ~ use regex matching (~ removed), if name contains _ use seq_id matching")) //
        .def(
            "filter", [](SeqdbSelected& selected, const std::function<bool(const SeqdbSeqRef&)>& predicate) { return selected.filter(predicate); }, "predicate"_a,
            pybind11::doc("Passed predicate (function with one arg: SeqdbSeqRef object)\nis called for each earlier selected sequence, if it returns True, that sequence is kept.")) //
        .def(
            "exclude", [](SeqdbSelected& selected, const std::function<bool(const SeqdbSeqRef&)>& predicate) { return selected.exclude(predicate); }, "predicate"_a,
            pybind11::doc("Passed predicate (function with one arg: SeqdbSeqRef object)\nis called for each earlier selected sequence, if it returns True, that sequence is removed.")) //
        .def(
            "check", [](SeqdbSelected& selected) {
                if (const auto empty_present = std::count_if(selected.begin(), selected.end(), [](const auto& ref) { return ref.empty(); }); empty_present)
                    AD_WARNING("{} empty entries present in seqdb selection containing {} references", empty_present, selected.size());
                return selected;
            },
            pybind11::doc("Checks if selection has null/empty entries")) //
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
                    fmt::print(stderr, ">> unrecognized soring order {} (+date assumed)", sorting_order);
                return selected.sort(ord);
            },
            "order"_a = "+date", pybind11::doc("sequences without date are ordered last if +date or -date ordering used")) //
        .def(
            "move_name_to_beginning", [](SeqdbSelected& selected, const std::vector<std::string>& names) -> SeqdbSelected& { return selected.move_name_to_beginning(names); }, "names"_a,
            pybind11::doc("if name starts with ~ use regex matching (~ removed), if name contains _ use seq_id matching")) //
        .def(
            "move_name_to_beginning", [](SeqdbSelected& selected, const std::string& name) -> SeqdbSelected& { return selected.move_name_to_beginning(std::vector<std::string>{name}); }, "names"_a,
            pybind11::doc("if name starts with ~ use regex matching (~ removed), if name contains _ use seq_id matching")) //
        .def("find_masters", &SeqdbSelected::find_masters)                                                                 //
        .def("find_clades", pybind11::overload_cast<std::string_view>(&SeqdbSelected::find_clades), "clades_file"_a)       //
        .def("remove_hash_duplicates", &SeqdbSelected::remove_hash_duplicates)                                             //
        .def("replace_with_master", &SeqdbSelected::replace_with_master)                                                   //
        .def("length_stat", &SeqdbSelected::length_stat)                                                                   //
        .def("max_length", [](const SeqdbSelected& selected) { const auto ml = selected.max_length(); return std::pair(*ml.first, *ml.second); })                                                                     //
        ;

    pybind11::class_<SeqdbSeqRef>(seqdb_submodule, "SeqdbSeqRef")             //
        .def("empty", &SeqdbSeqRef::empty)                                    //
        .def("__bool__", [](const SeqdbSeqRef& ref) { return !ref.empty(); }) //
        .def("is_master", &SeqdbSeqRef::is_master)                            //

        .def("seq_id", [](const SeqdbSeqRef& ref) { return ref.seq_id().get(); })                                                                               //
        .def("name", [](const SeqdbSeqRef& ref) { return ref.entry->name; })                                                                                    //
        .def("date", &SeqdbSeqRef::date)                                                                                                                        //
        .def("passage", [](const SeqdbSeqRef& ref) { return !ref.seq->passages.empty() ? ref.seq->passages.front() : std::string{}; })                          //
        .def("reassortant", [](const SeqdbSeqRef& ref) { return !ref.seq->reassortants.empty() ? ref.seq->reassortants.front() : std::string{}; })              //
        .def("annotations", [](const SeqdbSeqRef& ref) { return !ref.seq->annotations.empty() ? ae::string::join(" ", ref.seq->annotations) : std::string{}; }) //
        .def_property_readonly("aa", &SeqdbSeqRef::aa)                                                                                                          //
        .def_property_readonly("nuc", &SeqdbSeqRef::nuc)                                                                                                        //
        .def("lineage", [](const SeqdbSeqRef& ref) -> const std::string& { return ref.entry->lineage; })                                                        //
        .def_property_readonly("clades", [](const SeqdbSeqRef& ref) -> std::vector<std::string> { return to_vector_base_t(ref.clades); })                       //
        .def("has_issues", [](const SeqdbSeqRef& ref) { return ref.seq->issues.has_issues(); })                                                                 //
        .def("issues", [](const SeqdbSeqRef& ref) { return ref.seq->issues.to_strings(); })                                                                     //
        .def("has_insertions", [](const SeqdbSeqRef& ref) { return !ref.seq->aa_insertions.empty(); })                                                          //
        .def("insertions", [](const SeqdbSeqRef& ref) { return ref.seq->aa_insertions; })                                                                       //
        .def("country", [](const SeqdbSeqRef& ref) { return ref.entry->country; })                                                                              //
        .def("continent", [](const SeqdbSeqRef& ref) { return ref.entry->continent; })                                                                          //
        .def("host", [](const SeqdbSeqRef& ref) { return ref.entry->host; })                                                                                    //
        .def("hash", [](const SeqdbSeqRef& ref) { return *ref.seq->hash; })                                                                                    //
        ;

    pybind11::class_<sequence_aa_t>(mdl, "SequenceAA") //
        .def(
            "__getitem__", [](const sequence_aa_t& seq, size_t pos) { return seq[pos1_t{pos}]; }, "pos"_a, pybind11::doc("pos is 1-based")) //
        .def(
            "__getitem__", [](const sequence_aa_t& seq, std::string_view pos_aa) { return matches_all(seq, pos_aa); }, "pos_aa"_a,
            pybind11::doc("pos_aa: \"193S\", \"!193S\", \"!56N 115E\" (matches all)")) //
        .def("__len__", [](const sequence_aa_t& seq) { return *seq.size(); })           //
        .def("__str__", [](const sequence_aa_t& seq) { return *seq; })                 //
        .def("__bool__", [](const sequence_aa_t& seq) { return !seq.empty(); })        //
        .def(
            "has",
            [](const sequence_aa_t& seq, size_t pos, std::string_view aas) {
                if (aas.size() > 1 && aas[0] == '!')
                    return aas.find(seq[pos1_t{pos}], 1) == std::string_view::npos;
                else
                    return aas.find(seq[pos1_t{pos}]) != std::string_view::npos;
            },
            "pos"_a, "letters"_a,
            pybind11::doc("return if seq has any of the letters at pos. if letters starts with ! then return if none of the letters are at pos")) //
        .def(
            "matches_all", [](const sequence_aa_t& seq, const std::vector<std::string>& pos_aa) { return matches_all(seq, pos_aa); }, "data"_a,
            pybind11::doc(R"(Returns if sequence matches all data entries, e.g. ["197N", "!199T"])")) //
        ;

    pybind11::class_<sequence_nuc_t>(mdl, "SequenceNuc") //
        .def(
            "__getitem__", [](const sequence_nuc_t& seq, size_t pos) { return seq[pos1_t{pos}]; }, "pos"_a, pybind11::doc("pos is 1-based")) //
        .def(
            "__getitem__", [](const sequence_nuc_t& seq, std::string_view pos_nuc) { return matches_all(seq, pos_nuc); }, "pos_nuc"_a,
            pybind11::doc("pos_nuc: \"193A\", \"!193A\", \"!56T 115A\" (matches all)")) //
        .def("__len__", [](const sequence_nuc_t& seq) { return *seq.size(); })           //
        .def("__str__", [](const sequence_nuc_t& seq) { return *seq; })                 //
        .def("__bool__", [](const sequence_nuc_t& seq) { return !seq.empty(); })        //
        .def(
            "has",
            [](const sequence_nuc_t& seq, size_t pos, std::string_view nucs) {
                if (nucs.size() > 1 && nucs[0] == '!')
                    return nucs.find(seq[pos1_t{pos}], 1) == std::string_view::npos;
                else
                    return nucs.find(seq[pos1_t{pos}]) != std::string_view::npos;
            },
            "pos"_a, "letters"_a,
            pybind11::doc("return if seq has any of the letters at pos. if letters starts with ! then return if none of the letters are at pos")) //
        .def(
            "matches_all", [](const sequence_nuc_t& seq, const std::vector<std::string>& pos_nuc) { return matches_all(seq, pos_nuc); }, "data"_a,
            pybind11::doc(R"(Returns if sequence matches all data entries, e.g. ["197A", "!199T"])")) //
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
        .def_readwrite("passage", &RawSequence::passage)
        .def_readwrite("gisaid_dna_accession_no", &RawSequence::gisaid_dna_accession_no)
        .def_readwrite("gisaid_dna_insdc", &RawSequence::gisaid_dna_insdc)
        .def_readwrite("gisaid_identifier", &RawSequence::gisaid_identifier)
        .def_readwrite("gisaid_last_modified", &RawSequence::gisaid_last_modified)
        .def_readwrite("gisaid_submitter", &RawSequence::gisaid_submitter)
        .def_readwrite("gisaid_originating_lab", &RawSequence::gisaid_originating_lab)
        .def_property(
            "lineage", [](const RawSequence& seq) -> const std::string& { return seq.lineage; }, //
            [](RawSequence& seq, std::string_view lineage) { seq.lineage = lineage; })           //
        .def_property(
            "type_subtype",                                                                                                                                                         //
            [](const RawSequence& seq) { return *seq.type_subtype; },                                                                                                               //
            [](RawSequence& seq, std::string_view type_subtype) { seq.type_subtype = ae::virus::type_subtype_t{type_subtype}; })                                                    //
        .def_property_readonly("aa", [](const RawSequence& sequence) { return sequence.sequence.aa.get(); })                                                                        //
        .def_property_readonly("nuc", [](const RawSequence& sequence) { return sequence.sequence.nuc.get(); })                                                                      //
        .def_property_readonly("raw", [](const RawSequence& sequence) { return sequence.raw_sequence; })                                                                            //
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

    pybind11::class_<fasta::Reader>(raw_sequence_submodule, "FastaReader") //
        .def(pybind11::init<const std::filesystem::path&>(), "filename"_a) //
        .def(
            "__iter__", [](fasta::Reader& reader) { return pybind11::make_iterator(reader.begin(), reader.end()); }, pybind11::keep_alive<0, 1>()) //
        ;

    raw_sequence_submodule.def("translate", &translate, "sequence"_a, "messages"_a);
    raw_sequence_submodule.def("align", &align, "sequence"_a, "messages"_a);
    raw_sequence_submodule.def("calculate_hash", &calculate_hash, "sequence"_a);
    raw_sequence_submodule.def(
        "hamming_distance_raw_sequence", [](const RawSequence& seq1, const RawSequence& seq2) { return ae::sequences::hamming_distance(seq1.raw_sequence, seq2.raw_sequence); }, "seq1"_a, "seq2"_a);

    // ----------------------------------------------------------------------
}

// ======================================================================
