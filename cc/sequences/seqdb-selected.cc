#include <regex>
#include <variant>
#include <map>

#include "utils/string.hh"
#include "utils/string-hash.hh"
#include "virus/passage.hh"
#include "sequences/seqdb-selected.hh"
#include "sequences/clades.hh"

// ======================================================================

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

// ======================================================================

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
    // erase_if(*this, [&matchers](const auto& ref) -> bool { const auto matched = any_matcher_match(matchers, ref); if (matched) fmt::print(">>>> excluded {}\n", ref.seq_id()); return matched; });
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

ae::sequences::SeqdbSelected& ae::sequences::SeqdbSelected::filter_name(std::string_view name, std::string_view reassortant, std::string_view passage)
{
    if (ae::string::startswith(name, fmt::format("{}/", seqdb_.subtype())))
        name.remove_prefix(seqdb_.subtype().size() + 1);
    erase_if(*this, [name](const auto& ref) -> bool { return ref.entry->name != name; });
    if (size() > 1) {
        const auto passage_parsed = virus::passage::parse(passage);
        const auto rank_passage = [passage_parsed](std::string_view seq_passage) -> size_t {
            const auto seq_passage_parsed = virus::passage::parse(seq_passage);
            // fmt::print(">>>> {}{} vs. {}{}\n", passage_parsed.last().name, passage_parsed.last().count, seq_passage_parsed.last().name, seq_passage_parsed.last().count);
            if (passage_parsed == seq_passage_parsed)
                return 0;
            else if (passage_parsed.empty() || seq_passage_parsed.empty())
                return 90;
            else if (passage_parsed.last() == seq_passage_parsed.last())
                return 10;
            else if (passage_parsed.last().name == seq_passage_parsed.last().name)
                return 20;
            else if (passage_parsed.egg() == seq_passage_parsed.egg())
                return 50;
            else
                return 90;
        };

        const auto rank = [reassortant, rank_passage](const SeqdbSeqRef& ref) -> size_t {
            size_t rnk{0};
            if ((!reassortant.empty() && std::find(std::begin(ref.seq->reassortants), std::end(ref.seq->reassortants), reassortant) == std::end(ref.seq->reassortants)) ||
                (reassortant.empty() && !ref.seq->reassortants.empty()))
                rnk += 100;
            size_t passage_rank{99};
            for (const auto& seq_passage : ref.seq->passages)
                passage_rank = std::min(passage_rank, rank_passage(seq_passage));
            rnk += passage_rank;
            return rnk;
        };

        std::unordered_map<seq_id_t, size_t, ae::string_hash_for_unordered_map, std::equal_to<>> ranks;
        std::transform(begin(), end(), std::inserter(ranks, ranks.end()), [rank](const auto& en) { return std::pair{en.seq_id(), rank(en)}; });
        std::sort(begin(), end(), [&ranks](const auto& en1, const auto& en2) { return ranks.find(en1.seq_id())->second < ranks.find(en2.seq_id())->second; });
    }
    return *this;

} // ae::sequences::SeqdbSelected::filter_name

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

ae::sequences::SeqdbSelected& ae::sequences::SeqdbSelected::find_clades(const Clades& clades)
{
    if (!empty()) {
        find_masters();
        if (const auto* entries = clades.get_entries(seqdb_.subtype(), refs_[0].entry->lineage); entries) {
            for (auto& ref : refs_)
                ref.clades = clades.clades(ref.aa(), ref.nuc(), *entries);
        }
    }
    return *this;

} // ae::sequences::SeqdbSelected::find_clades

// ----------------------------------------------------------------------

ae::sequences::SeqdbSelected& ae::sequences::SeqdbSelected::find_clades(std::string_view clades_json_file)
{
    if (!empty())
        find_clades(Clades{clades_json_file});
    return *this;

} // ae::sequences::SeqdbSelected::find_clades

// ----------------------------------------------------------------------

ae::sequences::SeqdbSelected& ae::sequences::SeqdbSelected::length_stat()
{
    std::map<size_t, size_t> aa_length_count, nuc_length_count;
    for (const auto& ref : refs_) {
        if (const auto [iter, added] = aa_length_count.try_emplace(ref.aa().size(), 1); !added)
            ++iter->second;
        if (const auto [iter, added] = nuc_length_count.try_emplace(ref.nuc().size(), 1); !added)
            ++iter->second;
    }
    fmt::print(">>> nuc size stat\n");
    for (const auto& [size, count]: nuc_length_count)
        fmt::print("  {:4d}  {:4d}\n", size, count);
    fmt::print(">>> aa size stat\n");
    for (const auto& [size, count]: aa_length_count)
        fmt::print("  {:4d}  {:4d}\n", size, count);

    return *this;

} // ae::sequences::SeqdbSelected::length_stat

// ----------------------------------------------------------------------

std::pair<size_t, size_t> ae::sequences::SeqdbSelected::max_length() const // returns max nuc and aa length
{
    size_t max_nuc{0}, max_aa{0};
    for (const auto& ref : refs_) {
        max_nuc = std::max(max_nuc, ref.nuc().size());
        max_aa = std::max(max_aa, ref.aa().size());
    }
    return {max_nuc, max_aa};

} // ae::sequences::SeqdbSelected::max_length

// ----------------------------------------------------------------------

ae::sequences::SeqdbSelected& ae::sequences::SeqdbSelected::filter_lab_id(std::string_view lab_id) // passed lab_id is "LAB#ID", e.g. "CDC#2007700886", it is stored in ace
{
    using namespace std::string_view_literals;
    if (const auto lab_and_id = string::split(lab_id, "#"sv); lab_and_id.size() == 2) {
        erase_if(*this, [&lab_and_id](const auto& ref) -> bool {
            for (const auto& [lab, ids] : ref.seq->lab_ids) {
                if (lab == lab_and_id[0]) {
                    for (const auto& id : ids) {
                        if (id == lab_and_id[1])
                            return false;
                    }
                }
            }
            return true;
        });
    }
    return *this;

} // ae::sequences::SeqdbSelected::filter_lab_id

// ----------------------------------------------------------------------
