#include "utils/string.hh"
#include "utils/regex.hh"
#include "utils/named-type.hh"
#include "virus/name-parse.hh"
#include "chart/v2/chart-modify.hh"

// ----------------------------------------------------------------------

template <typename AgSr> static std::string name_full_without_passage(const AgSr& ag_sr)
{
    return ae::string::join(" ", ag_sr.name(), fmt::format("{: }", ag_sr.annotations()), ag_sr.reassortant());
}

inline static std::string name_full(const ae::chart::v2::Antigen& ag)
{
    return ae::string::join(" ", name_full_without_passage(ag), ag.passage());
}

inline static std::string name_full(const ae::chart::v2::Serum& sr)
{
    return ae::string::join(" ", name_full_without_passage(sr), sr.serum_id());
}

template <typename AgSr> static std::string name_full_passage(const AgSr& ag_sr)
{
    return ae::string::join(" ", name_full_without_passage(ag_sr), ag_sr.passage());
}

template <typename AgSr> static std::string location_abbreviated(const AgSr& ag_sr)
{
    return ae::locdb::get().abbreviation(ae::virus::name::location(ag_sr.name()));
}

template <typename AgSr> static std::string year4(const AgSr& ag_sr)
{
    return ae::virus::name::year(ag_sr.name());
}

template <typename AgSr> static std::string year2(const AgSr& ag_sr)
{
    if (const auto yr = year4(ag_sr); yr.size() == 4)
        return yr.substr(2);
    else
        return yr;
}

template <typename AgSr> static std::string name_abbreviated(const AgSr& ag_sr)
{
    return ae::string::join("-", ae::string::join("/", location_abbreviated(ag_sr), ae::virus::name::isolation(ag_sr.name()), year2(ag_sr)), ag_sr.reassortant());
}

template <typename AgSr> static std::string fields(const AgSr& ag_sr)
{
    fmt::memory_buffer output;
    fmt::format_to(std::back_inserter(output), "{}", ag_sr.name());
    if (const auto value = fmt::format("{: }", ag_sr.annotations()); !value.empty())
        fmt::format_to(std::back_inserter(output), " annotations=\"{}\"", value);
    if (const auto value = ag_sr.reassortant(); !value.empty())
        fmt::format_to(std::back_inserter(output), " reassortant=\"{}\"", *value);
    if constexpr (std::is_same_v<AgSr, ae::chart::v2::Serum>) {
        if (const auto value = ag_sr.serum_id(); !value.empty())
            fmt::format_to(std::back_inserter(output), " serum_id=\"{}\"", *value);
    }
    if (const auto value = ag_sr.passage(); !value.empty())
        fmt::format_to(std::back_inserter(output), " passage=\"{}\" ptype={}", value, value.passage_type());
    if constexpr (std::is_same_v<AgSr, ae::chart::v2::Antigen>) {
        if (const auto value = ag_sr.date(); !value.empty())
            fmt::format_to(std::back_inserter(output), " date={}", *value);
    }
    else {
        if (const auto value = ag_sr.serum_species(); !value.empty())
            fmt::format_to(std::back_inserter(output), " serum_species=\"{}\"", *value);
    }
    if (const auto value = ag_sr.lineage(); value != ae::chart::v2::BLineage::Unknown)
        fmt::format_to(std::back_inserter(output), " lineage={}", value);
    if constexpr (std::is_same_v<AgSr, ae::chart::v2::Antigen>) {
        if (ag_sr.reference())
            fmt::format_to(std::back_inserter(output), " reference");
        if (const auto value = ag_sr.lab_ids().join(); !value.empty())
            fmt::format_to(std::back_inserter(output), " lab_ids=\"{}\"", value);
    }
    return fmt::to_string(output);
}

// ----------------------------------------------------------------------

std::string ae::chart::v2::detail::AntigenSerum::format(std::string_view pattern, collapse_spaces_t cs) const
{
    fmt::memory_buffer output;
    format(output, pattern);
    return ae::chart::v2::collapse_spaces(fmt::to_string(output), cs);

} // ae::chart::v2::detail::AntigenSerum::format

// ----------------------------------------------------------------------

// from seqdb sequence.hh
struct sequence_aligned_t : public ae::named_string_t<std::string, struct seqdb_sequence_aligned_tag_t>
{
    using base = named_string_t<std::string, struct seqdb_sequence_aligned_tag_t>;
    using base::named_string_t;
    char at(size_t pos0) const noexcept { return pos0 < size() ? operator[](pos0) : ' '; }
    size_t size() const noexcept { return base::size(); }
};

// {} - whole sequence
// {:193} - at 193
// {:193:6} - at 193-198 (inclusive)
template <> struct fmt::formatter<sequence_aligned_t>
{
    template <typename ParseContext> auto parse(ParseContext& ctx)
    {
        auto it = ctx.begin();
        const auto get = [&it, &ctx]() -> size_t {
            if (it != ctx.end() && *it == ':')
                ++it;
            if (it == ctx.end() || *it == '}')
                return 0;
            char* end;
            const auto value = std::strtoul(&*it, &end, 10);
            it = std::next(it, end - &*it);
            return value;
        };

        first_ = get();
        len_ = get();
        while (it != ctx.end() && *it != '}')
            ++it;
        return it;
    }

    template <typename Seq, typename FormatContext> auto format(const Seq& seq, FormatContext& ctx) const
    {
        if (first_ == 0)
            return format_to(ctx.out(), "{}", *seq);
        else
            return format_to(ctx.out(), "{}", seq->substr(first_ - 1, len_ ? len_ : 1));
    }

  private:
    size_t first_{0};
    size_t len_{0};
};

// ----------------------------------------------------------------------

#define FKF(key, call) std::pair{key, [](fmt::memory_buffer& output, std::string_view format, [[maybe_unused]] const auto& ag_sr) { fmt::format_to(std::back_inserter(output), fmt::runtime(format), fmt::arg(key, call)); }}

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif

const std::tuple format_subst_ag_sr{
    FKF("abbreviated_location_with_passage_type", ae::string::join(" ", location_abbreviated(ag_sr), ag_sr.passage().passage_type())),        // mapi
    FKF("abbreviated_name_with_passage_type", fmt::format("{}-{}", name_abbreviated(ag_sr), ag_sr.passage().passage_type())),                 // mapi
    FKF("annotations", ag_sr.annotations()),                                                                                                  //
    FKF("continent", ag_sr.location_data().continent),                                                                                        //
    FKF("country", ag_sr.location_data().country),                                                                                            //
    FKF("fields", fields(ag_sr)),                                                                                                             //
    FKF("full_name", name_full(ag_sr)),                                                                                                       //
    FKF("latitude", ag_sr.location_data().latitude),                                                                                          //
    FKF("lineage", ag_sr.lineage().to_string()),                                                                                              //
    FKF("location", ae::virus::name::location(ag_sr.name())),                                                                                 //
    FKF("location_abbreviated", location_abbreviated(ag_sr)),                                                                                 //
    FKF("longitude", ag_sr.location_data().longitude),                                                                                        //
    FKF("name", ag_sr.name()),                                                                                                                //
    FKF("name_abbreviated", name_abbreviated(ag_sr)),                                                                                         //
    FKF("name_full", name_full(ag_sr)),                                                                                                       //
    FKF("name_full_passage", name_full_passage(ag_sr)),                                                                                       //
    FKF("name_without_subtype", ae::virus::name::without_subtype(ag_sr.name())),                                                              //
    FKF("name_anntotations_reassortant", ae::string::join(" ", ag_sr.name(), fmt::format("{: }", ag_sr.annotations()), ag_sr.reassortant())), //
    FKF("passage", ag_sr.passage()),                                                                                                          //
    FKF("passage_type", ag_sr.passage().passage_type()),                                                                                      //
    FKF("reassortant", ag_sr.reassortant()),                                                                                                  //
    FKF("year", year4(ag_sr)),                                                                                                                //
    FKF("year2", year2(ag_sr)),                                                                                                               //
    FKF("year4", year4(ag_sr)),                                                                                                               //
    FKF("aa", sequence_aligned_t{ag_sr.sequence_aa()}),                                                                                       //
    FKF("nuc", sequence_aligned_t{ag_sr.sequence_nuc()}),                                                                                     //
};

const std::tuple format_subst_antigen{
    FKF("ag_sr", "AG"),                                         //
    FKF("date", ag_sr.date()),                                  //
    FKF("date_in_brackets", fmt::format("[{}]", ag_sr.date())), //
    FKF("designation", name_full(ag_sr)),                       //
    FKF("lab_ids", ag_sr.lab_ids().join()),                     //
    FKF("ref", ag_sr.reference() ? "Ref" : ""),                 //
    FKF("clades", ag_sr.clades()),                                                                                                                  //
    FKF("serum_species", ""),                                   //
    FKF("species", ""),                                         //
};

const std::tuple format_subst_serum{
    FKF("ag_sr", "SR"),                                                                            //
    FKF("designation", ae::string::join(" ", name_full_without_passage(ag_sr), ag_sr.serum_id())), //
    FKF("designation_without_serum_id", name_full_without_passage(ag_sr)),                         //
    FKF("serum_id", ag_sr.serum_id()),                                                             //
    FKF("serum_species", ag_sr.serum_species()),                                                   //
    FKF("serum_species", ag_sr.serum_species()),                                                   //
    FKF("species", ag_sr.serum_species()),                                                         //
    FKF("date_in_brackets", ""),                                                                   //
    FKF("clades", ""),                                                                             //
    FKF("lab_ids", ""),                                                                            //
    FKF("ref", ""),                                                                                //
};

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

namespace fmt
{
    enum class if_no_substitution_found { leave_as_is, empty };

    inline std::vector<std::pair<std::string_view, std::string_view>> split_for_formatting(std::string_view source) // pair{key, format}: {"key", "{key:03d}"} {"", "between-format"}
    {
        std::vector<std::pair<std::string_view, std::string_view>> result;
        size_t beg{0};
        size_t inside_format{0};
        for (size_t pos{0}; pos < source.size(); ++pos) {
            switch (source[pos]) {
                case '{':
                    if (pos > beg && inside_format == 0) {
                        result.push_back(std::pair{source.substr(beg, pos - beg), source.substr(beg, pos - beg)});
                        // result.push_back(std::pair{std::string_view{}, source.substr(beg, pos - beg)});
                        beg = pos;
                    }
                    ++inside_format;
                    break;
                case '}':
                    if (inside_format > 0)
                        --inside_format;
                    if (inside_format == 0) {
                        const auto pattern = source.substr(beg, pos - beg + 1);
                        if (const auto end = pattern.find(':', 1); end != std::string_view::npos)
                            result.push_back(std::pair{pattern.substr(1, end - 1), pattern});
                        else
                            result.push_back(std::pair{pattern.substr(1, pattern.size() - 2), pattern});
                        beg = pos + 1;
                    }
                    break;
            }
        }
        if (beg < source.size())
            result.push_back(std::pair{std::string_view{}, source.substr(beg)});
        // AD_DEBUG("split_for_formatting {}", result);
        return result;
    }

    template <typename FormatMatched, typename FormatNoPattern, typename... Args>
    void substitute_to(FormatMatched&& format_matched, FormatNoPattern&& format_no_pattern, std::string_view pattern, if_no_substitution_found insf, Args&&... args)
    {
        const auto match_and_format = [&format_matched](std::string_view look_for, std::string_view pattern_arg, const auto& en) {
            if (look_for == std::get<0>(en)) {
                format_matched(pattern_arg, en);
                return true;
            }
            else
                return false;
        };

        for (const auto& [key, pattern_arg] : split_for_formatting(pattern)) {
            if (!key.empty()) {
                if (!(match_and_format(key, pattern_arg, args) || ...)) {
                    // not matched
                    switch (insf) {
                        case if_no_substitution_found::leave_as_is:
                            format_no_pattern(pattern_arg);
                            break;
                        case if_no_substitution_found::empty:
                            break;
                    }
                }
            }
            else
                format_no_pattern(pattern_arg);
        }
    }

    // substitute_to args:
    // std::pair{"name", value}                                     -- {name}, {name:3d}
    // std::pair{"name", []() -> decltype(value) { return value; }} -- {name}, {name:3d}
    // std::tuple{"name1", val1, "name2", val2}                     -- {name1:{name2}d}
    template <typename... Args> void substitute_to(memory_buffer& output, std::string_view pattern, if_no_substitution_found insf, Args&&... args)
    {
        const auto format_matched = [&output](std::string_view pattern_arg, const auto& key_value) {
            static_assert(std::is_same_v<std::decay_t<decltype(std::get<0>(key_value))>, const char*>);
            if constexpr (std::tuple_size<std::decay_t<decltype(key_value)>>::value == 2) {
                if constexpr (std::is_invocable_v<decltype(std::get<1>(key_value))>)
                    format_to(std::back_inserter(output), fmt::runtime(pattern_arg), arg(std::get<0>(key_value), std::invoke(std::get<1>(key_value))));
                else
                    format_to(std::back_inserter(output), fmt::runtime(pattern_arg), arg(std::get<0>(key_value), std::get<1>(key_value)));
            }
            else if constexpr (std::tuple_size<std::decay_t<decltype(key_value)>>::value == 4) {
                format_to(std::back_inserter(output), fmt::runtime(pattern_arg), arg(std::get<0>(key_value), std::get<1>(key_value)), arg(std::get<2>(key_value), std::get<3>(key_value)));
            }
            else
                static_assert(
                    std::tuple_size<std::decay_t<decltype(key_value)>>::value == 0,
                    "fmt::substitute arg can be used in the following forms: std::pair<const char*, value>, std::pair<const char*, lambda>, std::tuple<const char*, value, const char*, value>");
        };
        const auto format_no_pattern = [&output](std::string_view no_pattern) { output.append(no_pattern); };
        substitute_to(format_matched, format_no_pattern, pattern, insf, std::forward<Args>(args)...);
    }

    // see acmacs-chart-2/cc/name-format.cc for usage example

    template <typename... Args> std::string substitute(std::string_view pattern, if_no_substitution_found insf, Args&&... args)
    {
        memory_buffer output;
        substitute_to(output, pattern, insf, std::forward<Args>(args)...);
        return to_string(output);
    }

    template <typename... Args> std::string substitute(std::string_view pattern, Args&&... args) { return substitute(pattern, if_no_substitution_found::leave_as_is, std::forward<Args>(args)...); }

} // namespace fmt

// ----------------------------------------------------------------------

template <typename AgSr, typename... Args> static inline void format_ag_sr(fmt::memory_buffer& output, const AgSr& ag_sr, std::string_view pattern, Args&&... args)
{
    const auto format_matched = [&output, &ag_sr](std::string_view pattern_arg, const auto& key_value) {
        static_assert(std::is_same_v<std::decay_t<decltype(std::get<0>(key_value))>, const char*>);
        if constexpr (std::is_invocable_v<decltype(std::get<1>(key_value)), fmt::memory_buffer&, std::string_view, const AgSr&>)
            std::invoke(std::get<1>(key_value), output, pattern_arg, ag_sr);
        else
            fmt::format_to(std::back_inserter(output), fmt::runtime(pattern_arg), fmt::arg(std::get<0>(key_value), std::get<1>(key_value)));
    };
    const auto format_no_pattern = [&output](std::string_view no_pattern) { output.append(no_pattern); };
    const auto format_args = [pattern, format_matched, format_no_pattern](const auto&... fargs) {
        fmt::substitute_to(format_matched, format_no_pattern, pattern, fmt::if_no_substitution_found::leave_as_is, fargs...);
    };
    std::apply(format_args, std::tuple_cat(args...));
}

// ----------------------------------------------------------------------


void ae::chart::v2::Antigen::format(fmt::memory_buffer& output, std::string_view pattern) const
{
    ::format_ag_sr(output, *this, pattern, format_subst_ag_sr, format_subst_antigen);

} // ae::chart::v2::Antigen::format

// ----------------------------------------------------------------------

void ae::chart::v2::Serum::format(fmt::memory_buffer& output, std::string_view pattern) const
{
    ::format_ag_sr(output, *this, pattern, format_subst_ag_sr, format_subst_serum);

} // ae::chart::v2::Serum::format

// ----------------------------------------------------------------------

const ae::chart::v2::detail::location_data_t& ae::chart::v2::detail::AntigenSerum::location_data() const
{
    if (!location_data_filled_) {
        const auto location{ae::virus::name::location(name())};
        const auto& locdb = ae::locdb::get();
        if (const auto [name, loc] = locdb.find(location); loc)
            location_data_ = location_data_t{std::string{name}, std::string{loc->country}, std::string{locdb.continent(loc->country)}, loc->latitude, loc->longitude};
        else
            location_data_ = location_data_t{std::string{location}, "*unknown*", "*unknown*", 360.0, 360.0};
        location_data_filled_ = true;
    }
    return location_data_;

} // ae::chart::v2::detail::AntigenSerum::location_data

// ----------------------------------------------------------------------

#define CS_SPACES "([ \t])"
#define CS_SPACES_OPT CS_SPACES "([ \t]*)"
#define CS_SPACES_OR_CS_OPT "( |\t|\\{ \\})*"
#define CS_CS1 "(\\{ \\})+"

std::string ae::chart::v2::collapse_spaces(std::string src, collapse_spaces_t cs)
{
#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

    static const std::array replace_data{
        ae::regex::look_replace_t{std::regex("^" CS_SPACES_OPT CS_CS1, std::regex::icase), {"$1$'"}},
        ae::regex::look_replace_t{std::regex(CS_CS1 CS_SPACES_OPT "$", std::regex::icase), {"$`$1"}},
        ae::regex::look_replace_t{std::regex(CS_SPACES_OR_CS_OPT CS_CS1 CS_SPACES_OR_CS_OPT, std::regex::icase), {"$` $'"}},
    };
#pragma GCC diagnostic pop

    switch (cs) {
        case collapse_spaces_t::yes:
            while (true) {
                if (const auto replacement = scan_replace(src, replace_data); replacement.has_value())
                    src = replacement->back();
                else
                    break;
            }
            break;
        case collapse_spaces_t::no:
            break;
    }
    return src;

} // ae::chart::v2::collapse_spaces

// ----------------------------------------------------------------------

std::string ae::chart::v2::format_antigen(std::string_view pattern, const Chart& chart, size_t antigen_no, collapse_spaces_t cs)
{
    const auto num_digits = chart.number_of_digits_for_antigen_serum_index_formatting();
    auto antigen = chart.antigens()->at(antigen_no);

    const auto ag_formatted = antigen->format(pattern, collapse_spaces_t::no);
    try {
        const auto substituted = fmt::substitute(                                                                                     //
            ag_formatted,                                                                                                             //
            fmt::if_no_substitution_found::leave_as_is,                                                                               //
            std::tuple{"no0", antigen_no, "num_digits", num_digits},                                                                  //
            std::tuple{"no1", antigen_no + 1, "num_digits", num_digits},                                                                  //
            std::pair{"sera_with_titrations", [&chart, antigen_no] { return chart.titers()->having_titers_with(antigen_no, false); }} //
        );
        return ae::chart::v2::collapse_spaces(substituted, cs);
    }
    catch (fmt::format_error& err) {
        AD_ERROR("format_error in {}: {}", ag_formatted, err.what());
        throw;
    }

} // ae::chart::v2::format_antigen

// ----------------------------------------------------------------------

std::string ae::chart::v2::format_serum(std::string_view pattern, const Chart& chart, size_t serum_no, collapse_spaces_t cs)
{
    const auto num_digits = chart.number_of_digits_for_antigen_serum_index_formatting();
    auto serum = chart.sera()->at(serum_no);

    const auto substituted = fmt::substitute(                                                                        //
        serum->format(pattern, collapse_spaces_t::no),                                                                 //
        std::tuple{"no0", serum_no, "num_digits", num_digits},                                                                  //
        std::tuple{"no1", serum_no + 1, "num_digits", num_digits},                                                                  //
        std::pair{"sera_with_titrations", chart.titers()->having_titers_with(serum_no + chart.number_of_antigens())} //
    );

    return collapse_spaces(substituted, cs);

} // ae::chart::v2::format_serum

// ----------------------------------------------------------------------

std::string ae::chart::v2::format_point(std::string_view pattern, const Chart& chart, size_t point_no, collapse_spaces_t cs)
{
    if (const auto num_ags = chart.number_of_antigens(); point_no < num_ags)
        return format_antigen(pattern, chart, point_no, cs);
    else
        return format_serum(pattern, chart, point_no - num_ags, cs);

} // ae::chart::v2::format_point

// ----------------------------------------------------------------------

constexpr const std::string_view pattern = R"(
{{ag_sr}}                                  : {ag_sr}
{{no0}}                                    : {no0}
{{no0:{num_digits}d}}                      : {no0:{num_digits}d}
{{no1}}                                    : {no1}
{{no1:{num_digits}d}}                      : {no1:{num_digits}d}
{{name}}                                   : {name}
{{full_name}}                              : {full_name}
{{name_full_passage}}                      : {name_full_passage}
{{fields}}                                 : {fields}
{{species}}                                : {species}
{{date}}                                   : {date}
{{lab_ids}}                                : {lab_ids}
{{ref}}                                    : {ref}
{{serum_id}}                               : {serum_id}
{{reassortant}}                            : {reassortant}
{{passage}}                                : {passage}
{{passage_type}}                           : {passage_type}
{{annotations}}                            : {annotations}
{{lineage}}                                : {lineage}
{{abbreviated_name}}                       : {abbreviated_name}
{{abbreviated_name_with_passage_type}}     : {abbreviated_name_with_passage_type}
{{abbreviated_name_with_serum_id}}         : {abbreviated_name_with_serum_id}
{{abbreviated_location_with_passage_type}} : {abbreviated_location_with_passage_type}
{{abbreviated_location_year}}              : {abbreviated_location_year}
{{designation}}                            : {designation}
{{name_abbreviated}}                       : {name_abbreviated}
{{name_without_subtype}}                   : {name_without_subtype}
{{location}}                               : {location}
{{location_abbreviated}}                   : {location_abbreviated}
{{country}}                                : {country}
{{continent}}                              : {continent}
{{latitude}}                               : {latitude}
{{longitude}}                              : {longitude}
{{sera_with_titrations}}                   : {sera_with_titrations}
{{aa}}                                     : {aa}
{{aa:193}}                                 : {aa:193}
{{aa:193:6}}                               : {aa:193:6}
{{nuc}}                                    : {nuc}
)";

std::string ae::chart::v2::format_help()
{
    ChartNew chart{101, 101};

    auto& antigen = chart.antigens_modify().at(67);
    antigen.name("A(H3N2)/KLAGENFURT/24/2020");
    antigen.date("2020-05-24");
    antigen.passage(ae::virus::Passage{"E1 (2020-04-01)"});
    antigen.reassortant(ae::virus::Reassortant{"NYMC-1A"});
    antigen.add_annotation("FLEDERMAUS");
    antigen.add_annotation("BAT");
    antigen.reference(true);
    antigen.add_clade("3C.3A");

    auto& serum = chart.sera_modify().at(12);
    serum.name("B/WUHAN/24/2020");
    serum.passage(ae::virus::Passage{"MDCK1/SIAT2 (2020-04-01)"});
    serum.lineage("YAMAGATA");
    serum.serum_id(SerumId{"2020-031"});
    serum.serum_species(SerumSpecies{"RAT"});

    return fmt::format("{}\n\n{}\n", format_antigen(pattern, chart, 67, collapse_spaces_t::yes), format_serum(pattern, chart, 12, collapse_spaces_t::yes));

} // ae::chart::v2::format_help

// ----------------------------------------------------------------------
