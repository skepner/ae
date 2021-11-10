// #include <unordered_map>
#include <memory>
#include <cstdlib>

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
        found->update(raw_sequence);
    else
        entries_.emplace(found, raw_sequence);

} // ae::sequences::Seqdb::add

// ----------------------------------------------------------------------

ae::sequences::SeqdbEntry::SeqdbEntry(const RawSequence& raw_sequence) : name{raw_sequence.name}
{
    update(raw_sequence);

} // ae::sequences::SeqdbEntry::SeqdbEntry

// ----------------------------------------------------------------------

void ae::sequences::SeqdbEntry::update(const RawSequence& raw_sequence)
{
    fmt::print("SeqdbEntry::update {}\n", raw_sequence.name);
    add_date(raw_sequence.date);

        // std::string continent;
        // std::string country;
        // std::string lineage;
        // std::vector<SeqdbSeq> seqs;

} // ae::sequences::Seqdb::update

// ----------------------------------------------------------------------

void ae::sequences::SeqdbEntry::add_date(std::string_view date)
{
    if (date.size() == 10) {    // only full dates used
        if (const auto found = std::lower_bound(dates.begin(), dates.end(), date); found == dates.end() || *found != date)
            dates.emplace(found, date);
    }

} // ae::sequences::SeqdbEntry::add_date

// ----------------------------------------------------------------------

void ae::sequences::Seqdb::save()
{
    // modified or does not exist
    // backup existing

    // number of entries

} // ae::sequences::Seqdb::save

// ----------------------------------------------------------------------
