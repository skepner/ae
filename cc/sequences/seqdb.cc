// #include <unordered_map>
#include <memory>
#include <cstdlib>

#include "ext/range-v3.hh"
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

} // namespace ae::sequences

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

ae::sequences::Seqdb& ae::sequences::seqdb_for_subtype(std::string_view subtype, verbose verb)
{
    if (auto found = std::find_if(sSeqdb.begin(), sSeqdb.end(), [subtype](const auto& en) { return en.subtype == subtype; }); found != sSeqdb.end()) {
        if (!found->db)
            found->db = std::make_unique<Seqdb>(subtype);
        found->db->set_verbose(verb);
        return *found->db;
    }
    else
        throw Error{"unsupported subtype: \"{}\"", subtype};

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

std::shared_ptr<ae::sequences::SeqdbSelected> ae::sequences::Seqdb::select_all() const
{
    auto selected = std::make_shared<SeqdbSelected>(*this);
    for (const auto& entry : entries_) {
        for (const auto& seq : entry.seqs)
            selected->refs_.push_back(SeqdbSeqRef{.entry = &entry, .seq = &seq});
    }
    return selected;

} // ae::sequences::Seqdb::select_all

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

    if (!raw_sequence.host.empty()) {
        if (host.empty())
            host = raw_sequence.host;
        else if (host != raw_sequence.host)
            fmt::print(">> SeqdbEntry::update \"{}\": conflicting host old:{} new:{}\n", name, host, raw_sequence.host);
    }
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
            throw Error{"SeqdbSeq::update keep_sequence={} raw_sequence.hash={} hash={} sequence present", keep_sequence, raw_sequence.hash_nuc, hash};
        if (hash.empty()) {
            hash = raw_sequence.hash_nuc;
            updated = true;
        }
        else if (hash != raw_sequence.hash_nuc)
            throw Error{"SeqdbSeq::update keep_sequence={} raw_sequence.hash={} hash={} hash difference", keep_sequence, raw_sequence.hash_nuc, hash};
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

inline ae::sequences::seq_id_t make_seq_id(std::string_view designation)
{
    const auto to_remove = [](char cc) {
        switch (cc) {
          case '(':
          case ')':
          case '[':
          case ']':
          case ':':
          case '\'':
          case ';':
              // unlikely to be in seqdb, but remove them just in case
          case '!':
          case '#':
          case '*':
          case '@':
          case '$':
              return true;
        }
        return false;
    };

    const auto to_replace_with_underscore = [](char cc) {
        switch (cc) {
          case ' ':
          case '&':
          case '=':
              return true;
        }
        return false;
    };

    const auto to_replace_with_slash = [](char cc) {
        switch (cc) {
          case ',':
          case '+':
              return true;
        }
        return false;
    };

    const std::regex re_no_spaces{"[^ &=]"};
    return ae::sequences::seq_id_t{
        designation
        | ranges::views::remove_if(to_remove)
        | ranges::views::replace('?', 'x')
        | ranges::views::replace_if(to_replace_with_slash, '/') // usually in passages
        | ranges::views::replace_if(to_replace_with_underscore, '_')
        | ranges::views::adjacent_remove_if([](char left, char right) { return left == '_' && right == '_'; })
        | ranges::to<std::string>()
                };

} // acmacs::seqdb::v3::make_seq_id

ae::sequences::seq_id_t ae::sequences::SeqdbSeqRef::seq_id() const
{
    if (entry != nullptr && seq != nullptr) {
        return make_seq_id(
            fmt::format("{} {} {} {}", entry->name, seq->reassortants.empty() ? std::string{} : seq->reassortants.front(), seq->passages.empty() ? std::string{} : seq->passages.front(), seq->hash));
    }
    else
        return seq_id_t{"<NULL>"};

} // ae::sequences::SeqdbSeqRef::seq_id

// ----------------------------------------------------------------------
