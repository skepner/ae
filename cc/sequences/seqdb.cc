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
    const auto found = std::lower_bound(std::begin(entries_), std::end(entries_), raw_sequence.name, [](const auto& entry, std::string_view nam) { return entry.name < nam; });
    if (found != std::end(entries_) && found->name == raw_sequence.name)
        modified_ = found->update(raw_sequence);
    else {
        entries_.emplace(found, raw_sequence);
        modified_ = true;
    }

} // ae::sequences::Seqdb::add

// ----------------------------------------------------------------------

ae::sequences::SeqdbEntry::SeqdbEntry(const RawSequence& raw_sequence) : name{raw_sequence.name}
{
    update(raw_sequence);

} // ae::sequences::SeqdbEntry::SeqdbEntry

// ----------------------------------------------------------------------

bool ae::sequences::SeqdbEntry::update(const RawSequence& raw_sequence)
{
    // fmt::print("SeqdbEntry::update {}\n", raw_sequence.name);
    bool updated = add_date(raw_sequence.date);

        // std::string continent;
        // std::string country;
        // std::string lineage;
        // std::vector<SeqdbSeq> seqs;

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
            if (!entry.dates.empty())
                fmt::format_to(std::back_inserter(json), ", \"d\": [\"{}\"]", fmt::join(entry.dates, "\", \""));
            // std::string continent;
            // std::string country;
            // std::string lineage;
            fmt::format_to(std::back_inserter(json), ",\n   \"s\": [\n");

            fmt::format_to(std::back_inserter(json), "\n   ]\n  }}"); // end-of "s": [], end-of entry {}
            if (no < entries_.size())
                fmt::format_to(std::back_inserter(json), ",");
            fmt::format_to(std::back_inserter(json), "\n");
            ++no;
        }

        fmt::format_to(std::back_inserter(json), " ]\n}}\n");
        fmt::print("writing seqdb to {}\n", db_filename);
        ae::file::write(db_filename, fmt::to_string(json), ae::file::force_compression::no, ae::file::backup_file::yes);
        fmt::print("{}\n", fmt::to_string(json));
    }

} // ae::sequences::Seqdb::save

// ----------------------------------------------------------------------
