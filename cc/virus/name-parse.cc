#include <array>
#include <vector>
#include <cctype>
#include <algorithm>

#include "virus/name-parse.hh"
#include "ext/lexy.hh"
#include "ext/date.hh"
#include "ext/from_chars.hh"
#include "locdb/locdb.hh"

// ======================================================================

namespace ae::virus::name::inline v1
{

    template <class T>
    concept Lexeme = requires(T a)
    {
        {a.begin()};
        {a.end()};
    };

    template <typename Iter> inline std::string uppercase_strip(Iter first, Iter last)
    {
        while (first != last && std::isspace(*first))
            ++first; // remove leading spaces
        std::string res(static_cast<size_t>(last - first), '?');
        auto output_end = std::transform(first, last, res.begin(), [](unsigned char cc) { return std::toupper(cc); });
        if ((output_end - res.begin()) > 1) { // output is not empty
            --output_end;
            while (output_end != res.begin() && std::isspace(*output_end))
                --output_end; // remove trailing spaces
            res.resize(static_cast<size_t>(output_end - res.begin()) + 1);
        }
        return res;
    }

    template <Lexeme Lex> inline std::string uppercase_strip(Lex&& lexeme) { return uppercase_strip(lexeme.begin(), lexeme.end()); }

    enum class part_type {
        type_subtype, // A, B, A(H3N2)
        subtype, // H3N2
        any,
        letters_only, // letter, cjk, space, underscore, dash
        letter_first, // first symbol is a letter or cjk
        digits_only,  // just decimal digits
        digit_first,  // first symbol is a decimal digit
        digits_hyphens,  // e.g. 2021-10-15
        size_
    };

    struct part_t
    {
        struct type_t : public std::bitset<static_cast<size_t>(part_type::size_)>
        {
            type_t() = default;
            type_t(part_type tt)
            {
                switch (tt) {
                    case part_type::type_subtype:
                    case part_type::subtype:
                    case part_type::any:
                        set(tt);
                        break;
                    case part_type::letters_only:
                        set(tt);
                        set(part_type::any);
                        set(part_type::letter_first);
                        break;
                    case part_type::letter_first:
                        set(tt);
                        set(part_type::any);
                        break;
                    case part_type::digits_only:
                        set(tt);
                        set(part_type::any);
                        set(part_type::digit_first);
                        set(part_type::digits_hyphens);
                        break;
                    case part_type::digit_first:
                        set(tt);
                        set(part_type::any);
                        break;
                    case part_type::digits_hyphens:
                        set(tt);
                        set(part_type::digit_first);
                        set(part_type::any);
                        break;
                    case part_type::size_:
                        break;
                }
            }

            void set(part_type tt) { std::bitset<static_cast<size_t>(part_type::size_)>::set(static_cast<size_t>(tt)); }
        };

        std::string head{};
        std::string tail{};
        type_t type{};

        part_t() = default;
        part_t(const part_t&) = default;
        part_t(part_t&&) = default;
        part_t(std::string&& text, type_t typ) : head{std::move(text)}, type{typ} {}
        part_t(const char* text, type_t typ) : head{text}, type{typ} {}
        part_t(type_t typ) : type{typ} {}
        template <Lexeme Lex> part_t(Lex&& lexeme, type_t typ) : head{uppercase_strip(lexeme)}, type{typ} {}
        template <Lexeme Lex> part_t(Lex&& lex1, Lex&& lex2, type_t typ) : head{uppercase_strip(lex1)}, tail{uppercase_strip(lex2)}, type{typ} {}
        part_t& operator=(const part_t&) = default;
        part_t& operator=(part_t&&) = default;
        constexpr bool empty() const { return head.empty(); }
        operator std::string() const { return fmt::format("{}{}", head, tail); }
    };

    using parts_t = std::array<part_t, 8>;

    inline bool types_match(const parts_t& parts, std::initializer_list<part_type> types)
    {
        // types is no longer than parts
        return std::equal(types.begin(), types.end(), parts.begin(), [](part_type e2, const part_t& e1) { return e1.type[static_cast<size_t>(e2)]; }) &&
               std::all_of(std::next(parts.begin(), types.size()), parts.end(), [](const auto& part) { return part.type.none(); });
    }

    // ======================================================================

    namespace grammar
    {
        namespace dsl = lexy::dsl;

        // ----------------------------------------------------------------------

        struct letter_extra_predicate
        {
            constexpr bool operator()(lexy::code_point cp)
            {                                                       //
                return                                              //
                    (cp.value() >= 0xC0 && cp.value() <= 0xFF)      // Latin-1 Supplement (including math x and math division symbol)
                    || (cp.value() >= 0x100 && cp.value() <= 0x17F) // Latin Extended-A
                    // cyrillic ?
                    || (cp.value() >= 0x4E00 && cp.value() <= 0x9FFF) // CJK Unified Ideographs
                    ;
            }
        };

        static constexpr auto letter_extra = dsl::code_point.if_<letter_extra_predicate>();

        // ----------------------------------------------------------------------

        struct subtype_a_hn
        {
            static constexpr auto whitespace = dsl::ascii::blank; // auto skip whitespaces
            static constexpr auto H = dsl::lit_c<'H'> / dsl::lit_c<'h'>;
            static constexpr auto N = dsl::lit_c<'N'> / dsl::lit_c<'n'>;

            // H3N2 | H3
            static constexpr auto rule = dsl::peek(H + dsl::digit<>) >> dsl::capture(H + dsl::digits<>) + dsl::opt(dsl::peek(N) >> dsl::capture(N + dsl::digits<>));
            static constexpr auto value = lexy::callback<part_t>([](auto lex1, auto lex2) {
                if constexpr (std::is_same_v<decltype(lex2), lexy::nullopt>)
                    return part_t{uppercase_strip(lex1), part_type::subtype};
                else
                    return part_t{uppercase_strip(lex1.begin(), lex2.end()), part_type::subtype};
            });
        };

        struct subtype_a
        {
            static constexpr auto whitespace = dsl::ascii::blank; // auto skip whitespaces
            static constexpr auto A = dsl::lit_c<'A'> / dsl::lit_c<'a'>;
            static constexpr auto OPEN = dsl::lit_c<'('>;
            static constexpr auto CLOSE = dsl::lit_c<')'>;

            // A | AH3 | AH3N2 | A(H3N2) | A(H3)
            static constexpr auto rule = A >> dsl::opt(dsl::p<subtype_a_hn> | OPEN >> dsl::p<subtype_a_hn> + CLOSE);
            static constexpr auto value = lexy::callback<part_t>([](auto lex) {
                if constexpr (std::is_same_v<decltype(lex), lexy::nullopt>)
                    return part_t{"A", part_type::type_subtype};
                else
                    return part_t{fmt::format("A({})", lex.head), part_type::type_subtype};
            });
        };

        struct subtype_b
        {
            static constexpr auto whitespace = dsl::ascii::blank; // auto skip whitespaces
            static constexpr auto B = dsl::lit_c<'B'> / dsl::lit_c<'b'>;

            static constexpr auto rule = B;
            static constexpr auto value = lexy::callback<part_t>([]() { return part_t{"B", part_type::type_subtype}; });
        };

        // ----------------------------------------------------------------------

        // chunk starting with a letter, followed by letters, digits, -, _, :, space
        struct letters
        {
            static constexpr auto whitespace = dsl::ascii::blank; // auto skip whitespaces
            static constexpr auto letters_only = dsl::ascii::alpha / letter_extra / dsl::lit_c<'_'> / dsl::hyphen / dsl::ascii::blank;
            static constexpr auto mixed = letters_only / dsl::ascii::digit / dsl::colon / dsl::period;

            static constexpr auto rule = dsl::peek(dsl::while_(dsl::ascii::blank) + dsl::ascii::alpha / letter_extra) >>
                                         dsl::capture(dsl::while_(letters_only)) + dsl::opt(dsl::peek_not(dsl::lit_c<'/'>) >> dsl::capture(dsl::while_(mixed)));
            static constexpr auto value = lexy::callback<part_t>([](auto lex1, auto lex2) {
                if constexpr (std::is_same_v<decltype(lex2), lexy::nullopt>)
                    return part_t{lex1, part_type::letters_only};
                else
                    return part_t{lex1, lex2, part_type::letter_first};
            });
        };

        // chunk starting with a digit, followed by letters, digits, -, _, :, (BUT NO space)
        struct digits
        {
            static constexpr auto mixed = dsl::digits<> / dsl::ascii::alpha / letter_extra / dsl::lit_c<'_'> / dsl::hyphen / dsl::ascii::digit / dsl::colon / dsl::period; // NO blank!!
            template <Lexeme L> static constexpr bool digits_hyphens(const L& lex) {
                for (char cc : lex) {
                    if (!std::isdigit(cc) && cc != '-')
                        return false;
                }
                return true;
            }

            static constexpr auto rule = dsl::peek(dsl::digit<>) >> dsl::capture(dsl::while_(dsl::digits<>)) + dsl::opt(dsl::peek(mixed) >> dsl::capture(dsl::while_(mixed)));
            static constexpr auto value = lexy::callback<part_t>([](auto lex1, auto lex2) {
                if constexpr (std::is_same_v<decltype(lex2), lexy::nullopt>)
                    return part_t{lex1, part_type::digits_only};
                else if (digits_hyphens(lex2))
                    return part_t{lex1, lex2, part_type::digits_hyphens};
                else
                    return part_t{lex1, lex2, part_type::digit_first};
            });
        };

        struct slash_separated
        {
            static constexpr auto rule = dsl::list(dsl::p<subtype_a_hn> | dsl::p<letters> | dsl::p<digits>, dsl::sep(dsl::slash));
            static constexpr auto value = lexy::as_list<std::vector<part_t>>;
        };

        // ----------------------------------------------------------------------

        struct rest
        {
            static constexpr auto rule = dsl::capture(dsl::any);
            static constexpr auto value = lexy::callback<part_t>([](auto lexeme) { return part_t{lexeme, part_type::any}; });
        };

        // ----------------------------------------------------------------------

        struct parts
        {
            static constexpr auto rule = (dsl::p<subtype_a> | dsl::p<subtype_b>)+dsl::slash + dsl::p<slash_separated> + dsl::p<rest> + dsl::eof;

            static constexpr auto value = lexy::callback<parts_t>([](auto subtype, auto slash_separated, auto rest) {
                parts_t parts{subtype};
                const auto last_output = std::move(std::begin(slash_separated), std::end(slash_separated), std::next(std::begin(parts)));
                if (!rest.empty())
                    *last_output = rest;
                return parts;
            });
        };

    } // namespace grammar

    // ----------------------------------------------------------------------

    // return source unchanged, if location not found but add message
    inline std::string_view fix_location(const std::string& location, std::string_view source, Parts& parts, parse_settings& settings, std::string_view context)
    {
        if (const auto fixed = locationdb::get().find(location); !fixed.empty()) {
            return fixed;
        }
        else {
            parts.issues.add(Parts::issue::unrecognized_location);
            settings.messages().unrecognized_location(location, fmt::format("{} {}", source, context));
            return location;
        }
    }

    // return source unchanged, if year is not valid but add message
    inline std::string fix_year(const std::string& year, std::string_view source, Parts& parts, parse_settings& settings, std::string_view context)
    {
        try {
            const size_t earlierst_year = 1900;
            const auto today_year = date::today_year();
            switch (year.size()) {
                case 4:
                    if (const auto year_i = ae::from_chars<size_t>(year); year_i < earlierst_year || year_i > today_year)
                        throw std::invalid_argument{fmt::format("out of range {}..{}", earlierst_year, today_year)};
                    return year;
                case 2:
                    if (const auto year_i = ae::from_chars<size_t>(year); year_i <= (today_year - 2000))
                        return fmt::format("{}", year_i + 2000);
                    else
                        return fmt::format("{}", year_i + 1900);
                case 10:        // e.g. 2021-10-15
                    if (year[4] != '-' && year[7] != '-')
                        throw std::invalid_argument{fmt::format("not a date")};
                    if (const auto year_i = ae::from_chars<size_t>(std::string_view(year.data(), 4)); year_i < earlierst_year || year_i > today_year)
                        throw std::invalid_argument{fmt::format("out of range {}..{}", earlierst_year, today_year)};
                    return year.substr(0, 4);
                default:
                    throw std::invalid_argument{"unsupported value length"};
            }
        }
        catch (std::exception& err) {
            parts.issues.add(Parts::issue::invalid_year);
            settings.messages().message(fmt::format("invalid year: \"{}\": {}", year, err.what()), fmt::format("{} {}", source, context));
        }
        return year;
    }

} // namespace ae::virus::name::inline v1

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::virus::name::part_t::type_t> : fmt::formatter<eu::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const ae::virus::name::part_t::type_t& value, FormatCtx& ctx)
    {
        using namespace ae::virus::name;

        bool empty = true;
        const auto add = [&empty, &ctx](const char* text) {
            if (!empty)
                format_to(ctx.out(), "{}", '|');
            else
                empty = false;
            format_to(ctx.out(), "{}", text);
        };

        for (size_t pt = 0; pt < value.size(); ++pt)
        {
            if (value[pt]) {
                switch (static_cast<part_type>(pt)) {
                    case part_type::type_subtype:
                        add("type_subtype");
                        break;
                    case part_type::subtype:
                        add("subtype");
                        break;
                    case part_type::any:
                        add("any");
                        break;
                    case part_type::letters_only:
                        add("letters_only");
                        break;
                    case part_type::letter_first:
                        add("letter_first");
                        break;
                    case part_type::digits_only:
                        add("digits_only");
                        break;
                    case part_type::digit_first:
                        add("digit_first");
                        break;
                    case part_type::digits_hyphens:
                        add("digits_hyphens");
                        break;
                    case part_type::size_:
                        break;
                }
            }
        }
        return ctx.out();
    }
};

template <> struct fmt::formatter<ae::virus::name::part_t> : fmt::formatter<eu::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const ae::virus::name::part_t& value, FormatCtx& ctx)
    {
        if (value.type.any())
            format_to(ctx.out(), "<{}>\"{}{}\"", value.type, value.head, value.tail);
        return ctx.out();
    }
};

// ======================================================================

std::string ae::virus::name::v1::Parts::name(mark_extra me) const
{
    fmt::memory_buffer out;
    const std::string_view nothing{}, space{" "}, slash{"/"}, question{"?"};
    const auto format_or = [&out](std::string_view field, std::string_view alt, std::string_view sep) {
        if (!field.empty())
            fmt::format_to(std::back_inserter(out), "{}{}", sep, field);
        else if (!alt.empty())
            fmt::format_to(std::back_inserter(out), "{}{}", sep, alt);
    };
    format_or(subtype, question, nothing);
    format_or(host, nothing, slash);
    format_or(location, question, slash);
    format_or(isolation, question, slash);
    format_or(year, question, slash);
    format_or(reassortant, nothing, space);
    format_or(extra, nothing, me == mark_extra::yes ? std::string_view{" * "} : space);
    return fmt::to_string(out);

} // ae::virus::name::v1::Parts::name

// ----------------------------------------------------------------------

ae::virus::name::v1::Parts ae::virus::name::v1::parse(std::string_view source, parse_settings& settings, std::string_view context)
{
    Parts result;

    if (settings.trace()) {
        fmt::print(">>> parsing \"{}\"\n", source);
        lexy::trace<grammar::parts>(stderr, lexy::string_input<lexy::utf8_encoding>{source});
    }
    const auto parsing_result = lexy::parse<grammar::parts>(lexy::string_input<lexy::utf8_encoding>{source}, lexy_ext::report_error);
    const auto parts = parsing_result.value();
    fmt::print("{}\n", parts);
    if (types_match(parts, {part_type::type_subtype, part_type::letters_only, part_type::any, part_type::digits_hyphens})) {
        // A(H3N2)/SINGAPORE/INFIMH-16-0019/2016
        // A/SINGAPORE/INFIMH-16-0019/2016
        result.subtype = parts[0];
        result.location = fix_location(parts[1], source, result, settings, context);
        result.isolation = parts[2];
        result.year = fix_year(parts[3], source, result, settings, context);
    }
    else if (types_match(parts, {part_type::type_subtype, part_type::subtype, part_type::letters_only, part_type::any, part_type::digits_hyphens})) {
        if (parts[0].head.size() == 1) {
            // A/H3N2/SINGAPORE/INFIMH-16-0019/2016
            result.subtype = fmt::format("{}({})", parts[0].head, parts[1].head);
        }
        else {
            // A(H3N2)/H3N2/SINGAPORE/INFIMH-16-0019/2016
            result.subtype = fmt::format("{}+{}", parts[0].head, parts[1].head);
            result.issues.add(Parts::issue::invalid_subtype);
            settings.messages().message(fmt::format("invalid subtype: \"{}/{}\"", parts[0].head, parts[1].head), fmt::format("{} {}", source, context));
        }
        result.location = fix_location(parts[2], source, result, settings, context);
        result.isolation = parts[3];
        result.year = fix_year(parts[4], source, result, settings, context);
    }
    else if (types_match(parts, {part_type::type_subtype, part_type::letters_only, part_type::letters_only, part_type::any, part_type::digits_hyphens})) {
        const auto& locdb = locationdb::get();
        if (const auto loc1 = locdb.find(parts[1].head), loc2 = locdb.find(parts[2].head); loc1.empty() && !loc2.empty()) {
            // A(H3N2)/HUMAN/SINGAPORE/INFIMH-16-0019/2016
            result.subtype = parts[0];
            result.host = parts[1];
            result.location = loc2;
            result.isolation = parts[3];
            result.year = fix_year(parts[4], source, result, settings, context);
        }
        else if (const auto loc12 = locdb.find(fmt::format("{} {}", parts[1].head, parts[2].head)); !loc12.empty()) {
            // A(H3N2)/LYON/CHU/19/2016
            result.subtype = parts[0];
            result.location = loc12;
            result.isolation = parts[3];
            result.year = fix_year(parts[4], source, result, settings, context);
        }
        else if (!loc1.empty() && loc2.empty()) {
            // A(H3N2)/LYON/XXX/19/2016
            result.subtype = parts[0];
            result.location = loc1;
            result.isolation = fmt::format("{}-{}", parts[2].head, std::string{parts[3]});
            result.year = fix_year(parts[4], source, result, settings, context);
        }
        else {                  // unrecognized_location
            result.subtype = parts[0];
            result.location = fmt::format("{}/{}", parts[1].head, parts[2].head);
            result.issues.add(Parts::issue::unrecognized_location);
            settings.messages().unrecognized_location(result.location, fmt::format("{} {}", source, context));
            result.isolation = parts[3];
            result.year = fix_year(parts[4], source, result, settings, context);
        }
    }
    // A/Zambia/13/174/2013
    // A/Lyon/CHU18.54.48/2018
    // reassortant at the end
    // IVR-153 (A/CALIFORNIA/07/2009)
    // (H3N2) at the end
    // A/swine/Chachoengsao/2003
    // A/chicken/Iran221/2001
    // A/chicken/Yunnan/Kunming/2007 -> A/chicken/Yunnan Kunming/?/2007
    // extra at the end
    else
        settings.messages().message(fmt::format("unhandled name: \"{}\" {}", source, parts), fmt::format("{} {}", source, context));
    return result;
}

// ----------------------------------------------------------------------
