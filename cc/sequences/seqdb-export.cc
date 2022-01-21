#include "ext/simdjson.hh"
#include "utils/file.hh"
#include "utils/timeit.hh"
#include "sequences/seqdb.hh"

// ======================================================================

using namespace std::string_view_literals;

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

namespace ae::sequences
{
    static const std::vector<std::pair<std::string_view, std::string_view>> sSeqdbSubtypeToFilename{
        {"B"sv, "seqdb-b.v4.json.xz"sv},        //
        {"A(H1N1)"sv, "seqdb-h1.v4.json.xz"sv}, //
        {"A(H3N2)"sv, "seqdb-h3.v4.json.xz"sv}  //
    };

} // namespace ae::sequences

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

std::filesystem::path ae::sequences::Seqdb::filename() const
{
    const char* seqdb_dir = std::getenv("SEQDB_V4");
    if (!seqdb_dir)
        throw Error{"SEQDB_V4 env variable not set"};
    return std::filesystem::path{seqdb_dir} / std::find_if(sSeqdbSubtypeToFilename.begin(), sSeqdbSubtypeToFilename.end(), [this](const auto& en) { return en.first == subtype_; })->second;

} // ae::sequences::Seqdb::filename

// ----------------------------------------------------------------------

inline void load_array_of_string(std::vector<std::string>& target, simdjson::simdjson_result<simdjson::fallback::ondemand::field>& field)
{
    for (std::string_view value : field.value().get_array())
        target.emplace_back(value);
}

inline void load_seq(ae::sequences::SeqdbSeq& seq, simdjson::ondemand::object& json_seq)
{
    using namespace std::string_view_literals;
    for (auto field : json_seq) {
        const std::string_view key = field.unescaped_key();
        if (key == "a"sv)
            seq.aa = std::string_view{field.value()};
        else if (key == "n"sv)
            seq.nuc = std::string_view{field.value()};
        else if (key == "H"sv)
            seq.hash = std::string_view{field.value()};
        else if (key == "r"sv)
            load_array_of_string(seq.reassortants, field);
        else if (key == "A"sv)
            load_array_of_string(seq.annotations, field);
        else if (key == "p"sv)
            load_array_of_string(seq.passages, field);
        else if (key == "i"sv)
            seq.issues = std::string_view{field.value()};
        else if (key == "l"sv) {
            for (auto lab_id_field : field.value().get_object()) {
                auto& lab_entry = seq.lab_ids.emplace_back(std::string_view{lab_id_field.unescaped_key()}, ae::sequences::SeqdbSeq::lab_ids_t{});
                load_array_of_string(lab_entry.second, lab_id_field);
            }
        }
        else if (key == "G"sv) {
            for (auto gisaid_field : field.value().get_object()) {
                const std::string_view gisaid_key = gisaid_field.unescaped_key();
                if (gisaid_key == "i"sv)
                    load_array_of_string(seq.gisaid.accession_number, gisaid_field);
                else if (gisaid_key == "S"sv)
                    load_array_of_string(seq.gisaid.gisaid_submitter, gisaid_field);
                else if (gisaid_key == "m"sv)
                    load_array_of_string(seq.gisaid.gisaid_last_modified, gisaid_field);
                else if (gisaid_key == "o"sv)
                    load_array_of_string(seq.gisaid.gisaid_originating_lab, gisaid_field);
                else if (gisaid_key == "t"sv)
                    load_array_of_string(seq.gisaid.gisaid_identifier, gisaid_field);
                else if (gisaid_key == "D"sv)
                    load_array_of_string(seq.gisaid.gisaid_dna_accession_no, gisaid_field);
                else if (gisaid_key == "d"sv)
                    load_array_of_string(seq.gisaid.gisaid_dna_insdc, gisaid_field);
                else
                    fmt::print("> seqdb load_seq unhandled gisaid key \"{}\"\n", gisaid_key);
            }
        }
        else
            fmt::print("> seqdb load_seq unhandled key \"{}\"\n", key);
    }
}

inline void load_entry(ae::sequences::SeqdbEntry& entry, simdjson::ondemand::object& json_entry)
{
    using namespace std::string_view_literals;
    for (auto field : json_entry) {
        const std::string_view key = field.unescaped_key();
        if (key == "N"sv)
            entry.name = std::string_view{field.value()};
        else if (key == "d"sv)
            load_array_of_string(entry.dates, field);
        else if (key == "C"sv)
            entry.continent = std::string_view{field.value()};
        else if (key == "c"sv)
            entry.country = std::string_view{field.value()};
        else if (key == "l"sv)
            entry.lineage = std::string_view{field.value()};
        else if (key == "h"sv)
            entry.host = std::string_view{field.value()};
        else if (key == "s"sv) {
            for (simdjson::ondemand::object json_seq : field.value().get_array())
                load_seq(entry.seqs.emplace_back(), json_seq);
        }
        else
            fmt::print("> seqdb load_entry unhandled key \"{}\"\n", key);
    }
}

void ae::sequences::Seqdb::load()
{
    Timeit ti{fmt::format("Loading seqdb for {}", subtype_), std::chrono::milliseconds{5000}};
    if (const auto db_filename = filename(); std::filesystem::exists(db_filename)) {
        if (is_verbose())
            fmt::print(">>>> load \"{}\" {}\n", subtype_, db_filename);

        using namespace ae::simdjson;
        try {
            Parser parser{db_filename};
            try {
                for (auto field : parser.doc().get_object()) {
                    const std::string_view key = field.unescaped_key();
                    // fmt::print(">>>> key \"{}\"\n", key);
                    if (key == "  version") {
                        if (const std::string_view ver{field.value()}; ver != "sequence-database-v4")
                            throw Error{"unsupported version: \"{}\"", ver};
                    }
                    else if (key == "size") {
                        entries_.reserve(static_cast<uint64_t>(field.value()));
                    }
                    else if (key == "data") {
                        for (ondemand::object json_entry : field.value().get_array())
                            load_entry(entries_.emplace_back(), json_entry);
                    }
                    else if (key == "subtype") {
                        if (const std::string_view subtype{field.value()}; subtype != subtype_)
                            throw Error{"wrong subtype: \"{}\"", subtype};
                    }
                    else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
                        fmt::print(">> [seqdb] unhandled \"{}\"\n", key);
                }
            }
            catch (simdjson_error& err) {
                fmt::print("> {} parsing error: {} at {} \"{}\"\n", db_filename, err.what(), parser.current_location_offset(), parser.current_location_snippet(50));
            }
        }
        catch (simdjson_error& err) {
            fmt::print("> {} json parser creation error: {} (UNESCAPED_CHARS means a char < 0x20)\n", db_filename, err.what());
        }
        make_hash_index();
        // fmt::print(">>>> loaded \"{}\" {} -> {}\n", subtype_, db_filename, entries_.size());
    }

} // ae::sequences::Seqdb::load

// ----------------------------------------------------------------------

void ae::sequences::Seqdb::make_hash_index()
{
    for (const auto& entry : entries_) {
        for (const auto& seq : entry.seqs) {
            hash_index_all_.try_emplace(seq.hash).first->second.push_back(SeqdbSeqRef{.entry = &entry, .seq = &seq});
            if (seq.is_master())
                hash_index_.try_emplace(seq.hash, entry.name);
        }
    }

} // ae::sequences::Seqdb::make_hash_index

// ----------------------------------------------------------------------

std::string ae::sequences::Seqdb::export_to_string() const
{
    const auto make_str_for_json = [](auto&& src) -> std::string {
        for (char cc : src) {
            if (cc < 0x20 || cc == '"')
                fmt::print(">> string with control character(s): \"{}\" 0x{:02X}\n", src, cc);
        }
        return std::string{src};
    };

    const auto fix_vec_str = [make_str_for_json](const std::vector<std::string>& src) -> std::vector<std::string> {
        for (const auto& elt : src)
            make_str_for_json(elt);
        return src;
    };

    fmt::memory_buffer json;
    fmt::format_to(
        std::back_inserter(json),
        "{{\"_\": \"-*- js-indent-level: 1 -*-\",\n \"  version\": \"sequence-database-v4\",\n \"  date\": \"{:%Y-%m-%d %H:%M %Z}\",\n \"size\": {:d},\n \"subtype\": \"{}\",\n \"data\": [\n",
        fmt::localtime(std::time(nullptr)), entries_.size(), subtype_);

    size_t entry_no{1};
    for (const auto& entry : entries_) {
        fmt::format_to(std::back_inserter(json), "  {{\"N\": \"{}\"", make_str_for_json(entry.name));
        if (!entry.host.empty())
            fmt::format_to(std::back_inserter(json), ", \"h\": \"{}\"", make_str_for_json(entry.host));
        if (entry.lineage)
            fmt::format_to(std::back_inserter(json), ", \"l\": \"{}\"", entry.lineage);
        if (!entry.dates.empty())
            fmt::format_to(std::back_inserter(json), ", \"d\": [\"{}\"]", fmt::join(fix_vec_str(entry.dates), "\", \""));
        if (!entry.continent.empty())
            fmt::format_to(std::back_inserter(json), ", \"C\": \"{}\"", make_str_for_json(entry.continent));
        if (!entry.country.empty())
            fmt::format_to(std::back_inserter(json), ", \"c\": \"{}\"", make_str_for_json(entry.country));
        fmt::format_to(std::back_inserter(json), ",\n   \"s\": [\n");

        size_t seq_no{1};
        for (const auto& seq : entry.seqs) {
            enum class newline { no, yes };
            const auto add_val = [&json, make_str_for_json](std::string_view key, std::string_view formatted, newline nl, bool& comma) {
                if (comma)
                    fmt::format_to(std::back_inserter(json), ",");
                if (nl == newline::yes)
                    fmt::format_to(std::back_inserter(json), "\n   ");
                fmt::format_to(std::back_inserter(json), " \"{}\": {}", make_str_for_json(key), formatted);
                comma = true;
            };
            const auto add_str = [add_val, make_str_for_json](std::string_view key, std::string_view value, newline nl, bool& comma) {
                if (!value.empty())
                    add_val(key, fmt::format("\"{}\"", make_str_for_json(value)), nl, comma);
            };
            const auto add_vec = [add_val, fix_vec_str](std::string_view key, const std::vector<std::string>& value, newline nl, bool& comma) {
                if (!value.empty())
                    add_val(key, fmt::format("[\"{}\"]", fmt::join(fix_vec_str(value), "\", \"")), nl, comma);
            };
            const auto add_lab_ids = [&json, add_vec](const SeqdbSeq::labs_t& lab_ids, bool& comma) {
                if (!lab_ids.empty()) {
                    if (comma)
                        fmt::format_to(std::back_inserter(json), ",");
                    fmt::format_to(std::back_inserter(json), " \"l\": {{");
                    bool comma_lab{false};
                    for (const auto& [lab, ids] : lab_ids)
                        add_vec(lab, ids, newline::no, comma_lab);
                    fmt::format_to(std::back_inserter(json), "}}");
                    comma = true;
                }
            };
            const auto add_gisaid = [&json, add_vec](const SeqdbSeq::gisaid_data_t& gisaid, bool& comma) {
                if (!gisaid.empty()) {
                    if (comma)
                        fmt::format_to(std::back_inserter(json), ",");
                    fmt::format_to(std::back_inserter(json), " \"G\": {{");
                    bool comma_gisaid{false};
                    add_vec("i", gisaid.accession_number, newline::no, comma_gisaid);
                    add_vec("D", gisaid.gisaid_dna_accession_no, newline::no, comma_gisaid);
                    add_vec("d", gisaid.gisaid_dna_insdc, newline::no, comma_gisaid);
                    add_vec("t", gisaid.gisaid_identifier, newline::no, comma_gisaid);

                    // add_vec("m", gisaid.gisaid_last_modified, newline::no, comma_gisaid);
                    // add_vec("S", gisaid.gisaid_submitter, newline::no, comma_gisaid);
                    // add_vec("o", gisaid.gisaid_originating_lab, newline::no, comma_gisaid);

                    fmt::format_to(std::back_inserter(json), "}}");
                    comma = true;
                }
            };

            bool comma{false};
            fmt::format_to(std::back_inserter(json), "   {{");
            const auto nl_after_seq = seq.nuc.empty() ? newline::no : newline::yes;
            const auto nl_after_i = seq.issues.data_.empty() ? nl_after_seq : newline::no;
            add_str("a", seq.aa, newline::yes, comma);
            add_str("n", seq.nuc, newline::yes, comma);
            add_str("i", seq.issues.data_, nl_after_seq, comma);
            add_str("H", seq.hash, nl_after_i, comma);
            add_vec("r", seq.reassortants, newline::no, comma);
            add_vec("A", seq.annotations, newline::no, comma);
            add_vec("p", seq.passages, newline::no, comma);
            add_lab_ids(seq.lab_ids, comma);
            add_gisaid(seq.gisaid, comma);

            if (!seq.nuc.empty())
                fmt::format_to(std::back_inserter(json), "\n   }}");
            else
                fmt::format_to(std::back_inserter(json), "}}");
            if (seq_no < entry.seqs.size())
                fmt::format_to(std::back_inserter(json), ",");
            fmt::format_to(std::back_inserter(json), "\n");

            ++seq_no;
        }

        fmt::format_to(std::back_inserter(json), "   ]\n  }}"); // end-of "s": [], end-of entry {}
        if (entry_no < entries_.size())
            fmt::format_to(std::back_inserter(json), ",");
        fmt::format_to(std::back_inserter(json), "\n");
        ++entry_no;
    }

    fmt::format_to(std::back_inserter(json), " ]\n}}\n");

    return fmt::to_string(json);

} // ae::sequences::Seqdb::export_to_string

// ----------------------------------------------------------------------

void ae::sequences::Seqdb::save() const
{
    const auto db_filename = filename();
    if (modified_ || !std::filesystem::exists(db_filename))
        save(db_filename);

} // ae::sequences::Seqdb::save

// ----------------------------------------------------------------------

void ae::sequences::Seqdb::save(const std::filesystem::path& filename) const
{
    Timeit ti{fmt::format("exporting seqdb {} to {}", subtype_, filename)};
    ae::file::write(filename, export_to_string(), ae::file::force_compression::no, ae::file::backup_file::yes);

} // ae::sequences::Seqdb::save

// ----------------------------------------------------------------------
