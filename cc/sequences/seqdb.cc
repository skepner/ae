#include <memory>
#include <cstdlib>
#include <variant>
#include <regex>

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

ae::sequences::SeqdbSeqRefList ae::sequences::SeqdbSeqRefList_empty{};

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

ae::sequences::Seqdb& ae::sequences::seqdb_for_subtype(const virus::type_subtype_t& subtype, verbose verb)
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

bool ae::sequences::Seqdb::add(const RawSequence& raw_sequence)
{
    bool added { false };
    const auto found_by_hash = find_by_hash(raw_sequence.hash_nuc);
    bool keep_sequence = !found_by_hash || found_by_hash.entry->name == raw_sequence.name;
    if (keep_sequence) {
        hash_index_.try_emplace(raw_sequence.hash_nuc, raw_sequence.name);
    }
    else if (raw_sequence.sequence.nuc.size() != found_by_hash.seq->nuc.size()) {
        fmt::print(">> [seqdb {}] hash {} conflict for \"{}\" (nucs: {}) and \"{}\" (nucs: {}) (short sequence will be thrown away)\n", subtype_, raw_sequence.hash_nuc, raw_sequence.name,
                   raw_sequence.sequence.nuc.size(), found_by_hash.entry->name, found_by_hash.seq->nuc.size());
        if (raw_sequence.issues.any()) {
            // do not add raw_sequence
            return added;
        }
        else {
            remove(found_by_hash); // also removes corresponding hash_index_ entry
            remove(raw_sequence.hash_nuc); // other sequence may refer to that hash, remove them too
            hash_index_.try_emplace(raw_sequence.hash_nuc, raw_sequence.name);
            keep_sequence = true;
        }
    }
    const auto found = std::lower_bound(std::begin(entries_), std::end(entries_), raw_sequence.name, [](const auto& entry, std::string_view nam) { return entry.name < nam; });
    if (found != std::end(entries_) && found->name == raw_sequence.name) {
        added = found->update(raw_sequence, keep_sequence);
        modified_ |= added;
    }
    else {
        const auto it_added = entries_.emplace(found, raw_sequence);
        if (!keep_sequence)
            it_added->find_by_hash(raw_sequence.hash_nuc)->dont_keep_sequence();
        modified_ = true;
        added = true;
    }
    return added;

} // ae::sequences::Seqdb::add

// ----------------------------------------------------------------------

void ae::sequences::Seqdb::remove(const SeqdbSeqRef& ref)
{
    if (const auto seq = std::find_if(std::begin(ref.entry->seqs), std::end(ref.entry->seqs), [&ref](const auto& eseq) { return &eseq == ref.seq; }); seq != std::end(ref.entry->seqs)) {
        if (const auto found_hash = hash_index_.find(seq->hash); found_hash != hash_index_.end() && found_hash->second == ref.entry->name)
            hash_index_.erase(found_hash);
        const_cast<SeqdbEntry*>(ref.entry)->seqs.erase(seq);
        if (ref.entry->seqs.empty()) {
            if (const auto entry_to_remove = std::find_if(std::begin(entries_), std::end(entries_), [&ref](const auto& entry) { return &entry == ref.entry; }); entry_to_remove != std::end(entries_))
                entries_.erase(entry_to_remove);
            else
                fmt::print(">> cannot remove entry from seqdb (not found) \"{}\"\n", ref.entry->name);
            // !!! invalidate seq_id_index_
        }
    }
    else
        fmt::print(">> cannor remove seq by SeqdbSeqRef: no ref.seq in ref.entry \"{}\"\n", ref.entry->name);

} // ae::sequences::Seqdb::remove

// ----------------------------------------------------------------------

void ae::sequences::Seqdb::remove(const hash_t& hash) // remove all seqs (and perhaps some entries) with that hash
{
    const auto do_remove = [&hash, this]() -> bool {
        for (const auto& entry : entries_) {
            for (const auto& seq : entry.seqs) {
                if (seq.hash == hash) {
                    remove(SeqdbSeqRef{.entry = &entry, .seq = &seq});
                    return true; // iteration over seq is now invalid (entry is perhaps removed) and also it makes no sense in looking in it further
                    // iteration over entries_ is invalid as well, has to start again
                }
            }
        }
        return false;
    };

    for (bool removed = true; removed;)
        removed = do_remove();

} // ae::sequences::Seqdb::remove

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

const ae::sequences::Seqdb::seq_id_index_t& ae::sequences::Seqdb::seq_id_index() const
{
    if (seq_id_index_.empty()) {
        for (const auto& entry : entries_) {
            for (const auto& seq : entry.seqs) {
                SeqdbSeqRef ref{.entry = &entry, .seq = &seq};
                seq_id_index_.try_emplace(ref.seq_id(), std::move(ref));
            }
        }
    }
    return seq_id_index_;

} // ae::sequences::Seqdb::seq_id_index

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
        if (host.empty()) {
            host = raw_sequence.host;
            updated = true;
        }
        else if (host != raw_sequence.host)
            fmt::print(">> SeqdbEntry::update \"{}\": conflicting host old:{} new:{}\n", name, host, raw_sequence.host);
    }
    if (raw_sequence.lineage) {
        if (!lineage) {
            lineage = raw_sequence.lineage;
            updated = true;
        }
        else if (lineage != raw_sequence.lineage)
            fmt::print(">> SeqdbEntry::update \"{}\": conflicting lineage old:{} new:{}\n", name, lineage, raw_sequence.lineage);
    }
    if (!raw_sequence.continent.empty()) {
        if (continent.empty()) {
            continent = raw_sequence.continent;
            updated = true;
        }
        else if (continent != raw_sequence.continent)
            fmt::print(">> SeqdbEntry::update \"{}\": conflicting continent old:{} new:{}\n", name, continent, raw_sequence.continent);
    }
    if (!raw_sequence.country.empty()) {
        if (country.empty()) {
            country = raw_sequence.country;
            updated = true;
        }
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

} // ae::sequences::SeqdbEntry::update

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
            if (!raw_sequence.hash_nuc.empty()) {
                hash = raw_sequence.hash_nuc;
                updated = true;
            }
            else
                fmt::print(">> no hash \"{}\" {}\n", raw_sequence.name, raw_sequence.sequence.nuc);
        }
        else if (hash != raw_sequence.hash_nuc)
            throw Error{"SeqdbSeq::update keep_sequence={} raw_sequence.hash={} hash={} hash difference", keep_sequence, raw_sequence.hash_nuc, hash};
    }

    update_vec(annotations, raw_sequence.annotations);
    update_vec(reassortants, raw_sequence.reassortant);
    update_vec(passages, raw_sequence.passage);
    updated |= issues.update(raw_sequence.issues);
    if (!raw_sequence.lab.empty()) {
        if (auto lab = std::find_if(std::begin(lab_ids), std::end(lab_ids), [&raw_sequence](const auto& en) { return en.first == raw_sequence.lab; }); lab != std::end(lab_ids)) {
            update_vec(lab->second, raw_sequence.lab_id);
        }
        else {
            lab_ids.emplace_back(raw_sequence.lab, lab_ids_t{raw_sequence.lab_id});
            updated = true;
        }
    }

    update_vec(gisaid.accession_number, raw_sequence.accession_number);
    update_vec(gisaid.gisaid_dna_accession_no, raw_sequence.gisaid_dna_accession_no);
    update_vec(gisaid.gisaid_dna_insdc, raw_sequence.gisaid_dna_insdc);
    update_vec(gisaid.gisaid_identifier, raw_sequence.gisaid_identifier);
    // update_vec(gisaid.gisaid_last_modified, raw_sequence.gisaid_last_modified);
    // update_vec(gisaid.gisaid_submitter, raw_sequence.gisaid_submitter);
    // update_vec(gisaid.gisaid_originating_lab, raw_sequence.gisaid_originating_lab);

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

struct match_name_str
{
    match_name_str(std::string_view pat) : pattern{pat} {}
    bool operator()(const ae::sequences::SeqdbSeqRef& ref) const { return ref.entry->name.find(pattern) != std::string::npos; }
    std::string_view pattern;
};

struct match_name_re
{
    match_name_re(std::string_view pat) : pattern{pat.begin(), pat.end(), std::regex::icase | std::regex::ECMAScript | std::regex::optimize} {}
    bool operator()(const ae::sequences::SeqdbSeqRef& ref) const { return std::regex_search(ref.entry->name, pattern); }
    std::regex pattern;
};

struct match_seq_id_str
{
    match_seq_id_str(std::string_view pat) : pattern{pat} {}
    bool operator()(const ae::sequences::SeqdbSeqRef& ref) const { return ref.seq_id().get().find(pattern) != std::string::npos; }
    std::string_view pattern;
};

struct match_seq_id_re
{
    match_seq_id_re(std::string_view pat) : pattern{pat.begin(), pat.end(), std::regex::icase | std::regex::ECMAScript | std::regex::optimize} {}
    bool operator()(const ae::sequences::SeqdbSeqRef& ref) const { return std::regex_search(ref.seq_id().get(), pattern); }
    std::regex pattern;
};

using matcher_t = std::variant<match_name_str, match_name_re, match_seq_id_str, match_seq_id_re>;

inline matcher_t make_matcher(std::string_view pat)
{
    if (pat.empty())
        return match_name_str{pat};
    if (pat[0] == '~') {
        if (pat.find('_') == std::string_view::npos)
            return match_name_re{pat.substr(1)};
        else
            return match_seq_id_re{pat.substr(1)};
    }
    else {
        if (pat.find('_') == std::string_view::npos)
            return match_name_str{pat};
        else
            return match_seq_id_str{pat};
    }
}

inline std::vector<matcher_t> make_matchers(const std::vector<std::string>& names)
{
    std::vector<matcher_t> matchers;
    matchers.reserve(names.size());
    for (const auto& name : names) {
        if (!name.empty())
            matchers.push_back(make_matcher(name));
        else
            fmt::print(">> empty name to match seqdb entry against (ignored)\n");
    }
    return matchers;
}

struct matcher_match
{
    const ae::sequences::SeqdbSeqRef& ref;
    bool operator()(const matcher_t& matcher) const
    {
        return std::visit([this](auto&& mat) { return mat(ref); }, matcher);
    }
};

inline bool any_matcher_match(const std::vector<matcher_t>& matchers, const ae::sequences::SeqdbSeqRef& ref)
{
    return std::any_of(std::begin(matchers), std::end(matchers), matcher_match{ref});
}

// ----------------------------------------------------------------------

ae::sequences::SeqdbSelected& ae::sequences::SeqdbSelected::filter_name(const std::vector<std::string>& names)
{
    const auto matchers = make_matchers(names);
    erase_if(*this, [&matchers](const auto& ref) -> bool { return !any_matcher_match(matchers, ref); });
    return *this;

} // ae::sequences::SeqdbSelected::filter_name

// ----------------------------------------------------------------------

ae::sequences::SeqdbSelected& ae::sequences::SeqdbSelected::exclude_name(const std::vector<std::string>& names)
{
    const auto matchers = make_matchers(names);
    erase_if(*this, [&matchers](const auto& ref) -> bool { return any_matcher_match(matchers, ref); });
    return *this;

} // ae::sequences::SeqdbSelected::exclude_name

// ----------------------------------------------------------------------

ae::sequences::SeqdbSelected& ae::sequences::SeqdbSelected::include_name(const std::vector<std::string>& names, bool include_with_issue_too)
{
    auto refs_with_name = seqdb_.select_all();
    refs_with_name->filter_name(names);
    erase_if(*refs_with_name,
             [this](const auto& ref_with_name) -> bool { return std::any_of(std::begin(refs_), std::end(refs_), [&ref_with_name](const auto& ref) { return ref_with_name == ref; }); });
    if (!include_with_issue_too)
        erase_with_issues(*refs_with_name);
    std::copy(std::begin(refs_with_name->refs_), std::end(refs_with_name->refs_), std::back_inserter(refs_));
    return *this;

} // ae::sequences::SeqdbSelected::include_name

// ----------------------------------------------------------------------

ae::sequences::SeqdbSelected& ae::sequences::SeqdbSelected::sort(order ord)
{
    switch (ord) {
        case order::date_ascending:
            std::sort(std::begin(refs_), std::end(refs_), [](const auto& ref1, const auto& ref2) { return ref1.entry->date_less_than(*ref2.entry); });
            break;
        case order::date_descending:
            std::sort(std::begin(refs_), std::end(refs_), [](const auto& ref1, const auto& ref2) { return ref1.entry->date_more_than(*ref2.entry); });
            break;
        case order::name_ascending:
            std::sort(std::begin(refs_), std::end(refs_), [](const auto& ref1, const auto& ref2) { return ref1.seq_id() < ref2.seq_id(); });
            break;
        case order::name_descending:
            std::sort(std::begin(refs_), std::end(refs_), [](const auto& ref1, const auto& ref2) { return ref2.seq_id() < ref1.seq_id(); });
            break;
        case order::hash:
            std::sort(std::begin(refs_), std::end(refs_), [](const auto& ref1, const auto& ref2) {
                if (const auto cmp = ref1.seq->hash <=> ref2.seq->hash; cmp == std::strong_ordering::equal)
                    return ref2.entry->date() < ref1.entry->date();
                else
                    return cmp == std::strong_ordering::less;
            });
            break;
        case order::none:
            break;
    }
    return *this;

} // ae::sequences::SeqdbSelected::sort

// ----------------------------------------------------------------------

ae::sequences::SeqdbSelected& ae::sequences::SeqdbSelected::move_name_to_beginning(const std::vector<std::string>& names)
{
    if (refs_.size() > 1) {
        const auto matchers = make_matchers(names);
        auto first = refs_.begin();
        for (auto it = refs_.begin() + 1; it != refs_.end(); ++it) {
            if (any_matcher_match(matchers, *it)) {
                std::rotate(first, it, it + 1);
                ++first;
            }
        }
    }
    return *this;

} // ae::sequences::SeqdbSelected::move_name_to_beginning

// ----------------------------------------------------------------------
