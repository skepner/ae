// #include <unordered_map>
#include <memory>
#include <cstdlib>

#include "utils/file.hh"
#include "sequences/seqdb.hh"
#include "sequences/raw-sequence.hh"

// ======================================================================

using namespace std::string_view_literals;

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

namespace ae::sequences
{
    struct SeqdbListElement
    {
        std::string_view subtype;
        std::unique_ptr<Seqdb> db{};

        SeqdbListElement(std::string_view a_subtype) : subtype{a_subtype} {}
        SeqdbListElement(const SeqdbListElement& src) : subtype{src.subtype} {}
    };

    static std::vector<SeqdbListElement> sSeqdb{
        "B"sv, //
        "A(H1N1)"sv, //
        "A(H3N2)"sv, //
    };

    static const std::vector<std::pair<std::string_view, std::string_view>> sSeqdbSubtypeToFilename{
        {"B"sv, "seqdb-b.v4.jxz"sv},        //
        {"A(H1N1)"sv, "seqdb-h1.v4.jxz"sv}, //
        {"A(H3N2)"sv, "seqdb-h3.v4.jxz"sv}  //
    };

} // namespace ae::sequences

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

ae::sequences::Seqdb& ae::sequences::seqdb_for_subtype(std::string_view subtype)
{
    if (auto found = std::find_if(sSeqdb.begin(), sSeqdb.end(), [subtype](const auto& en) { return en.subtype == subtype; }); found != sSeqdb.end()) {
        if (!found->db)
            found->db = std::make_unique<Seqdb>(subtype);
        return *found->db;
    }
    else
        throw std::runtime_error{fmt::format("seqbd: unsupported subtype: \"{}\"", subtype)};

} // ae::sequences::seqdb

// ----------------------------------------------------------------------

void ae::sequences::seqdb_save()
{
    for (const auto& [subtype, seqdb] : sSeqdb) {
        if (seqdb)
            seqdb->save();
    }

} // ae::sequences::seqdb_save

// ----------------------------------------------------------------------

ae::sequences::Seqdb::Seqdb(std::string_view subtype) : subtype_{subtype}
{
    if (const auto db_filename = filename(); std::filesystem::exists(db_filename)) {
        // load
        // build hash_index_
    }

} // ae::sequences::Seqdb::Seqdb

// ----------------------------------------------------------------------

std::filesystem::path ae::sequences::Seqdb::filename() const
{
    const char* seqdb_dir = std::getenv("SEQDB_V4");
    if (!seqdb_dir)
        throw std::runtime_error{"SEQDB_V4 env variable not set"};
    return std::filesystem::path{seqdb_dir} / std::find_if(sSeqdbSubtypeToFilename.begin(), sSeqdbSubtypeToFilename.end(), [this](const auto& en) { return en.first == subtype_; })->second;

} // ae::sequences::Seqdb::filename

// ----------------------------------------------------------------------

void ae::sequences::Seqdb::add(const RawSequence& raw_sequence)
{
    const auto found_by_hash = find_by_hash(raw_sequence.hash_nuc);
    const bool keep_sequence = !found_by_hash || found_by_hash.entry->name == raw_sequence.name;
    if (keep_sequence)
        hash_index_.try_emplace(raw_sequence.hash_nuc, raw_sequence.name);
    // else
    //     fmt::print(">>>> dont_keep_sequence \"{}\" {} -> \"{}\"\n", raw_sequence.name, raw_sequence.hash_nuc, found_by_hash.entry->name);
    const auto found = std::lower_bound(std::begin(entries_), std::end(entries_), raw_sequence.name, [](const auto& entry, std::string_view nam) { return entry.name < nam; });
    if (found != std::end(entries_) && found->name == raw_sequence.name) {
        modified_ = found->update(raw_sequence, keep_sequence);
    }
    else {
        const auto added = entries_.emplace(found, raw_sequence);
        if (!keep_sequence)
            added->find_by_hash(raw_sequence.hash_nuc)->dont_keep_sequence();
        modified_ = true;
    }

} // ae::sequences::Seqdb::add

// ----------------------------------------------------------------------

ae::sequences::SeqdbEntry::SeqdbEntry(const RawSequence& raw_sequence) : name{raw_sequence.name}
{
    update(raw_sequence);

} // ae::sequences::SeqdbEntry::SeqdbEntry

// ----------------------------------------------------------------------

bool ae::sequences::SeqdbEntry::update(const RawSequence& raw_sequence, bool keep_sequence)
{
    // fmt::print("SeqdbEntry::update {}\n", raw_sequence.name);
    bool updated = add_date(raw_sequence.date);

    if (!raw_sequence.lineage.empty()) {
        if (lineage.empty())
            lineage = raw_sequence.lineage;
        else if (lineage != raw_sequence.lineage)
            fmt::print(">> SeqdbEntry::update \"{}\": conflicting lineage old:{} new:{}\n", name, lineage, raw_sequence.lineage);
    }
    if (!raw_sequence.continent.empty()) {
        if (continent.empty())
            continent = raw_sequence.continent;
        else if (continent != raw_sequence.continent)
            fmt::print(">> SeqdbEntry::update \"{}\": conflicting continent old:{} new:{}\n", name, continent, raw_sequence.continent);
    }
    if (!raw_sequence.country.empty()) {
        if (country.empty())
            country = raw_sequence.country;
        else if (country != raw_sequence.country)
            fmt::print(">> SeqdbEntry::update \"{}\": conflicting country old:{} new:{}\n", name, country, raw_sequence.country);
    }

    if (auto found = std::find_if(std::begin(seqs), std::end(seqs), [&raw_sequence](const auto& seq) { return seq.hash == raw_sequence.hash_nuc; }); found == std::end(seqs)) {
        auto& seq = seqs.emplace_back();
        updated |= seq.update(raw_sequence, keep_sequence);
    }
    else {
        updated |= found->update(raw_sequence, keep_sequence);
    }

    return updated;

} // ae::sequences::Seqdb::update

// ----------------------------------------------------------------------

bool ae::sequences::SeqdbEntry::add_date(std::string_view date)
{
    bool added { false };
    if (date.size() == 10) {    // only full dates used
        if (const auto found = std::lower_bound(dates.begin(), dates.end(), date); found == dates.end() || *found != date) {
            dates.emplace(found, date);
            added = true;
        }
    }
    return added;

} // ae::sequences::SeqdbEntry::add_date

// ----------------------------------------------------------------------

bool ae::sequences::SeqdbSeq::update(const RawSequence& raw_sequence, bool keep_sequence) // returns if entry was modified
{
    bool updated{false};
    const auto update_vec = [&updated](auto& vec, std::string_view source) {
        if (!source.empty() && std::find(std::begin(vec), std::end(vec), source) == std::end(vec)) {
            vec.emplace_back(source);
            updated = true;
        }
    };

    if (keep_sequence) {
        if (aa != raw_sequence.sequence.aa) {
            aa = raw_sequence.sequence.aa;
            updated = true;
        }
        if (nuc != raw_sequence.sequence.nuc) {
            nuc = raw_sequence.sequence.nuc;
            hash = raw_sequence.hash_nuc;
            updated = true;
        }
    }
    else {
        if (!aa.empty() || !nuc.empty())
            throw std::runtime_error{fmt::format("SeqdbSeq::update keep_sequence={} raw_sequence.hash={} hash={} sequence present", keep_sequence, raw_sequence.hash_nuc, hash)};
        if (hash.empty()) {
            hash = raw_sequence.hash_nuc;
            updated = true;
        }
        else if (hash != raw_sequence.hash_nuc)
            throw std::runtime_error{fmt::format("SeqdbSeq::update keep_sequence={} raw_sequence.hash={} hash={} hash difference", keep_sequence, raw_sequence.hash_nuc, hash)};
    }

    update_vec(annotations, raw_sequence.annotations);
    update_vec(reassortants, raw_sequence.reassortant);
    update_vec(passages, raw_sequence.passage);
    updated |= issues.update(raw_sequence.issues);
    if (!raw_sequence.lab.empty()) {
        if (auto lab = std::find_if(std::begin(lab_ids), std::end(lab_ids), [&raw_sequence](const auto& en) { return en.first == raw_sequence.lab; }); lab != std::end(lab_ids))
            update_vec(lab->second, raw_sequence.lab_id);
        else
            lab_ids.emplace_back(raw_sequence.lab, lab_ids_t{raw_sequence.lab_id});
        updated = true;
    }

    update_vec(gisaid.accession_number, raw_sequence.accession_number);
    update_vec(gisaid.gisaid_dna_accession_no, raw_sequence.gisaid_dna_accession_no);
    update_vec(gisaid.gisaid_dna_insdc, raw_sequence.gisaid_dna_insdc);
    update_vec(gisaid.gisaid_identifier, raw_sequence.gisaid_identifier);
    update_vec(gisaid.gisaid_last_modified, raw_sequence.gisaid_last_modified);
    update_vec(gisaid.gisaid_submitter, raw_sequence.gisaid_submitter);
    update_vec(gisaid.gisaid_originating_lab, raw_sequence.gisaid_originating_lab);

    return updated;

} // ae::sequences::SeqdbSeq::update

// ----------------------------------------------------------------------

void ae::sequences::Seqdb::save()
{
    const auto db_filename = filename();
    if (modified_ || !std::filesystem::exists(db_filename)) {
        // std::time_t now = std::time(nullptr);
        fmt::memory_buffer json;
        fmt::format_to(
            std::back_inserter(json),
            "{{\"_\": \"-*- js-indent-level: 1 -*-\",\n \"  version\": \"sequence-database-v4\",\n \"  date\": \"{:%Y-%m-%d %H:%M %Z}\",\n \"size\": {:d},\n \"subtype\": \"{}\",\n \"data\": [\n",
            fmt::localtime(std::time(nullptr)), entries_.size(), subtype_);

        size_t no{1};
        for (const auto& entry : entries_) {
            fmt::format_to(std::back_inserter(json), "  {{\"N\": \"{}\"", entry.name);
            if (!entry.lineage.empty())
                fmt::format_to(std::back_inserter(json), ", \"l\": \"{}\"", entry.lineage);
            if (!entry.dates.empty())
                fmt::format_to(std::back_inserter(json), ", \"d\": [\"{}\"]", fmt::join(entry.dates, "\", \""));
            if (!entry.continent.empty())
                fmt::format_to(std::back_inserter(json), ", \"C\": \"{}\"", entry.continent);
            if (!entry.country.empty())
                fmt::format_to(std::back_inserter(json), ", \"c\": \"{}\"", entry.country);
            fmt::format_to(std::back_inserter(json), ",\n   \"s\": [\n");

            size_t seq_no{1};
            for (const auto& seq : entry.seqs) {
                enum class newline { no, yes };
                const auto add_val = [&json](std::string_view key, std::string_view formatted, newline nl, bool& comma) {
                    if (comma)
                        fmt::format_to(std::back_inserter(json), ",");
                    if (nl == newline::yes)
                        fmt::format_to(std::back_inserter(json), "\n   ");
                    fmt::format_to(std::back_inserter(json), " \"{}\": {}", key, formatted);
                    comma = true;
                };
                const auto add_str = [add_val](std::string_view key, std::string_view value, newline nl, bool& comma) {
                    if (!value.empty())
                        add_val(key, fmt::format("\"{}\"", value), nl, comma);
                };
                const auto add_vec = [add_val](std::string_view key, const auto& value, newline nl, bool& comma) {
                    if (!value.empty())
                        add_val(key, fmt::format("[\"{}\"]", fmt::join(value, "\", \"")), nl, comma);
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
            if (no < entries_.size())
                fmt::format_to(std::back_inserter(json), ",");
            fmt::format_to(std::back_inserter(json), "\n");
            ++no;
        }

        fmt::format_to(std::back_inserter(json), " ]\n}}\n");
        fmt::print(">>> writing seqdb to {}\n", db_filename);
        ae::file::write(db_filename, fmt::to_string(json), ae::file::force_compression::no, ae::file::backup_file::yes);
        // fmt::print("{}\n", fmt::to_string(json));
    }

} // ae::sequences::Seqdb::save

// ----------------------------------------------------------------------
