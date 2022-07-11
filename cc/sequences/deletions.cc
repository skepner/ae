#include <numeric>

#include "utils/messages.hh"
#include "utils/log.hh"
#include "sequences/deletions.hh"
#include "sequences/master.hh"

// ======================================================================

constexpr const ssize_t common_threshold = 3; // assume the chunk is common after that number of consecutive common positions
constexpr const size_t max_deletions_insertions = 200; // give up if this number of deletions/insertions does not help
// constexpr double verify_threshold = 0.6;              // if number of common is less than this fraction of non-X in shortest of to_align and master sequences, verification fails

// ----------------------------------------------------------------------

struct deletions_insertions_t
{
    struct pos_num_t
    {
        ae::sequences::pos0_t pos;
        size_t num;
        auto operator<=>(const pos_num_t&) const = default;
    };

    using data_t = std::vector<pos_num_t>;
    data_t deletions{};
    data_t insertions{};

    bool empty() const { return deletions.empty() && insertions.empty(); }

    // returns {pos deleted, adjusted pos}
    std::pair<bool, ae::sequences::pos0_t> aa_apply_deletions(ae::sequences::pos0_t pos) const
    {
        using namespace ae::sequences;

        for (const auto& pos_num : deletions) {
            if (pos_num.pos <= pos) {
                if ((pos_num.pos + pos_num.num) > pos)
                    return {true, pos};
                else
                    pos -= pos_num.num;
            }
            else
                break;
        }
        return {false, pos};
    }

    // returns {pos deleted, adjusted pos}
    std::pair<bool, ae::sequences::pos0_t> nuc_apply_deletions(ae::sequences::pos0_t pos) const
    {
        const auto [pos_deleted, adjusted_pos_aa] = aa_apply_deletions(pos.nuc_to_aa());
        return {pos_deleted, ae::sequences::pos0_t{(*adjusted_pos_aa * 3) + pos.nuc_offset()}};
    }

    size_t number_of_deleted_positions() const
    {
        return std::accumulate(std::begin(deletions), std::end(deletions), 0ul, [](size_t sum, const auto& del) { return sum + del.num; });
    }

}; // struct deletions_insertions_t

template <> struct fmt::formatter<deletions_insertions_t::pos_num_t> : fmt::formatter<ae::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const deletions_insertions_t::pos_num_t& value, FormatCtx& ctx) const
    {
        return format_to(ctx.out(), "{}:{}", value.pos, value.num);
    }
};

template <> struct fmt::formatter<deletions_insertions_t> : fmt::formatter<ae::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const deletions_insertions_t& value, FormatCtx& ctx) const
    {
        if (!value.empty()) {
            if (!value.deletions.empty())
                format_to(ctx.out(), "del:{}", value.deletions);
            if (!value.insertions.empty())
                format_to(ctx.out(), "ins:{}", value.insertions);
            return ctx.out();
        }
        else
            return format_to(ctx.out(), "{}", "{}");
    }
};

// ----------------------------------------------------------------------

inline bool is_yamagata(const deletions_insertions_t& deletions)
{
    using namespace ae::sequences;
    using pos_num_t = deletions_insertions_t::pos_num_t;
    return deletions.deletions.size() == 1 && deletions.deletions[0] == pos_num_t{pos1_t{163}, 1};
}

inline bool is_victoria_2del(const deletions_insertions_t& deletions)
{
    using namespace ae::sequences;
    using pos_num_t = deletions_insertions_t::pos_num_t;
    return deletions.deletions.size() == 1 && deletions.deletions[0] == pos_num_t{pos1_t{162}, 2};
}

inline bool is_victoria_3del(const deletions_insertions_t& deletions)
{
    using namespace ae::sequences;
    using pos_num_t = deletions_insertions_t::pos_num_t;
    return deletions.deletions.size() == 1 && deletions.deletions[0] == pos_num_t{pos1_t{162}, 3};
}

// ----------------------------------------------------------------------

inline bool N_deletions_at(const deletions_insertions_t& deletions, size_t num_deletions, ae::sequences::pos1_t pos_min, ae::sequences::pos1_t pos_max)
{
    return !deletions.deletions.empty() && deletions.deletions.front().pos >= pos_min && deletions.deletions.front().pos <= pos_max && deletions.deletions.front().num == num_deletions;
}

inline bool N_deletions_at(const deletions_insertions_t& deletions, size_t num_deletions, ae::sequences::pos1_t pos)
{
    return !deletions.deletions.empty() && deletions.deletions.front().pos == pos && deletions.deletions.front().num == num_deletions;
}

inline bool N_insertions_at(const deletions_insertions_t& deletions, size_t num_insertions, ae::sequences::pos1_t pos)
{
    return !deletions.insertions.empty() && deletions.insertions.front().pos == pos && deletions.insertions.front().num == num_insertions;
}

// ----------------------------------------------------------------------

struct find_head_t
{
    size_t head{0};
    size_t common{0};
};

inline find_head_t find_common_head(std::string_view s1, std::string_view s2)
{
    // find the last part with common_f()==true that is not shorter than common_threshold
    // returns offset of the end of this part
    auto f1 = s1.begin(), common_start = s1.end(), last_common_end = s1.begin();
    size_t common = 0, common_at_last_common_end = 0, really_common_in_this_common_chunk = 0;
    const auto update_last_common_end = [&]() {
        if (common_start != s1.end() && really_common_in_this_common_chunk >= common_threshold) {
            last_common_end = f1;
            common_at_last_common_end = common;
        }
    };

    for (auto first2 = s2.begin(); f1 != s1.end() && first2 != s2.end(); ++f1, ++first2) {
        if (*f1 == *first2 || *f1 == 'X' || *first2 == 'X') {
            if (*f1 == *first2) {
                ++common;
                ++really_common_in_this_common_chunk;
            }
            if (common_start == s1.end())
                common_start = f1;
        }
        else {
            update_last_common_end();
            common_start = s1.end();
            really_common_in_this_common_chunk = 0;
        }
    }
    update_last_common_end();

    if (const auto head = static_cast<size_t>(last_common_end - s1.begin()); common_at_last_common_end * 3 > head) {
        // AD_DEBUG(dbg, "head: {}  common_at_last_common_end: {}", head, common_at_last_common_end);
        return {head, common_at_last_common_end};
    }
    else {
        // AD_DEBUG(dbg, "too few common in the head, try more deletions");
        return {0, 0}; // too few common in the head, try more deletions
    }
}

// ----------------------------------------------------------------------

struct deletions_insertions_at_start_t
{
    size_t deletions = 0;
    size_t insertions = 0;
    find_head_t head{};
};

inline deletions_insertions_at_start_t deletions_insertions_at_start(std::string_view master, std::string_view to_align)
{
    deletions_insertions_at_start_t result;
    for (size_t dels = 1; dels < max_deletions_insertions; ++dels) {
        if (dels < master.size()) {
            result.head = find_common_head(master.substr(dels), to_align);
            if (result.head.head > common_threshold) {
                result.deletions = dels;
                break;
            }
        }
        if (dels < to_align.size()) {
            result.head = find_common_head(master, to_align.substr(dels));
            if (result.head.head > common_threshold) {
                result.insertions = dels;
                break;
            }
        }
    }
    return result;
}

// ----------------------------------------------------------------------

constexpr const auto are_common = [](char a, char b) -> bool { return a == b && a != 'X' && a != '-'; };

inline size_t number_of_common(std::string_view s1, std::string_view s2)
{
    size_t common = 0;
    for (auto p1 = s1.begin(), p2 = s2.begin(); p1 != s1.end() && p2 != s2.end(); ++p1, ++p2) {
        if (are_common(*p1, *p2))
            ++common;
    }
    return common;
}

// ======================================================================

inline deletions_insertions_t find_deletions_insertions(const ae::sequences::RawSequence& sequence)
{
    using namespace ae::sequences;

    const auto& master = *master_sequence_for(sequence.type_subtype);
    const std::string_view master_aa{master.aa}, sequence_aa{sequence.sequence.aa};
    deletions_insertions_t deletions;
    const auto initial_head = find_common_head(master.aa, sequence.sequence.aa);

    size_t master_offset = initial_head.head, to_align_offset = initial_head.head;
    std::string_view master_tail = master_aa.substr(master_offset), to_align_tail = sequence_aa.substr(to_align_offset);

    const auto update_both = [](size_t dels, size_t head, std::string_view& s1, std::string_view& s2, size_t& offset1, size_t& offset2) {
        s1.remove_prefix(head + dels);
        s2.remove_prefix(head);
        offset1 += head + dels;
        offset2 += head;
    };

    // size_t common = initial_head.common;
    while (!master_tail.empty() && !to_align_tail.empty()) {
        const auto tail_deletions = deletions_insertions_at_start(master_tail, to_align_tail);
        number_of_common(master_tail.substr(tail_deletions.deletions, tail_deletions.head.head), to_align_tail.substr(tail_deletions.insertions, tail_deletions.head.head));
        if (tail_deletions.head.head == 0) {                        // < local::common_threshold) {
            // common += number_of_common(master_tail, to_align_tail); // to avoid common diff warning below in case tail contains common aas
            break;                                                  // tails are different, insertions/deletions do not help
        }
        if (tail_deletions.deletions) {
            deletions.deletions.push_back({pos0_t{to_align_offset}, tail_deletions.deletions});
            update_both(tail_deletions.deletions, tail_deletions.head.head, master_tail, to_align_tail, master_offset, to_align_offset);
        }
        else { // insertions or nothing (in some cases)
            if (tail_deletions.insertions) {
                deletions.insertions.push_back({pos0_t{master_offset}, tail_deletions.insertions});
            }
            update_both(tail_deletions.insertions, tail_deletions.head.head, to_align_tail, master_tail, to_align_offset, master_offset);
        }
        // common += tail_deletions.head.common;
    }

    if (deletions.deletions.size() == 1 && deletions.insertions.empty()) {
        // fmt::print("{} {} {} pos-get={} num==1:{} p0-162=>{} {}\n", deletions.deletions[0], deletions.deletions[0].pos == pos1_t{162}, deletions.deletions[0].pos, deletions.deletions[0].pos.get(),
        //            deletions.deletions[0].num == 1, pos0_t{162},  sequence_aa.substr(159, 5));
        if ((deletions.deletions[0] == deletions_insertions_t::pos_num_t{pos1_t{162}, 1} && sequence.sequence.aa.substr(pos1_t{160}, 2) == "VP")     //
            || (deletions.deletions[0] == deletions_insertions_t::pos_num_t{pos1_t{164}, 1} && sequence.sequence.aa.substr(pos1_t{160}, 3) == "VPK") //
            || (deletions.deletions[0] == deletions_insertions_t::pos_num_t{pos1_t{159}, 1} && sequence.sequence.aa.substr(pos1_t{158}, 3) == "WVS") //
        ) {
            deletions.deletions[0].pos = pos1_t{163};
        }
        else if (N_deletions_at(deletions, 3, pos1_t{162}, pos1_t{164})) {
            deletions.deletions[0].pos = pos1_t{162}; // victoria 3del
        }
        else if (N_deletions_at(deletions, 3, pos1_t{166}) && sequence.sequence.aa.substr(pos1_t{160}, 7) == "VPKNXXA") {
            // 3del, misleading XX
            deletions.deletions[0] = deletions_insertions_t::pos_num_t{pos1_t{162}, 3};
        }
        else if (N_deletions_at(deletions, 2, pos1_t{164}) && sequence.sequence.aa.substr(pos1_t{160}, 6) == "VPKNNK") {
            // B/GANSU_ANDING/1194/2021 etc.
            // Vic 3del with insertion at 166: "VPKNNK" -> "VP---KNK"
            deletions.deletions[0] = deletions_insertions_t::pos_num_t{pos1_t{162}, 3};
            deletions.insertions.push_back(deletions_insertions_t::pos_num_t{pos1_t{167}, 1}); // can be 166 and 167, Sarah reports that the labs use 167
            // AD_INFO("B/Vic 3del with 167N {}\n{}---{}\n{}---------{}", sequence.name, sequence.sequence.aa.substr(pos1_t{160}, 2), sequence.sequence.aa.substr(pos1_t{162}, 10), sequence.sequence.nuc.substr(pos0_t{159 * 3}, 6), sequence.sequence.nuc.substr(pos0_t{161 * 3}, 30));
        }
    }

    return deletions;

    // // // sanity check (remove)
    // // if (const auto nc = local::number_of_common(master, to_align, deletions); nc != common)
    // //     fmt::print(stderr, "common diff: {} vs. number_of_common:{} {}\n{}\n{}\n", common, nc, format(deletions), acmacs::seqdb::v3::scan::format(deletions.insertions, master, '.'),
    // acmacs::seqdb::v3::scan::format(deletions.deletions, sequence_aa, '.'));

    // // verify
    // const auto get_num_non_x = [](std::string_view seq) { return seq.size() - static_cast<size_t>(std::count(std::begin(seq), std::end(seq), 'X')); };
    // const auto num_common_threshold = static_cast<double>(master_aa.size() < sequence_aa.size() ? get_num_non_x(master) : get_num_non_x(sequence_aa)) * local::verify_threshold;
    // if (static_cast<double>(common) < num_common_threshold) {
    //     throw local::not_verified(fmt::format("common:{} vs size:{} num_common_threshold:{:.2f}\n{}\n{}\n{}\n{}\n",
    //                                           common, sequence_aa.size(), num_common_threshold, master, sequence_aa,
    //                                           acmacs::seqdb::v3::scan::format_aa(deletions.insertions, master, '.'), acmacs::seqdb::v3::scan::format_aa(deletions.deletions, sequence_aa, '.')));
    // }

} // ae::sequences::find_deletions_insertions

// ----------------------------------------------------------------------

void ae::sequences::find_deletions_insertions_set_lineage(RawSequence& sequence, Messages& messages)
{
    const auto apply_deletions = [&sequence](const deletions_insertions_t& deletions) {
        std::string_view source_aa{sequence.sequence.aa}, source_nuc{sequence.sequence.nuc};
        fmt::memory_buffer aa, nuc;
        pos0_t pos{0};
        for (const auto& en : deletions.deletions) {
            fmt::format_to(std::back_inserter(aa), "{}{}", source_aa.substr(*pos, *(en.pos - pos)), std::string(en.num, '-'));
            fmt::format_to(std::back_inserter(nuc), "{}{}", source_nuc.substr(*pos * 3, *(en.pos - pos) * 3), std::string(en.num * 3, '-'));
            pos = en.pos;
        }
        fmt::format_to(std::back_inserter(aa), "{}", source_aa.substr(*pos));
        fmt::format_to(std::back_inserter(nuc), "{}", source_nuc.substr(*pos * 3));
        sequence.sequence.set(fmt::to_string(aa), fmt::to_string(nuc));

        size_t base_pos_aa{0};
        for (const auto& en : deletions.insertions) {
            sequence.aa_insertions.push_back(insertion_t{en.pos, sequence.sequence.aa.get().substr(en.pos.get0() + base_pos_aa, en.num)});
            sequence.sequence.aa.get().erase(en.pos.get0() + base_pos_aa, en.num);
            base_pos_aa += en.num;
            sequence.nuc_insertions.push_back(insertion_t{en.pos.aa_to_nuc(), sequence.sequence.nuc.get().substr(en.pos.aa_to_nuc().get0() + base_pos_aa * 3, en.num * 3)});
            sequence.sequence.nuc.get().erase(en.pos.aa_to_nuc().get0() + base_pos_aa * 3, en.num * 3);
        }
    };

    const auto apply_lineage = [&sequence, &messages](std::string_view lineage) {
        if (!sequence.lineage)
            sequence.lineage = lineage;
        else if (sequence.lineage != lineage_t{lineage}) {
            if (sequence.sequence.aa.size() > 500)
                messages.add(Message::lineage_mismatch, sequence.type_subtype, fmt::format("detected: {} provided: {}", lineage, sequence.lineage),
                             fmt::format("lineage difference \"{}\" provided:{} detected:{}\nS:  {}", sequence.name, sequence.lineage, lineage, sequence.sequence.aa));
            sequence.lineage = lineage;
        }
    };

    const auto apply_deletions_lineage = [apply_deletions, apply_lineage](const deletions_insertions_t& deletions, std::string_view lineage) {
        apply_deletions(deletions);
        apply_lineage(lineage);
    };

    using namespace std::string_view_literals;
    if (sequence.type_subtype == "B"sv) {
        // if (const auto deletions = find_deletions_insertions(sequence);
        if (const auto deletions = find_deletions_insertions(sequence); is_yamagata(deletions)) {
            apply_deletions_lineage(deletions, "YAMAGATA");
        }
        else if (is_victoria_2del(deletions) || is_victoria_3del(deletions)) {
            apply_deletions_lineage(deletions, "VICTORIA");
        }
        else if (N_deletions_at(deletions, 2, pos1_t{169})) {
            // 12 sequences from TAIWAN 2010 have deletions 169:2
        }
        else if (N_deletions_at(deletions, 2, pos1_t{163}) /* && sequence.year() <= 2013 */) {
            apply_lineage("YAMAGATA");
        }
        else if (N_deletions_at(deletions, 1, pos1_t{159}) && sequence.sequence.aa.substr(pos1_t{158}, 4) == "WVIP") { // B/Lee/40
            // ignore
            apply_deletions(deletions);
        }
        else if (N_deletions_at(deletions, 1, pos1_t{1})) {
            // do not apply, ignore
        }
        else if (!deletions.empty()) {
            apply_deletions(deletions);
            if (sequence.sequence.aa.size() > 500)
                messages.add(Message::unrecognized_deletions, sequence.type_subtype, fmt::format("\"{}\" {}", sequence.name, sequence.lineage),
                             fmt::format("{}\n                S:  {}\n                M:  {}", deletions, sequence.sequence.aa, master_sequence_for(ae::virus::type_subtype_t{"B"})->aa));
        }
    }

    if (sequence.type_subtype == "A(H1N1)"sv || sequence.type_subtype == "A(H1)"sv) {
        // expected deletion at 130 in INDONESIA/2389/2006 and number of others seasonal H1
        auto deletions = find_deletions_insertions(sequence);
        if (N_deletions_at(deletions, 1, pos1_t{124}, pos1_t{129}) || (N_deletions_at(deletions, 1, pos1_t{156}, pos1_t{190}) && sequence.sequence.aa.substr(pos1_t{130}, 5) == "GVSAS")) { // misplacement, according to Colin it's 130
            deletions.deletions[0].pos = pos1_t{130};
            apply_deletions(deletions);
        }
        else if (!deletions.empty()) { // && sequence.host.empty())
            apply_deletions(deletions);
        }
        if (sequence.sequence.aa.substr(pos1_t{130}, 5) == "GVSAS") { // must be deletion at 130
            // AD_DEBUG("130:GVSAS {:10s} {:50s} {}", sequence.date, sequence.name, deletions);
            deletions_insertions_t deletions2;
            deletions2.deletions.push_back({pos1_t{130}, 1});
            apply_deletions(deletions2);

            auto deletions3 = find_deletions_insertions(sequence);
            if (N_insertions_at(deletions3, 1, pos1_t{283}))
                deletions3.insertions[0].pos = pos1_t{286};
            if (!deletions3.empty())
                apply_deletions(deletions3);
            // AD_DEBUG("del2      {:61s} {}", std::string_view{}, deletions3);
            // AD_DEBUG("{}", sequence.sequence.aa);
        }

        // else if (sequence.date < "2008" && sequence.host.empty())
        //     AD_DEBUG("H1 no-del {:10s} {:50s}              {}", sequence.date, sequence.name, sequence.sequence.aa.substr(pos1_t{124}, 20));
    }

} // ae::sequences::find_deletions_insertions_set_lineage

// ----------------------------------------------------------------------
