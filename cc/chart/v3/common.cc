#include <unordered_set>

#include "utils/log.hh"
#include "chart/v3/common.hh"
#include "chart/v3/chart.hh"

// ----------------------------------------------------------------------

ae::chart::v3::common_antigens_sera_t::common_antigens_sera_t(const Chart& primary, const Chart& secondary, antigens_sera_match_level_t match_level)
    : antigens_{primary.antigens(), secondary.antigens(), match_level}, sera_{primary.sera(), secondary.sera(), match_level}
{

} // ae::chart::v3::common_antigens_sera_t::common_antigens_sera_t

// ----------------------------------------------------------------------

ae::chart::v3::common_antigens_sera_t::common_antigens_sera_t(const Chart& primary)
    : antigens_{primary.antigens()}, sera_{primary.sera()}
{

} // ae::chart::v3::common_antigens_sera_t::common_antigens_sera_t

// ----------------------------------------------------------------------

std::vector<ae::chart::v3::common_antigens_sera_t::common_t> ae::chart::v3::common_antigens_sera_t::points() const
{
    const auto ags = antigens();
    const auto srs = sera();
    std::vector<common_t> points(ags.size() + srs.size());
    const auto output_last = std::transform(ags.begin(), ags.end(), points.begin(), [](const auto& src) { return std::pair{point_index{*src.first}, point_index{*src.second}}; });
    std::transform(srs.begin(), srs.end(), output_last, [ps = antigens_.size_primary(), ss = antigens_.size_secondary()](const auto& src) {
        return std::pair{ps + src.first, ss + src.second};
    });
    return points;

} // ae::chart::v3::common_antigens_sera_t::points

// ----------------------------------------------------------------------

template <typename AgSrs> ae::chart::v3::common_data_t<AgSrs>::common_data_t(const AgSrs& primary, const AgSrs& secondary, antigens_sera_match_level_t match_level)
    : primary_{primary}, secondary_{secondary}, min_number_{std::min(primary.size(), secondary.size())}
{
    build_match(match_level);
    sort_match();
    mark_match_use(match_level);
}

template ae::chart::v3::common_data_t<ae::chart::v3::Antigens>::common_data_t(const Antigens& primary, const Antigens& secondary, antigens_sera_match_level_t match_level);
template ae::chart::v3::common_data_t<ae::chart::v3::Sera>::common_data_t(const Sera& primary, const Sera& secondary, antigens_sera_match_level_t match_level);

// ----------------------------------------------------------------------

template <typename AgSrs> ae::chart::v3::common_data_t<AgSrs>::common_data_t(const AgSrs& primary)
    : primary_{primary}, secondary_{primary}, min_number_{primary.size()}
{
    for (const auto index : primary_.size())
        match_.push_back({.primary = index, .secondary = index, .score = score_t::full_match, .use = true});
}

template ae::chart::v3::common_data_t<ae::chart::v3::Antigens>::common_data_t(const Antigens& primary);
template ae::chart::v3::common_data_t<ae::chart::v3::Sera>::common_data_t(const Sera& primary);

// ----------------------------------------------------------------------

template <typename AgSrs> std::vector<typename ae::chart::v3::common_data_t<AgSrs>::common_t> ae::chart::v3::common_data_t<AgSrs>::common() const
{
    std::vector<common_t> result;
    for (const auto& mm : match_) {
        if (mm.use)
            result.emplace_back(mm.primary, mm.secondary);
    }
    return result;

} // ae::chart::v3::common_data_t<AgSrs>::common

template std::vector<ae::chart::v3::common_data_t<ae::chart::v3::Antigens>::common_t> ae::chart::v3::common_data_t<ae::chart::v3::Antigens>::common() const;
template std::vector<ae::chart::v3::common_data_t<ae::chart::v3::Sera>::common_t> ae::chart::v3::common_data_t<ae::chart::v3::Sera>::common() const;

// ----------------------------------------------------------------------

template <typename AgSrs> typename ae::chart::v3::common_data_t<AgSrs>::indexes_t ae::chart::v3::common_data_t<AgSrs>::primary() const
{
    indexes_t result;
    for (const auto& mm : match_) {
        if (mm.use)
            result.push_back(mm.primary);
    }
    std::sort(std::begin(result), std::end(result));
    return result;

} // ae::chart::v3::common_data_t<AgSrs>::primary

template ae::antigen_indexes ae::chart::v3::common_data_t<ae::chart::v3::Antigens>::primary() const;
template ae::serum_indexes ae::chart::v3::common_data_t<ae::chart::v3::Sera>::primary() const;

// ----------------------------------------------------------------------

template <typename AgSrs> void ae::chart::v3::common_data_t<AgSrs>::build_match(antigens_sera_match_level_t match_level)
{
    auto primary_sorted{index_range(primary_.size())};
    std::sort(primary_sorted.begin(), primary_sorted.end(), [this](auto i1, auto i2) { return compare_basic_designations(primary_[i1], primary_[i2]) == std::strong_ordering::less; });

    const auto primary_secondary_less = [this](index_t primary_index, const typename AgSrs::element_t& seco) -> bool {
        const auto& prim = primary_[primary_index];
        return compare_basic_designations(prim, seco) == std::strong_ordering::less;
    };
    const auto secondary_primary_less = [this](const typename AgSrs::element_t& seco, index_t primary_index) -> bool {
        const auto& prim = primary_[primary_index];
        return compare_basic_designations(seco, prim) == std::strong_ordering::less;
    };
    for (const auto secondary_index : secondary_.size()) {
        const auto& seco = secondary_[secondary_index];
        const auto first = std::lower_bound(primary_sorted.begin(), primary_sorted.end(), seco, primary_secondary_less);
        const auto last = std::upper_bound(primary_sorted.begin(), primary_sorted.end(), seco, secondary_primary_less);
        for (auto p_e = first; p_e != last; ++p_e) {
            if (const auto score = match(primary_[*p_e], seco, match_level); score != score_t::no_match) {
                match_.push_back({.primary = *p_e, .secondary = secondary_index, .score = score});
            }
        }
    }

} // ae::chart::v3::common_data_t<AgSrs>::build_match

template void ae::chart::v3::common_data_t<ae::chart::v3::Antigens>::build_match(antigens_sera_match_level_t match_level);
template void ae::chart::v3::common_data_t<ae::chart::v3::Sera>::build_match(antigens_sera_match_level_t match_level);

// ----------------------------------------------------------------------

template <typename AgSrs> void ae::chart::v3::common_data_t<AgSrs>::sort_match()
{
    auto order = [](const auto& a, const auto& b) {
                     if (a.score != b.score)
                         return a.score > b.score;
                     else if (a.primary != b.primary)
                         return a.primary < b.primary;
                     return a.secondary < b.secondary;
                 };
    std::sort(match_.begin(), match_.end(), order);

} // ae::chart::v3::common_data_t<AgSrs>::sort_match

template void ae::chart::v3::common_data_t<ae::chart::v3::Antigens>::sort_match();
template void ae::chart::v3::common_data_t<ae::chart::v3::Sera>::sort_match();

// ----------------------------------------------------------------------

template <typename AgSrs> void ae::chart::v3::common_data_t<AgSrs>::mark_match_use(antigens_sera_match_level_t match_level)
{
    std::unordered_set<index_t, index_hash_for_unordered_map, std::equal_to<>> primary_used, secondary_used;
    auto use_entry = [this, &primary_used, &secondary_used](auto& match_entry) {
        if (primary_used.find(match_entry.primary) == primary_used.end() && secondary_used.find(match_entry.secondary) == secondary_used.end()) {
            match_entry.use = true;
            primary_used.insert(match_entry.primary);
            secondary_used.insert(match_entry.secondary);
            ++this->number_of_common_;
        }
    };

    const size_t automatic_level_threshold = std::max(3UL, *min_number_ / 10);
    score_t previous_score{score_t::no_match};
    bool stop = false;
    for (auto& match_entry : match_) {
        switch (match_level) {
            case antigens_sera_match_level_t::strict:
                if (match_entry.score == score_t::full_match)
                    use_entry(match_entry);
                else
                    stop = true;
                break;
            case antigens_sera_match_level_t::relaxed:
                if (match_entry.score >= score_t::egg)
                    use_entry(match_entry);
                else
                    stop = true;
                break;
            case antigens_sera_match_level_t::ignored:
                if (match_entry.score >= score_t::passage_serum_id_ignored)
                    use_entry(match_entry);
                else
                    stop = true;
                break;
            case antigens_sera_match_level_t::automatic:
                if (previous_score == match_entry.score || number_of_common_ < automatic_level_threshold) {
                    use_entry(match_entry);
                    previous_score = match_entry.score;
                }
                else
                    stop = true;
                break;
        }
        if (stop)
            break;
    }

} // ae::chart::v3::common_data_t<AgSrs>::mark_match_use

template void ae::chart::v3::common_data_t<ae::chart::v3::Antigens>::mark_match_use(antigens_sera_match_level_t match_level);
template void ae::chart::v3::common_data_t<ae::chart::v3::Sera>::mark_match_use(antigens_sera_match_level_t match_level);

// ----------------------------------------------------------------------

template <typename AgSrs>
typename ae::chart::v3::common_data_t<AgSrs>::score_t ae::chart::v3::common_data_t<AgSrs>::match(const AgSr& prim, const AgSr& seco, antigens_sera_match_level_t match_level) const
{
    // using namespace std::string_view_literals;
    const auto match_report = [](bool equals, std::string_view report) -> std::string_view {
        if (equals)
            return {};
        else
            return report;
    };

    const auto name_neq = match_report(prim.name() == seco.name(), "name"),                      //
        reassortant_neq = match_report(prim.reassortant() == seco.reassortant(), "reassortant"), //
        annotations_neq = match_report(prim.annotations() == seco.annotations(), "annotations")  //,
        // primary_distict = match_report(!prim.annotations().distinct(), "primary-distinct"),      //
        // secondary_distict = match_report(!seco.annotations().distinct(), "secondary-distinct") //
        ;

    if (name_neq.empty() && reassortant_neq.empty() && annotations_neq.empty() && !prim.annotations().distinct() && !seco.annotations().distinct()) {
        // AD_LOG(acmacs::log::common, "{} \"{}\" == \"{}\"", primary.ag_sr(), primary.full_name(), secondary.full_name());
        switch (match_level) {
            case antigens_sera_match_level_t::ignored:
                return score_t::passage_serum_id_ignored;
            case antigens_sera_match_level_t::strict:
            case antigens_sera_match_level_t::relaxed:
            case antigens_sera_match_level_t::automatic:
                return match_not_ignored(prim, seco);
        }
    }

    // AD_DEBUG("{} \"{}\" != \"{}\": {} {} {}", prim.ag_sr, prim.designation(), seco.designation(), name_neq, reassortant_neq, annotations_neq);
    return score_t::no_match;
}

template ae::chart::v3::common_data_t<ae::chart::v3::Antigens>::score_t ae::chart::v3::common_data_t<ae::chart::v3::Antigens>::match(const AgSr& prim, const AgSr& seco, antigens_sera_match_level_t match_level) const;
template ae::chart::v3::common_data_t<ae::chart::v3::Sera>::score_t ae::chart::v3::common_data_t<ae::chart::v3::Sera>::match(const AgSr& prim, const AgSr& seco, antigens_sera_match_level_t match_level) const;

// ----------------------------------------------------------------------

template <typename AgSrs> typename ae::chart::v3::common_data_t<AgSrs>::score_t ae::chart::v3::common_data_t<AgSrs>::match_not_ignored(const AgSr& prim, const AgSr& seco) const
{
    auto result = score_t::passage_serum_id_ignored;
    if constexpr (std::is_same_v<AgSrs, Antigens>) { // AG
        if (prim.passage().empty() || seco.passage().empty()) {
            if (prim.passage() == seco.passage() && !prim.reassortant().empty() && !seco.reassortant().empty()) // reassortant assumes egg passage
                result = score_t::egg;
        }
        else if (prim.passage() == seco.passage())
            result = score_t::full_match;
        else if (prim.passage().without_date() == seco.passage().without_date())
            result = score_t::without_date;
        else if (prim.passage().is_egg() == seco.passage().is_egg())
            result = score_t::egg;
    }
    else { // SR
        if (prim.serum_id() == seco.serum_id() && !prim.serum_id().empty())
            result = score_t::full_match;
        else if (!prim.passage().empty() && !seco.passage().empty() && prim.passage().is_egg() == seco.passage().is_egg())
            result = score_t::egg;
    }
    return result;
}

template ae::chart::v3::common_data_t<ae::chart::v3::Antigens>::score_t ae::chart::v3::common_data_t<ae::chart::v3::Antigens>::match_not_ignored(const AgSr& prim, const AgSr& seco) const;
template ae::chart::v3::common_data_t<ae::chart::v3::Sera>::score_t ae::chart::v3::common_data_t<ae::chart::v3::Sera>::match_not_ignored(const AgSr& prim, const AgSr& seco) const;

// ----------------------------------------------------------------------

template <typename AgSrs> std::optional<typename ae::chart::v3::common_data_t<AgSrs>::index_t> ae::chart::v3::common_data_t<AgSrs>::primary_by_secondary(index_t secondary_no) const
{
    if (const auto found = std::find_if(match_.begin(), match_.end(), [secondary_no](const auto& match) { return match.use && match.secondary == secondary_no; }); found != match_.end())
        return found->primary;
    else
        return std::nullopt;

} // ae::chart::v3::common_data_t<AgSrs>::primary_by_secondary

template std::optional<ae::antigen_index> ae::chart::v3::common_data_t<ae::chart::v3::Antigens>::primary_by_secondary(ae::antigen_index secondary_no) const;
template std::optional<ae::serum_index> ae::chart::v3::common_data_t<ae::chart::v3::Sera>::primary_by_secondary(ae::serum_index secondary_no) const;

// ----------------------------------------------------------------------

template <typename AgSrs> std::optional<typename ae::chart::v3::common_data_t<AgSrs>::index_t> ae::chart::v3::common_data_t<AgSrs>::secondary_by_primary(index_t primary_no) const
{
    if (const auto found = std::find_if(match_.begin(), match_.end(), [primary_no](const auto& match) { return match.use && match.primary == primary_no; }); found != match_.end())
        return found->secondary;
    else
        return std::nullopt;

} // ae::chart::v3::common_data_t<AgSrs>::secondary_by_primary

template std::optional<ae::antigen_index> ae::chart::v3::common_data_t<ae::chart::v3::Antigens>::secondary_by_primary(ae::antigen_index primary_no) const;
template std::optional<ae::serum_index> ae::chart::v3::common_data_t<ae::chart::v3::Sera>::secondary_by_primary(ae::serum_index primary_no) const;

// ----------------------------------------------------------------------

template <typename AgSrs> size_t ae::chart::v3::common_data_t<AgSrs>::primary_name_max_size() const
{
    return std::accumulate(primary_.begin(), primary_.end(), 0ul, [](size_t mx, const auto& ag_sr) { return std::max(mx, ag_sr.designation_size()); });

} // ae::chart::v3::common_data_t<AgSrs>::primary_name_max_size

template size_t ae::chart::v3::common_data_t<ae::chart::v3::Antigens>::primary_name_max_size() const;
template size_t ae::chart::v3::common_data_t<ae::chart::v3::Sera>::primary_name_max_size() const;

// ----------------------------------------------------------------------

template <typename AgSrs> std::string ae::chart::v3::common_data_t<AgSrs>::report(size_t indent) const
{
    using namespace std::string_view_literals;
    std::string_view prefix, ignored_key;
    if constexpr (std::is_same_v<AgSrs, Antigens>) {
        prefix = "antigens"sv;
        ignored_key = "no-passage"sv;
    }
    else {
        ignored_key = "no-serum-id"sv;
        prefix = "sera"sv;
    }
    const std::array score_names{"no-match"sv, ignored_key, "egg"sv, "no-date"sv, "full"sv};
    const auto score_names_max = std::accumulate(std::begin(score_names), std::end(score_names), 0ul, [](size_t mx, std::string_view nam) { return std::max(mx, nam.size()); });

    fmt::memory_buffer output;
    if (number_of_common_) {
        const size_t primary_name_size = primary_name_max_size();
        const auto num_dgt = number_of_decimal_digits(std::max(*primary_.size(), *secondary_.size()));
        fmt::format_to(std::back_inserter(output), "{:{}s}common {}: {} (total primary:{} secondary:{})\n", "", indent, prefix, number_of_common_, primary_.size(), secondary_.size());
        const auto common = used();
        for (const auto& cmn : common) {
            fmt::format_to(std::back_inserter(output), "{:{}c}{:<{}s} {:{}d} {:<{}s} | {:{}d} {}\n", ' ', indent, score_names[static_cast<size_t>(cmn.score)], score_names_max, cmn.primary, num_dgt,
                           primary_[cmn.primary].designation(), primary_name_size, cmn.secondary, num_dgt, secondary_[cmn.secondary].designation());
        }

        // if (acmacs::log::is_enabled(acmacs::log::common)) {
        //     fmt::format_to(std::back_inserter(output), ">>>> [common] {:{}}common in primary {}: {}\n", "", indent, prefix,
        //                    ranges::views::transform(common, [](const auto& cmn) { return cmn.primary; }) | ranges::to_vector);
        //     fmt::format_to(std::back_inserter(output), ">>>> [common] {:{}}common in secondary {}: {}\n", "", indent, prefix,
        //                    ranges::views::transform(common, [](const auto& cmn) { return cmn.secondary; }) | ranges::to_vector);
        //     const auto [unique_in_primary, unique_in_secondary] = find_unique(common);
        //     fmt::format_to(std::back_inserter(output), ">>>> [common] {:{}}unique in primary {}: {}\n", "", indent, prefix, unique_in_primary);
        //     fmt::format_to(std::back_inserter(output), ">>>> [common] {:{}}unique in secondary {}: {}\n", "", indent, prefix, unique_in_secondary);
        // }
    }
    else {
        fmt::format_to(std::back_inserter(output), "{:{}}no common {}\n", ' ', indent, prefix);
    }
    return fmt::to_string(output);

} // ae::chart::v3::common_data_t<AgSrs>::report

template std::string ae::chart::v3::common_data_t<ae::chart::v3::Antigens>::report(size_t indent) const;
template std::string ae::chart::v3::common_data_t<ae::chart::v3::Sera>::report(size_t indent) const;

// ----------------------------------------------------------------------

std::string ae::chart::v3::common_antigens_sera_t::report(size_t indent) const
{
    return fmt::format("{}\n{}", antigens_.report(indent), sera_.report(indent));

} // ae::chart::v3::common_antigens_sera_t::report

// ----------------------------------------------------------------------


// ======================================================================
// ======================================================================
// ======================================================================


// using namespace ae::chart::v3;

#if 0
// ----------------------------------------------------------------------

enum class score_t : size_t { no_match = 0, passage_serum_id_ignored = 1, egg = 2, without_date = 3, full_match = 4 };

template <> struct fmt::formatter<score_t> : fmt::formatter<acmacs::fmt_helper::default_formatter> {
    auto format(const score_t& value, format_context& ctx) const
    {
        switch (value) {
            case score_t::no_match:
                format_to(ctx.out(), "no-match");
                break;
            case score_t::passage_serum_id_ignored:
                format_to(ctx.out(), "passage-serum-id-mismatch");
                break;
            case score_t::egg:
                format_to(ctx.out(), "egg");
                break;
            case score_t::without_date:
                format_to(ctx.out(), "without-date");
                break;
            case score_t::full_match:
                format_to(ctx.out(), "full");
                break;
        }
        return ctx.out();
    }
};

// ----------------------------------------------------------------------

class CommonAntigensSera::Impl
{
 public:
    Impl(const Chart& primary, const Chart& secondary, match_level_t match_level);
    Impl(const Chart& primary, const Chart& secondary, common::antigen_selector_t antigen_entry_extractor, common::serum_selector_t serum_entry_extractor, match_level_t match_level);
    Impl(const Chart& primary);

    struct MatchEntry
    {
        size_t primary_index;
        size_t secondary_index;
        score_t score;
        bool use{false};

    }; // class MatchEntry

    template <typename AgSrEntry> class ChartData
    {
     public:
        ChartData(const Chart& primary, const Chart& secondary, match_level_t match_level);
        template <typename Selector> ChartData(const Chart& primary, const Chart& secondary, Selector selector, match_level_t match_level);
        ChartData(const Chart& primary);
        std::string report(size_t indent, std::string_view prefix, std::string_view ignored_key) const;
        std::string report_unique(size_t indent, std::string_view prefix) const;
        std::vector<CommonAntigensSera::common_t> common() const;
        std::optional<size_t> primary_by_secondary(size_t secondary_no) const;
        std::optional<size_t> secondary_by_primary(size_t primary_no) const;
        void clear() { number_of_common_ = 0; match_.clear(); }

        void keep_only(const PointIndexList& indexes)
        {
            number_of_common_ = 0;
            for (auto& en : match_) {
                if (en.use) {
                    if (!indexes.contains(en.primary_index))
                        en.use = false;
                    else
                        ++number_of_common_;
                }
            }
        }

        std::vector<AgSrEntry> primary_;
        std::vector<AgSrEntry> secondary_;
        std::vector<MatchEntry> match_;
        size_t number_of_common_ = 0;
        const size_t primary_base_, secondary_base_; // 0 for antigens, number_of_antigens for sera
        const size_t min_number_ = 0;   // for match_level_t::automatic threshold

     private:
        template <typename AgSr> static void make(std::vector<AgSrEntry>& target, const AgSr& source)
            {
                for (size_t index = 0; index < target.size(); ++index) {
                    target[index] = AgSrEntry(index, *source[index]);
                }
            }

        template <typename AgSr, typename Selector> static void make(std::vector<AgSrEntry>& target, Selector selector, const AgSr& source)
            {
                for (size_t index = 0; index < target.size(); ++index) {
                    target[index] = selector(index, source[index]);
                }
            }

        void match(match_level_t match_level);
        score_t match(const AgSrEntry& primary, const AgSrEntry& secondary, match_level_t match_level) const;
        score_t match_not_ignored(const AgSrEntry& primary, const AgSrEntry& secondary) const;
        int num_digits() const;
        size_t primary_name_max_size() const;
        const AgSrEntry& find_primary(size_t index) const;
        std::vector<MatchEntry> find_common() const;
        std::pair<std::vector<size_t>, std::vector<size_t>> find_unique(const std::vector<MatchEntry>& common) const;

    }; // class ChartData<AgSrEntry>

    ChartData<common::AntigenEntry> antigens_;
    ChartData<common::SerumEntry> sera_;

}; // class CommonAntigensSera::Impl

// ----------------------------------------------------------------------

// template <> CommonAntigensSera::Impl::ChartData<common::AntigenEntry>::ChartData(const Chart& primary, const Chart& secondary, match_level_t match_level)
//     : primary_(primary.number_of_antigens()), secondary_(secondary.number_of_antigens()),
//       primary_base_{0}, secondary_base_{0},
//       min_number_{std::min(primary_.size(), secondary_.size())}
// {
//     make(primary_, *primary.antigens());
//     std::sort(primary_.begin(), primary_.end());
//     make(secondary_, *secondary.antigens());
//     match(match_level);

// } // CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::AntigenEntry>::ChartData

// ----------------------------------------------------------------------

template <> template <> CommonAntigensSera::Impl::ChartData<common::AntigenEntry>::ChartData(const Chart& primary, const Chart& secondary, common::antigen_selector_t selector, match_level_t match_level)
    : primary_(primary.number_of_antigens()), secondary_(secondary.number_of_antigens()),
      primary_base_{0}, secondary_base_{0},
      min_number_{std::min(primary_.size(), secondary_.size())}
{
    make(primary_, selector, *primary.antigens());
    std::sort(primary_.begin(), primary_.end());
    make(secondary_, selector, *secondary.antigens());
    match(match_level);

} // CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::AntigenEntry>::ChartData

// ----------------------------------------------------------------------

template <> CommonAntigensSera::Impl::ChartData<common::AntigenEntry>::ChartData(const Chart& primary)
    : primary_(primary.number_of_antigens()), secondary_(primary.number_of_antigens()), match_(primary.number_of_antigens()), primary_base_{0}, secondary_base_{0}
{
    make(primary_, *primary.antigens());
    std::sort(primary_.begin(), primary_.end());
    make(secondary_, *primary.antigens());
    for (const auto antigen_no : range_from_0_to(match_.size())) {
        match_[antigen_no].primary_index = match_[antigen_no].secondary_index = antigen_no;
        match_[antigen_no].score = score_t::full_match;
        match_[antigen_no].use = true;
    }
    number_of_common_ = match_.size();

} // CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::AntigenEntry>::ChartData

// ----------------------------------------------------------------------

// template <> CommonAntigensSera::Impl::ChartData<common::SerumEntry>::ChartData(const Chart& primary, const Chart& secondary, match_level_t match_level)
//     : primary_(primary.number_of_sera()), secondary_(secondary.number_of_sera()),
//       primary_base_{primary.number_of_antigens()}, secondary_base_{secondary.number_of_antigens()},
//       min_number_{std::min(primary_.size(), secondary_.size())}
// {
//     make(primary_, *primary.sera());
//     std::sort(primary_.begin(), primary_.end());
//     make(secondary_, *secondary.sera());
//     match(match_level);

// } // CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::SerumEntry>::ChartData

// ----------------------------------------------------------------------

template <> template <> CommonAntigensSera::Impl::ChartData<common::SerumEntry>::ChartData(const Chart& primary, const Chart& secondary, common::serum_selector_t selector, match_level_t match_level)
    : primary_(primary.number_of_sera()), secondary_(secondary.number_of_sera()),
      primary_base_{primary.number_of_antigens()}, secondary_base_{secondary.number_of_antigens()},
      min_number_{std::min(primary_.size(), secondary_.size())}
{
    make(primary_, selector, *primary.sera());
    std::sort(primary_.begin(), primary_.end());
    make(secondary_, selector, *secondary.sera());
    match(match_level);

} // CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::SerumEntry>::ChartData

// ----------------------------------------------------------------------

template <> CommonAntigensSera::Impl::ChartData<common::SerumEntry>::ChartData(const Chart& primary)
    : primary_(primary.number_of_sera()), secondary_(primary.number_of_sera()),
      match_(primary.number_of_sera()), primary_base_{primary.number_of_antigens()}, secondary_base_{primary.number_of_antigens()}
{
    make(primary_, *primary.sera());
    std::sort(primary_.begin(), primary_.end());
    make(secondary_, *primary.sera());
    for (const auto serum_no : range_from_0_to(match_.size())) {
        match_[serum_no].primary_index = match_[serum_no].secondary_index = serum_no;
        match_[serum_no].score = score_t::full_match;
        match_[serum_no].use = true;
    }
    number_of_common_ = match_.size();

} // CommonAntigensSera::Impl::ChartData<CommonAntigensSera::Impl::SerumEntry>::ChartData

// ----------------------------------------------------------------------

// template <typename AgSrEntry> void CommonAntigensSera::Impl::ChartData<AgSrEntry>::match(CommonAntigensSera::match_level_t match_level)
// {
//     for (const auto& secondary: secondary_) {
//         const auto [first, last] = std::equal_range(primary_.begin(), primary_.end(), secondary, AgSrEntry::less);
//         for (auto p_e = first; p_e != last; ++p_e) {
//             if (const auto score = match(*p_e, secondary, match_level); score != score_t::no_match) {
//                 AD_LOG(acmacs::log::common, "{} {:25s} -- \"{}\" <> \"{}\"", p_e->ag_sr(), fmt::format("{}", score), p_e->full_name(), secondary.full_name());
//                 // match_.emplace_back(p_e->index, secondary.index, score);
//                 match_.push_back({p_e->index, secondary.index, score});
//             }
//         }
//     }
//     auto order = [](const auto& a, const auto& b) {
//                      if (a.score != b.score)
//                          return a.score > b.score;
//                      else if (a.primary_index != b.primary_index)
//                          return a.primary_index < b.primary_index;
//                      return a.secondary_index < b.secondary_index;
//                  };
//     std::sort(match_.begin(), match_.end(), order);

//     std::set<size_t> primary_used, secondary_used;
//     auto use_entry = [this,&primary_used,&secondary_used](auto& match_entry) {
//         if (primary_used.find(match_entry.primary_index) == primary_used.end() && secondary_used.find(match_entry.secondary_index) == secondary_used.end()) {
//             match_entry.use = true;
//             primary_used.insert(match_entry.primary_index);
//             secondary_used.insert(match_entry.secondary_index);
//             ++this->number_of_common_;
//         }
//     };

//     const size_t automatic_level_threshold = std::max(3UL, min_number_ / 10);
//     score_t previous_score{score_t::no_match};
//     bool stop = false;
//     for (auto& match_entry: match_) {
//         switch (match_level) {
//           case match_level_t::strict:
//               if (match_entry.score == score_t::full_match)
//                   use_entry(match_entry);
//               else
//                   stop = true;
//               break;
//           case match_level_t::relaxed:
//               if (match_entry.score >= score_t::egg)
//                   use_entry(match_entry);
//               else
//                   stop = true;
//               break;
//           case match_level_t::ignored:
//               if (match_entry.score >= score_t::passage_serum_id_ignored)
//                   use_entry(match_entry);
//               else
//                   stop = true;
//               break;
//           case match_level_t::automatic:
//               if (previous_score == match_entry.score || number_of_common_ < automatic_level_threshold) {
//                   use_entry(match_entry);
//                   previous_score = match_entry.score;
//               }
//               else
//                   stop = true;
//               break;
//         }
//         if (stop)
//             break;
//     }

// } // CommonAntigensSera::Impl::ChartData::match

// // ----------------------------------------------------------------------

// template <typename AgSrEntry>
// score_t CommonAntigensSera::Impl::ChartData<AgSrEntry>::match(const AgSrEntry& primary, const AgSrEntry& secondary, CommonAntigensSera::match_level_t match_level) const
// {
//     using namespace std::string_view_literals;
//     const auto match_report = [](bool equals, std::string_view report) -> std::string_view {
//         if (equals)
//             return {};
//         else
//             return report;
//     };

//     const auto name_neq = match_report(primary.name == secondary.name, "name"), reassortant_neq = match_report(primary.reassortant == secondary.reassortant, "reassortant"),
//                annotations_neq = match_report(primary.annotations == secondary.annotations, "annotations"), primary_distict = match_report(!primary.annotations.distinct(), "primary-distinct"),
//                secondary_distict = match_report(!secondary.annotations.distinct(), "secondary-distinct");

//     if (name_neq.empty() && reassortant_neq.empty() && annotations_neq.empty() && primary_distict.empty() && secondary_distict.empty()) {
//         // AD_LOG(acmacs::log::common, "{} \"{}\" == \"{}\"", primary.ag_sr(), primary.full_name(), secondary.full_name());
//         switch (match_level) {
//             case match_level_t::ignored:
//                 return score_t::passage_serum_id_ignored;
//             case match_level_t::strict:
//             case match_level_t::relaxed:
//             case match_level_t::automatic:
//                 return match_not_ignored(primary, secondary);
//         }
//     }

//     AD_LOG(acmacs::log::common, "{} \"{} {} {}\" != \"{} {} {}\": {} {} {} {} {}", primary.ag_sr(), //
//            primary.name, primary.reassortant, primary.annotations,                                  //
//            secondary.name, secondary.reassortant, secondary.annotations,                            //
//            name_neq, reassortant_neq, annotations_neq, primary_distict, secondary_distict);
//     return score_t::no_match;

// } // CommonAntigensSera::Impl::ChartData::match

// // ----------------------------------------------------------------------

// template <> score_t CommonAntigensSera::Impl::ChartData<common::AntigenEntry>::match_not_ignored(const common::AntigenEntry& primary, const common::AntigenEntry& secondary) const
// {
//     auto result = score_t::passage_serum_id_ignored;
//     if (primary.passage.empty() || secondary.passage.empty()) {
//         if (primary.passage == secondary.passage && !primary.reassortant.empty() && !secondary.reassortant.empty()) // reassortant assumes egg passage
//             result = score_t::egg;
//     }
//     else if (primary.passage == secondary.passage)
//         result = score_t::full_match;
//     else if (primary.passage.without_date() == secondary.passage.without_date())
//         result = score_t::without_date;
//     else if (primary.passage.is_egg() == secondary.passage.is_egg())
//         result = score_t::egg;
//     return result;

// } // CommonAntigensSera::Impl::ChartData<AntigenEntry>::match_not_ignored

// // ----------------------------------------------------------------------

// template <> score_t CommonAntigensSera::Impl::ChartData<common::SerumEntry>::match_not_ignored(const common::SerumEntry& primary, const common::SerumEntry& secondary) const
// {
//     auto result = score_t::passage_serum_id_ignored;
//     if (primary.serum_id == secondary.serum_id && !primary.serum_id.empty())
//         result = score_t::full_match;
//     else if (!primary.passage.empty() && !secondary.passage.empty() && primary.passage.is_egg() == secondary.passage.is_egg())
//         result = score_t::egg;
//     return result;

// } // CommonAntigensSera::Impl::ChartData<AntigenEntry>::match_not_ignored

// ----------------------------------------------------------------------

template <typename AgSrEntry> int CommonAntigensSera::Impl::ChartData<AgSrEntry>::num_digits() const
{
    return static_cast<int>(std::log10(std::max(primary_.size(), secondary_.size()))) + 1;
}

// ----------------------------------------------------------------------

template <typename AgSrEntry> size_t CommonAntigensSera::Impl::ChartData<AgSrEntry>::primary_name_max_size() const
{
    return ranges::accumulate(primary_, 0ul, [](size_t mx, const auto& ag_sr) { return std::max(mx, ag_sr.full_name_length()); });
}

// ----------------------------------------------------------------------

template <typename AgSrEntry> const AgSrEntry& CommonAntigensSera::Impl::ChartData<AgSrEntry>::find_primary(size_t index) const
{
    return *std::find_if(primary_.begin(), primary_.end(), [index](const auto& element) { return element.index == index; });
}

// ----------------------------------------------------------------------

template <typename AgSrEntry> std::vector<CommonAntigensSera::Impl::MatchEntry> CommonAntigensSera::Impl::ChartData<AgSrEntry>::find_common() const
{
    return ranges::views::filter(match_, [](const auto& match) { return match.use; }) | ranges::to_vector;
}

// ----------------------------------------------------------------------

template <typename AgSrEntry> std::pair<std::vector<size_t>, std::vector<size_t>> CommonAntigensSera::Impl::ChartData<AgSrEntry>::find_unique(const std::vector<MatchEntry>& common) const
{
    std::vector<size_t> unique_in_primary(primary_.size() - common.size()), unique_in_secondary(secondary_.size() - common.size());
    std::copy_if(acmacs::index_iterator(0UL), acmacs::index_iterator(primary_.size()), unique_in_primary.begin(),
                 [&common](size_t index) { return std::find_if(std::begin(common), std::end(common), [index](const auto& cmn) { return cmn.primary_index == index; }) == std::end(common); });
    std::copy_if(acmacs::index_iterator(0UL), acmacs::index_iterator(secondary_.size()), unique_in_secondary.begin(),
                 [&common](size_t index) { return std::find_if(std::begin(common), std::end(common), [index](const auto& cmn) { return cmn.secondary_index == index; }) == std::end(common); });
    return {unique_in_primary, unique_in_secondary};
}

// ----------------------------------------------------------------------

// template <typename AgSrEntry> std::string CommonAntigensSera::Impl::ChartData<AgSrEntry>::report(size_t indent, std::string_view prefix, std::string_view ignored_key) const
// {
//     using namespace std::string_view_literals;
//     const std::array score_names{"no-match"sv, ignored_key, "egg"sv, "no-date"sv, "full"sv};
//     const auto score_names_max = std::accumulate(std::begin(score_names), std::end(score_names), 0ul, [](size_t mx, std::string_view nam) { return std::max(mx, nam.size()); });

//     fmt::memory_buffer output;
//     if (number_of_common_) {
//         const size_t primary_name_size = primary_name_max_size();
//         const auto num_dgt = num_digits();
//         fmt::format_to_mb(output, "{:{}s}common {}: {} (total primary: {} secondary: {})\n", "", indent, prefix, number_of_common_, primary_.size(), secondary_.size());
//         const auto common = find_common();
//         for (const auto& cmn : common) {
//             fmt::format_to_mb(output, "{:{}c}{:<{}s} {:{}d} {:<{}s} | {:{}d} {}\n", ' ', indent, score_names[static_cast<size_t>(cmn.score)], score_names_max, cmn.primary_index, num_dgt,
//                            find_primary(cmn.primary_index).full_name(), primary_name_size, cmn.secondary_index, num_dgt, secondary_[cmn.secondary_index].full_name());
//         }
//         if (acmacs::log::is_enabled(acmacs::log::common)) {
//             fmt::format_to_mb(output, ">>>> [common] {:{}}common in primary {}: {}\n", "", indent, prefix,
//                            ranges::views::transform(common, [](const auto& cmn) { return cmn.primary_index; }) | ranges::to_vector);
//             fmt::format_to_mb(output, ">>>> [common] {:{}}common in secondary {}: {}\n", "", indent, prefix,
//                            ranges::views::transform(common, [](const auto& cmn) { return cmn.secondary_index; }) | ranges::to_vector);
//             const auto [unique_in_primary, unique_in_secondary] = find_unique(common);
//             fmt::format_to_mb(output, ">>>> [common] {:{}}unique in primary {}: {}\n", "", indent, prefix, unique_in_primary);
//             fmt::format_to_mb(output, ">>>> [common] {:{}}unique in secondary {}: {}\n", "", indent, prefix, unique_in_secondary);
//         }
//     }
//     else {
//         fmt::format_to_mb(output, "{:{}}no common {}\n", ' ', indent, prefix);
//     }
//     return fmt::to_string(output);

// } // CommonAntigensSera::Impl::ChartData<AgSrEntry>::report

// ----------------------------------------------------------------------

template <typename AgSrEntry> std::string CommonAntigensSera::Impl::ChartData<AgSrEntry>::report_unique(size_t indent, std::string_view prefix) const
{
    const auto num_dgt = num_digits();
    const size_t primary_name_size = primary_name_max_size();
    const auto [unique_in_primary, unique_in_secondary] = find_unique(find_common());
    fmt::memory_buffer output;
    fmt::format_to_mb(output, "{:{}}unique primary {}: {} (total: {}) secondary: {} (total: {})\n", "", indent, prefix, unique_in_primary.size(), primary_.size(), unique_in_secondary.size(),
                   secondary_.size());
    for (const auto no : range_from_0_to(std::max(unique_in_primary.size(), unique_in_secondary.size()))) {
        if (no < unique_in_primary.size())
            fmt::format_to_mb(output, "{:{}s}{:{}d} {:<{}s} |", "", indent, unique_in_primary[no], num_dgt, find_primary(unique_in_primary[no]).full_name(), primary_name_size);
        else
            fmt::format_to_mb(output, "{:{}s}{:{}c} {:{}c} |", "", indent, ' ', num_dgt, ' ', primary_name_size);
        if (no < unique_in_secondary.size())
            fmt::format_to_mb(output, " {:{}d} {}", unique_in_secondary[no], num_dgt, secondary_[unique_in_secondary[no]].full_name());
        fmt::format_to_mb(output, "\n");
    }
    return fmt::to_string(output);

} // CommonAntigensSera::Impl::ChartData<AgSrEntry>::report_unique

// ----------------------------------------------------------------------

template <typename AgSrEntry> inline std::vector<CommonAntigensSera::common_t> CommonAntigensSera::Impl::ChartData<AgSrEntry>::common() const
{
    std::vector<CommonAntigensSera::common_t> result;
    for (const auto& m: match_) {
        if (m.use)
            result.emplace_back(m.primary_index, m.secondary_index);
    }
    return result;

} // CommonAntigensSera::Impl::ChartData<AgSrEntry>::common

// ----------------------------------------------------------------------

// #pragma GCC diagnostic push
// #if __GNUC__ == 8
// #pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
// // g++-8.2 issues strange warning about return {} statement in two template functions below
// #endif

// template <typename AgSrEntry> inline std::optional<size_t> CommonAntigensSera::Impl::ChartData<AgSrEntry>::primary_by_secondary(size_t secondary_no) const
// {
//     if (const auto found = std::find_if(match_.begin(), match_.end(), [secondary_no](const auto& match) { return match.use && match.secondary_index == secondary_no; }); found != match_.end())
//         return found->primary_index;
//     else
//         return std::nullopt;

// } // CommonAntigensSera::Impl::ChartData<AgSrEntry>::primary_by_secondary

// // ----------------------------------------------------------------------

// template <typename AgSrEntry> inline std::optional<size_t> CommonAntigensSera::Impl::ChartData<AgSrEntry>::secondary_by_primary(size_t primary_no) const
// {
//     if (const auto found = std::find_if(match_.begin(), match_.end(), [primary_no](const auto& match) { return match.use && match.primary_index == primary_no; }); found != match_.end())
//         return found->secondary_index;
//     else
//         return std::nullopt;

// } // CommonAntigensSera::Impl::ChartData<AgSrEntry>::secondary_by_primary

// #pragma GCC diagnostic pop

// ----------------------------------------------------------------------

inline CommonAntigensSera::Impl::Impl(const Chart& primary, const Chart& secondary, match_level_t match_level)
    : antigens_(primary, secondary, match_level), sera_(primary, secondary, match_level)
{
}

// ----------------------------------------------------------------------

inline CommonAntigensSera::Impl::Impl(const Chart& primary, const Chart& secondary, common::antigen_selector_t antigen_entry_extractor, common::serum_selector_t serum_entry_extractor, match_level_t match_level)
    : antigens_(primary, secondary, antigen_entry_extractor, match_level), sera_(primary, secondary, serum_entry_extractor, match_level)
{
}

// ----------------------------------------------------------------------

inline CommonAntigensSera::Impl::Impl(const Chart& primary)
    : antigens_(primary), sera_(primary)
{
}

// ----------------------------------------------------------------------

CommonAntigensSera::CommonAntigensSera(const Chart& primary, const Chart& secondary, match_level_t match_level)
    : impl_(&primary == &secondary ? std::make_unique<Impl>(primary) : std::make_unique<Impl>(primary, secondary, match_level))
{
}

// ----------------------------------------------------------------------

CommonAntigensSera::CommonAntigensSera(const Chart& primary, const Chart& secondary, common::antigen_selector_t antigen_entry_extractor, common::serum_selector_t serum_entry_extractor, match_level_t match_level)
    : impl_(std::make_unique<Impl>(primary, secondary, antigen_entry_extractor, serum_entry_extractor, match_level))
{

} // CommonAntigensSera::CommonAntigensSera

// ----------------------------------------------------------------------

// for procrustes between projections of the same chart
CommonAntigensSera::CommonAntigensSera(const Chart& primary)
    : impl_(std::make_unique<Impl>(primary))
{

} // CommonAntigensSera::CommonAntigensSera

// ----------------------------------------------------------------------

CommonAntigensSera::CommonAntigensSera(CommonAntigensSera&&) = default;
CommonAntigensSera& CommonAntigensSera::operator=(CommonAntigensSera&&) = default;

// must be here to allow std::unique_ptr<Impl> with incomplete Impl in .hh
CommonAntigensSera::~CommonAntigensSera()
{
}

// ----------------------------------------------------------------------

std::string CommonAntigensSera::report(size_t indent) const
{
    using namespace std::string_view_literals;
    return fmt::format("{}\n{}", impl_->antigens_.report(indent, "antigens"sv, "no-passage"sv), impl_->sera_.report(indent, "sera"sv, "no-serum-id"sv));

} // CommonAntigensSera::report

// ----------------------------------------------------------------------

std::string CommonAntigensSera::report_unique(size_t indent) const
{
    using namespace std::string_view_literals;
    return fmt::format("{}\n{}", impl_->antigens_.report_unique(indent, "antigens"sv), impl_->sera_.report_unique(indent, "sera"sv));

} // CommonAntigensSera::report_unique

// ----------------------------------------------------------------------

CommonAntigensSera::operator bool() const
{
    return impl_->antigens_.number_of_common_ > 0 || impl_->sera_.number_of_common_ > 0;

} // bool

// ----------------------------------------------------------------------

void CommonAntigensSera::keep_only(const PointIndexList& antigens, const PointIndexList& sera)
{
    if (antigens.empty())
        impl_->antigens_.clear();
    else
        impl_->antigens_.keep_only(antigens);
    if (sera.empty())
        impl_->sera_.clear();
    else
        impl_->sera_.keep_only(sera);

} // CommonAntigensSera::keep_only

// ----------------------------------------------------------------------

void CommonAntigensSera::antigens_only()   // remove sera from common lists
{
    impl_->sera_.clear();

} // CommonAntigensSera::antigens_only

// ----------------------------------------------------------------------

void CommonAntigensSera::sera_only()   // remove antigens from common lists
{
    impl_->antigens_.clear();

} // CommonAntigensSera::sera_only

// ----------------------------------------------------------------------

size_t CommonAntigensSera::common_antigens() const
{
    return impl_->antigens_.number_of_common_;

} // CommonAntigensSera::common_antigens

// ----------------------------------------------------------------------

size_t CommonAntigensSera::common_sera() const
{
    return impl_->sera_.number_of_common_;

} // CommonAntigensSera::common_sera

// ----------------------------------------------------------------------

std::vector<CommonAntigensSera::common_t> CommonAntigensSera::antigens() const
{
    return impl_->antigens_.common();

} // CommonAntigensSera::antigens

// ----------------------------------------------------------------------

std::vector<CommonAntigensSera::common_t> CommonAntigensSera::sera() const
{
    return impl_->sera_.common();

} // CommonAntigensSera::sera

// ----------------------------------------------------------------------

Indexes CommonAntigensSera::common_primary_antigens() const
{
    const auto ags{antigens()};
    Indexes result(ags.size());
    std::transform(std::begin(ags), std::end(ags), std::begin(result), [](const auto& en) { return en.primary; });
    std::sort(std::begin(result), std::end(result));
    return result;

} // CommonAntigensSera::common_primary_antigens

// ----------------------------------------------------------------------

// returns serum indexes (NOT point indexes)!
Indexes CommonAntigensSera::common_primary_sera() const
{
    const auto srs{sera()};
    Indexes result(srs.size());
    std::transform(std::begin(srs), std::end(srs), std::begin(result), [](const auto& en) { return en.primary; });
    std::sort(std::begin(result), std::end(result));
    return result;

} // CommonAntigensSera::common_primary_sera

// ----------------------------------------------------------------------

std::vector<CommonAntigensSera::common_t> CommonAntigensSera::sera_as_point_indexes() const
{
    auto result = impl_->sera_.common();
    std::transform(result.begin(), result.end(), result.begin(), [primary_base_=impl_->sera_.primary_base_,secondary_base=impl_->sera_.secondary_base_](const auto& entry) -> common_t { return {entry.primary + primary_base_, entry.secondary + secondary_base}; });
    return result;

} // CommonAntigensSera::sera_as_point_indexes

// ----------------------------------------------------------------------

std::vector<CommonAntigensSera::common_t> CommonAntigensSera::points(subset a_subset) const
{
    switch (a_subset) {
      case subset::all:
          return points();
      case subset::antigens:
          return impl_->antigens_.common();
      case subset::sera:
          return sera_as_point_indexes();
    }
    return {};

} // CommonAntigensSera::points

// ----------------------------------------------------------------------

std::vector<CommonAntigensSera::common_t> CommonAntigensSera::points() const
{
    auto result = impl_->antigens_.common();
    for (auto [primary, secondary]: impl_->sera_.common())
        result.emplace_back(primary + impl_->sera_.primary_base_, secondary + impl_->sera_.secondary_base_);
    return result;

} // CommonAntigensSera::points

// ----------------------------------------------------------------------

std::vector<CommonAntigensSera::common_t> CommonAntigensSera::points_for_primary_antigens(const Indexes& antigen_indexes) const
{
    auto result = impl_->antigens_.common();
    result.erase(std::remove_if(std::begin(result), std::end(result), [&antigen_indexes](const auto& entry) { return !antigen_indexes.contains(entry.primary); }), std::end(result));
    return result;

} // CommonAntigensSera::points_for_primary_antigens

// ----------------------------------------------------------------------

std::vector<CommonAntigensSera::common_t> CommonAntigensSera::points_for_primary_sera(const Indexes& serum_indexes) const
{
    auto result = impl_->sera_.common();
    result.erase(std::remove_if(std::begin(result), std::end(result), [&serum_indexes](const auto& entry) { return !serum_indexes.contains(entry.primary); }), std::end(result));
    std::transform(result.begin(), result.end(), result.begin(), [primary_base_=impl_->sera_.primary_base_,secondary_base=impl_->sera_.secondary_base_](const auto& entry) -> common_t { return {entry.primary + primary_base_, entry.secondary + secondary_base}; });
    return result;

} // CommonAntigensSera::points_for_primary_sera

// ----------------------------------------------------------------------

std::optional<size_t> CommonAntigensSera::antigen_primary_by_secondary(size_t secondary_no) const
{
    return impl_->antigens_.primary_by_secondary(secondary_no);

} // CommonAntigensSera::antigen_primary_by_secondary

// ----------------------------------------------------------------------

std::optional<size_t> CommonAntigensSera::antigen_secondary_by_primary(size_t primary_no) const
{
    return impl_->antigens_.secondary_by_primary(primary_no);

} // CommonAntigensSera::antigen_secondary_by_primary

// ----------------------------------------------------------------------

std::optional<size_t> CommonAntigensSera::serum_primary_by_secondary(size_t secondary_no) const
{
    return impl_->sera_.primary_by_secondary(secondary_no);

} // CommonAntigensSera::serum_primary_by_secondary

// ----------------------------------------------------------------------

std::optional<size_t> CommonAntigensSera::serum_secondary_by_primary(size_t primary_no) const
{
    return impl_->sera_.secondary_by_primary(primary_no);

} // CommonAntigensSera::serum_secondary_by_primary

// ----------------------------------------------------------------------

CommonAntigensSera::match_level_t CommonAntigensSera::match_level(std::string_view source)
{
    auto match_level{match_level_t::automatic};
    if (!source.empty()) {
        switch (source[0]) {
            case 's':
                match_level = match_level_t::strict;
                break;
            case 'r':
                match_level = match_level_t::relaxed;
                break;
            case 'i':
                match_level = match_level_t::ignored;
                break;
            case 'a':
                match_level = match_level_t::automatic;
                break;
            default:
                AD_WARNING("unrecognized match level: \"{}\", automatic assumed", source);
                break;
        }
    }
    return match_level;

} // CommonAntigensSera::match_level

// ----------------------------------------------------------------------
#endif
