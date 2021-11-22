#include <cctype>
#include <stdexcept>
#include <charconv>
#include <limits>

#include "utils/enum.hh"
#include "utils/messages.hh"
#include "utils/string.hh"
#include "sequences/fasta.hh"
#include "sequences/translate.hh"
#include "sequences/align.hh"
#include "sequences/seqdb.hh"

#include "py/module.hh"

// ======================================================================

namespace ae::sequences
{
    namespace detail
    {
        inline size_t from_chars(std::string_view src)
        {
            size_t result;
            if (const auto [p, ec] = std::from_chars(&*src.begin(), &*src.end(), result); ec == std::errc{} && p == &*src.end())
                return result;
            else
                return std::numeric_limits<size_t>::max();
        }

    } // namespace detail

    class extract_at_pos_error : public std::runtime_error
    {
      public:
        using std::runtime_error::runtime_error;
    };

    struct aa_nuc_at_pos1_eq_t : public std::tuple<pos1_t, char, bool> // pos (1-based), aa, equal/not-equal
    {
        using std::tuple<pos1_t, char, bool>::tuple;
        constexpr aa_nuc_at_pos1_eq_t() : std::tuple<pos1_t, char, bool>{pos1_t{0}, ' ', false} {}
    };

    using amino_acid_at_pos1_eq_list_t = std::vector<aa_nuc_at_pos1_eq_t>;

    template <size_t MIN_SIZE, size_t MAX_SIZE> inline aa_nuc_at_pos1_eq_t extract_aa_nuc_at_pos1_eq(std::string_view source)
    {
        if (source.size() >= MIN_SIZE && source.size() <= MAX_SIZE && std::isdigit(source.front()) && (std::isalpha(source.back()) || source.back() == '-'))
            return {pos1_t{detail::from_chars(source.substr(0, source.size() - 1))}, source.back(), true};
        else if (source.size() >= (MIN_SIZE + 1) && source.size() <= (MAX_SIZE + 1) && source.front() == '!' && std::isdigit(source[1]) && (std::isalpha(source.back()) || source.back() == '-'))
            return {pos1_t{detail::from_chars(source.substr(1, source.size() - 2))}, source.back(), false};
        else
            throw extract_at_pos_error{fmt::format("invalid aa/nuc-pos: \"{}\" (expected 183P or !183P)", source)};
    }

    template <size_t MIN_SIZE, size_t MAX_SIZE> inline amino_acid_at_pos1_eq_list_t extract_aa_nuc_at_pos1_eq_list(std::string_view source)
    {
        const auto fields = ae::string::split(source, ae::string::split_emtpy::remove);
        amino_acid_at_pos1_eq_list_t pos1_aa_eq(fields.size());
        std::transform(std::begin(fields), std::end(fields), std::begin(pos1_aa_eq), [](std::string_view field) { return extract_aa_nuc_at_pos1_eq<MIN_SIZE, MAX_SIZE>(field); });
        return pos1_aa_eq;

    } // acmacs::seqdb::v3::extract_aa_at_pos_eq_list

    inline amino_acid_at_pos1_eq_list_t extract_aa_nuc_at_pos1_eq_list(const std::vector<std::string>& source)
    {
        amino_acid_at_pos1_eq_list_t list(source.size());
        std::transform(std::begin(source), std::end(source), std::begin(list), [](const auto& en) { return extract_aa_nuc_at_pos1_eq<2, 4>(en); });
        return list;

    } // acmacs::seqdb::v3::extract_aa_at_pos1_eq_list

    inline amino_acid_at_pos1_eq_list_t extract_aa_nuc_at_pos1_eq_list(std::string_view source) { return extract_aa_nuc_at_pos1_eq_list<2, 4>(source); }

    // ----------------------------------------------------------------------

    template <typename Seq, typename ARG> inline static bool matches_all(const Seq& seq, ARG data)
    {
        using namespace ae::sequences;
        const auto matches = [&seq](const auto& en) {
            const auto eq = seq[std::get<pos1_t>(en)] == std::get<char>(en);
            return std::get<bool>(en) == eq;
        };
        const auto elts = extract_aa_nuc_at_pos1_eq_list(data);
        return std::all_of(std::begin(elts), std::end(elts), matches);
    }

} // namespace ae::sequences

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
            "order"_a = "+date", pybind11::doc("sequences without date are ordered last if +date or -date ordering used")) //
        .def(
            "move_name_to_beginning", [](SeqdbSelected& selected, const std::vector<std::string>& names) -> SeqdbSelected& { return selected.move_name_to_beginning(names); }, "names"_a,
            pybind11::doc("if name starts with ~ use regex matching (~ removed), if name contains _ use seq_id matching")) //
        .def(
            "move_name_to_beginning", [](SeqdbSelected& selected, const std::string& name) -> SeqdbSelected& { return selected.move_name_to_beginning(std::vector<std::string>{name}); }, "names"_a,
            pybind11::doc("if name starts with ~ use regex matching (~ removed), if name contains _ use seq_id matching")) //
        .def("find_masters", &SeqdbSelected::find_masters)                                                                 //
        .def("remove_hash_duplicates", &SeqdbSelected::remove_hash_duplicates)                                             //
        .def("replace_with_master", &SeqdbSelected::replace_with_master)                                                   //
        ;

    pybind11::class_<SeqdbSeqRef>(seqdb_submodule, "SeqdbSeqRef")                                        //
        .def("seq_id", [](const SeqdbSeqRef& ref) { return ref.seq_id().get(); })                        //
        .def("date", &SeqdbSeqRef::date)                                                                 //
        .def_property_readonly("aa", &SeqdbSeqRef::aa)                                                   //
        .def_property_readonly("nuc", &SeqdbSeqRef::nuc)                                                 //
        .def("lineage", [](const SeqdbSeqRef& ref) -> const std::string& { return ref.entry->lineage; }) //
        .def("has_issues", [](const SeqdbSeqRef& ref) { return ref.seq->issues.has_issues(); })          //
        .def("issues", [](const SeqdbSeqRef& ref) { return ref.seq->issues.to_strings(); })              //
        ;

    pybind11::class_<sequence_aa_t>(mdl, "SequenceAA") //
        .def(
            "__getitem__", [](const sequence_aa_t& seq, size_t pos) { return seq[pos1_t{pos}]; }, "pos"_a, pybind11::doc("pos is 1-based")) //
        .def(
            "__getitem__", [](const sequence_aa_t& seq, std::string_view pos_aa) { return matches_all(seq, pos_aa); }, "pos_aa"_a,
            pybind11::doc("pos_aa: \"193S\", \"!193S\", \"!56N 115E\" (matches all)")) //
        .def("__len__", [](const sequence_aa_t& seq) { return seq.size(); })           //
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
        .def("__len__", [](const sequence_nuc_t& seq) { return seq.size(); })           //
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
