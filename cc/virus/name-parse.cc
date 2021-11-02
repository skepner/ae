#include <array>
#include <vector>
#include <cctype>
#include <algorithm>

#include "virus/name-parse.hh"
#include "utils/messages.hh"
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
        reassortant,            // IVR-ddd
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
                    case part_type::reassortant:
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

    inline parts_t operator+(const parts_t& parts, const part_t& to_append)
    {
        if (to_append.empty())
            return parts;
        parts_t res{parts};
        if (const auto end = std::find_if(std::begin(res), std::end(res), [](const part_t& part) { return part.type.none(); }); end != std::end(res))
            *end = to_append;
        return res;
    }

    inline bool types_match(const parts_t& parts, std::initializer_list<part_type> types)
    {
        // types is no longer than parts
        return std::equal(types.begin(), types.end(), parts.begin(), [](part_type e2, const part_t& e1) { return e1.type[static_cast<size_t>(e2)]; }) &&
               std::all_of(std::next(parts.begin(), types.size()), parts.end(), [](const auto& part) { return part.type.none(); });
    }

    inline bool type_match(const part_t& part, part_type type) { return part.type[static_cast<size_t>(type)]; }

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

        static constexpr auto A = dsl::lit_c<'A'> / dsl::lit_c<'a'>;
        static constexpr auto B = dsl::lit_c<'B'> / dsl::lit_c<'b'>;
        static constexpr auto C = dsl::lit_c<'C'> / dsl::lit_c<'c'>;
        // static constexpr auto D = dsl::lit_c<'D'> / dsl::lit_c<'d'>;
        static constexpr auto E = dsl::lit_c<'E'> / dsl::lit_c<'e'>;
        // static constexpr auto F = dsl::lit_c<'F'> / dsl::lit_c<'f'>;
        // static constexpr auto G = dsl::lit_c<'G'> / dsl::lit_c<'g'>;
        static constexpr auto H = dsl::lit_c<'H'> / dsl::lit_c<'h'>;
        static constexpr auto I = dsl::lit_c<'I'> / dsl::lit_c<'i'>;
        // static constexpr auto J = dsl::lit_c<'J'> / dsl::lit_c<'j'>;
        // static constexpr auto K = dsl::lit_c<'K'> / dsl::lit_c<'k'>;
        // static constexpr auto L = dsl::lit_c<'L'> / dsl::lit_c<'l'>;
        static constexpr auto M = dsl::lit_c<'M'> / dsl::lit_c<'m'>;
        static constexpr auto N = dsl::lit_c<'N'> / dsl::lit_c<'n'>;
        // static constexpr auto O = dsl::lit_c<'O'> / dsl::lit_c<'o'>;
        // static constexpr auto P = dsl::lit_c<'P'> / dsl::lit_c<'p'>;
        // static constexpr auto Q = dsl::lit_c<'Q'> / dsl::lit_c<'q'>;
        static constexpr auto R = dsl::lit_c<'R'> / dsl::lit_c<'r'>;
        static constexpr auto S = dsl::lit_c<'S'> / dsl::lit_c<'s'>;
        // static constexpr auto T = dsl::lit_c<'T'> / dsl::lit_c<'t'>;
        // static constexpr auto U = dsl::lit_c<'U'> / dsl::lit_c<'u'>;
        static constexpr auto V = dsl::lit_c<'V'> / dsl::lit_c<'v'>;
        // static constexpr auto W = dsl::lit_c<'W'> / dsl::lit_c<'w'>;
        static constexpr auto X = dsl::lit_c<'X'> / dsl::lit_c<'x'>;
        static constexpr auto Y = dsl::lit_c<'Y'> / dsl::lit_c<'y'>;
        // static constexpr auto Z = dsl::lit_c<'Z'> / dsl::lit_c<'z'>;

        static constexpr auto OPEN = dsl::lit_c<'('>;
        static constexpr auto CLOSE = dsl::lit_c<')'>;
        static constexpr auto OPT_SPACES = dsl::while_(dsl::ascii::blank);

        // ----------------------------------------------------------------------

        struct subtype_a_hn
        {
            static constexpr auto whitespace = dsl::ascii::blank; // auto skip whitespaces

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

            static constexpr auto rule = B;
            static constexpr auto value = lexy::callback<part_t>([]() { return part_t{"B", part_type::type_subtype}; });
        };

        // ----------------------------------------------------------------------

        struct cber
        {
            // BVR-11A, CBER-11A
            static constexpr auto peek_bvr = dsl::peek(B + V + R + dsl::hyphen / dsl::ascii::blank / dsl::digit<>);
            static constexpr auto peek_cber = dsl::peek(C + B + E + R + dsl::hyphen / dsl::ascii::blank / dsl::digit<>);
            static constexpr auto peek = peek_bvr | peek_cber;

            static constexpr auto rule = peek >> dsl::while_(dsl::ascii::alpha) + dsl::while_(dsl::hyphen / dsl::ascii::blank) + dsl::capture(dsl::while_(dsl::digit<> / dsl::ascii::alpha));
            static constexpr auto value = lexy::callback<part_t>([](auto lex) { return part_t{"CBER-" + uppercase_strip(lex), part_type::reassortant}; });
        };

        struct nymc_x_bx
        {
            // NYMC-307A, X-307A, BX-11, NYMC-X-307A, NYMC-X307A, NYMC X-307A,
            static constexpr auto peek_nymc = dsl::peek(N + Y + M + C + dsl::hyphen / dsl::ascii::blank / dsl::digit<>);
            static constexpr auto peek_bx = dsl::peek(B + X + dsl::hyphen / dsl::ascii::blank / dsl::digit<>);
            static constexpr auto peek_x = dsl::peek(X + dsl::hyphen / dsl::ascii::blank / dsl::digit<>);
            static constexpr auto peek = peek_nymc | peek_bx | peek_x;
            static constexpr auto nymc = peek_nymc                                  //
                                         >> dsl::while_(dsl::ascii::alpha)          //
                                                + (dsl::hyphen / dsl::ascii::blank) //
                + dsl::opt(dsl::peek(X / B) >> dsl::while_(dsl::ascii::alpha) + dsl::while_(dsl::hyphen / dsl::ascii::blank));
            static constexpr auto xbx = (peek_x | peek_bx) >> dsl::while_(dsl::ascii::alpha) + dsl::hyphen / dsl::ascii::blank;

            static constexpr auto rule = (nymc | xbx) + dsl::capture(dsl::while_(dsl::digit<> / dsl::ascii::alpha/ dsl::hyphen));
            static constexpr auto value = lexy::callback<part_t>( //
                [](lexy::nullopt, auto lex) {
                    return part_t{"NYMC-" + uppercase_strip(lex), part_type::reassortant};
                },
                [](auto lex) {
                    return part_t{"NYMC-" + uppercase_strip(lex), part_type::reassortant};
                } //
            );
        };

        struct reassortant
        {
            static constexpr auto whitespace = dsl::ascii::blank; // auto skip whitespaces
            static constexpr auto IVR = I + V + R;
            static constexpr auto CNIC = C + N + I + C;
            static constexpr auto SAN = S + A + N;
            static constexpr auto prefix = dsl::peek(IVR + dsl::hyphen) | dsl::peek(CNIC + dsl::hyphen) | dsl::peek(SAN + dsl::hyphen);

            static constexpr auto rule = (nymc_x_bx::peek >> dsl::p<nymc_x_bx>) //
                                         | (cber::peek >> dsl::p<cber>)         //
                                         | (prefix >> dsl::capture(dsl::while_(dsl::ascii::alpha) + dsl::hyphen + dsl::while_(dsl::digit<> / dsl::ascii::alpha)));
            static constexpr auto value = lexy::callback<part_t>( //
                [](const part_t& part) { return part; },          //
                [](auto lex) {
                    return part_t{uppercase_strip(lex), part_type::reassortant};
                } //
            );
        };

        // ----------------------------------------------------------------------

        // chunk starting with a letter, followed by letters, digits, -, _, :, space
        struct letters
        {
            static constexpr auto whitespace = dsl::ascii::blank; // auto skip whitespaces
            static constexpr auto letters_only = dsl::ascii::alpha / letter_extra / dsl::lit_c<'_'> / dsl::hyphen / dsl::ascii::blank / dsl::period / dsl::apostrophe;
            static constexpr auto mixed = letters_only / dsl::ascii::digit / dsl::colon / dsl::period;

            static constexpr auto rule = dsl::peek(OPT_SPACES + dsl::ascii::alpha / letter_extra) //
                                         >> dsl::capture(dsl::while_(letters_only)) + dsl::opt(dsl::peek_not(dsl::lit_c<'/'>) >> dsl::capture(dsl::while_(mixed)));
            static constexpr auto value = lexy::callback<part_t>([](auto lex1, auto lex2) {
                if constexpr (std::is_same_v<decltype(lex2), lexy::nullopt>)
                    return part_t{lex1, part_type::letters_only};
                else
                    return part_t{lex1, lex2, part_type::letter_first};
            });
        };

        // chunk starting with a digit, followed by letters, digits, -, _, :,
        struct digits
        {
            static constexpr auto mixed = dsl::digits<> / dsl::ascii::alpha / letter_extra / dsl::lit_c<'_'> / dsl::hyphen / dsl::ascii::digit / dsl::hash_sign / dsl::colon / dsl::period / dsl::ascii::blank;

            template <Lexeme L> static constexpr bool spaces_only(const L& lex)
            {
                for (char cc : lex) {
                    if (!std::isspace(cc))
                        return false;
                }
                return true;
            }

            static constexpr auto
                rule = dsl::peek(OPT_SPACES + dsl::digit<> / dsl::hash_sign) //
                       >> OPT_SPACES +
                              dsl::capture(dsl::while_(dsl::digit<>)) + dsl::opt(dsl::peek(mixed) >> dsl::capture(dsl::while_(mixed))) + dsl::opt(dsl::peek(OPT_SPACES + dsl::slash) >> OPT_SPACES);

            static constexpr auto value12 = [](auto lex1, auto lex2) {
                if constexpr (std::is_same_v<decltype(lex2), lexy::nullopt>)
                    return part_t{lex1, part_type::digits_only};
                else if (spaces_only(lex2))
                    return part_t{lex1, part_type::digits_only};
                else
                    return part_t{std::string(lex1.begin(), lex1.end()) + std::string(lex2.begin(), lex2.end()), part_type::digit_first}; // no strip!
            };
            static constexpr auto value = lexy::callback<part_t>([](auto lex1, auto lex2) { return value12(lex1, lex2); }, [](auto lex1, auto lex2, lexy::nullopt) { return value12(lex1, lex2); });
        };

        template <size_t N> struct year_followed_by_space
        {
            static constexpr auto rule = dsl::peek(OPT_SPACES + dsl::n_digits<N> + dsl::ascii::blank) >> (OPT_SPACES + dsl::capture(dsl::while_(dsl::digits<>)) + OPT_SPACES);
            static constexpr auto value = lexy::callback<part_t>( //
                [](auto lex) {
                    return part_t{lex, part_type::digits_only};
                });
        };

        struct slash_separated
        {
            static constexpr auto rule = dsl::list(dsl::p<letters> | dsl::p<year_followed_by_space<2>> | dsl::p<year_followed_by_space<4>> | dsl::p<digits>, dsl::sep(dsl::slash));
            static constexpr auto value = lexy::as_list<std::vector<part_t>>;
        };

        // ----------------------------------------------------------------------

        struct rest
        {
            static constexpr auto rule = dsl::capture(dsl::any);
            static constexpr auto value = lexy::callback<part_t>([](auto lexeme) { return part_t{lexeme, part_type::any}; });
        };

        // ----------------------------------------------------------------------

        struct virus_name
        {
            static constexpr auto rule = (dsl::p<subtype_a> | dsl::p<subtype_b>) + dsl::slash + dsl::p<slash_separated> + OPT_SPACES;
            static constexpr auto value = lexy::callback<parts_t>([](auto subtype, auto slash_separated) {
                parts_t parts{subtype};
                std::move(std::begin(slash_separated), std::end(slash_separated), std::next(std::begin(parts)));
                return parts;
            });

        };

        struct parts
        {
            static constexpr auto rule =
                ((dsl::p<reassortant> >> OPT_SPACES + OPEN + dsl::p<virus_name> + CLOSE)
                 | (dsl::else_ >> dsl::p<virus_name> + dsl::opt(dsl::p<reassortant>) + dsl::p<rest>))
                + dsl::eof;
            static constexpr auto value = lexy::callback<parts_t>(
                [](const part_t& reassortant, const parts_t& virus_name) {
                    return virus_name + reassortant; // move reassortant to the end
                },
                [](const parts_t& virus_name, lexy::nullopt, auto rest) {
                    return virus_name + rest;
                },
                [](const parts_t& virus_name, const part_t& reassortant, auto rest) {
                    return virus_name + reassortant + rest;
                }
            );
        };

    } // namespace grammar

    // ----------------------------------------------------------------------

    // return source unchanged, if location not found but add message
    inline std::string_view fix_location(const std::string& location, std::string_view source, Parts& parts, Messages& messages, const MessageLocation& message_location)
    {
        if (const auto fixed = locationdb::get().find(location); !fixed.empty()) {
            return fixed;
        }
        else {
            parts.issues.add(Parts::issue::unrecognized_location);
            messages.add(Message::unrecognized_location, location, source, message_location);
            return location;
        }
    }

    // drop leading zeros
    inline std::string_view fix_isolation(const std::string& isolation_s, std::string_view /*source*/, Parts& /*parts*/, Messages& /*messages*/, const MessageLocation& /*message_location*/)
    {
        const std::string_view isolation{isolation_s};
        if (const auto non_zero_offset = isolation.find_first_not_of('0'); non_zero_offset != std::string_view::npos)
            return isolation.substr(non_zero_offset);
        else
            return isolation;
    }

    // return source unchanged, if year is not valid but add message
    inline std::string fix_year(const std::string& year, std::string_view source, Parts& parts, Messages& messages, const MessageLocation& message_location)
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
        catch (std::exception&) {
            parts.issues.add(Parts::issue::invalid_year);
            messages.add(Message::invalid_year, year, source, message_location);
        }
        return year;
    }

    inline std::string_view fix_reassortant(const std::string& reassortant, std::string_view /*source*/, Parts& /*parts*/, Messages& /*messages*/, const MessageLocation& /*message_location*/)
    {
        return reassortant;
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
                    case part_type::reassortant:
                        add("reassortant");
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

ae::virus::name::v1::Parts ae::virus::name::v1::parse(std::string_view source, parse_settings& settings, Messages& messages, const MessageLocation& message_location)
{
    Parts result;

    if (settings.trace()) {
        fmt::print(">>> parsing \"{}\"\n", source);
        lexy::trace<grammar::parts>(stderr, lexy::string_input<lexy::utf8_encoding>{source});
    }
    const auto parsing_result = lexy::parse<grammar::parts>(lexy::string_input<lexy::utf8_encoding>{source}, lexy_ext::report_error);
    const auto parts = parsing_result.value();
    if (settings.trace())
        fmt::print(">>> parts: {}\n", parts);
    if (types_match(parts, {part_type::type_subtype, part_type::letters_only, part_type::any, part_type::digits_hyphens})
       || types_match(parts, {part_type::type_subtype, part_type::letters_only, part_type::any, part_type::digits_hyphens, part_type::reassortant})) {
        // A(H3N2)/SINGAPORE/INFIMH-16-0019/2016
        // A/SINGAPORE/INFIMH-16-0019/2016
        // A/Pennsylvania/1025/2019  IVR-213
        // IVR-213(A/Pennsylvania/1025/2019)
        result.subtype = parts[0];
        result.location = fix_location(parts[1], source, result, messages, message_location);
        result.isolation = fix_isolation(parts[2], source, result, messages, message_location);
        result.year = fix_year(parts[3], source, result, messages, message_location);
        if (type_match(parts[4], part_type::reassortant))
            result.reassortant = fix_reassortant(parts[4], source, result, messages, message_location);
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
            messages.add(Message::invalid_subtype, fmt::format("{}/{}", parts[0].head, parts[1].head), source, message_location);
        }
        result.location = fix_location(parts[2], source, result, messages, message_location);
        result.isolation = fix_isolation(parts[3], source, result, messages, message_location);
        result.year = fix_year(parts[4], source, result, messages, message_location);
    }
    else if (types_match(parts, {part_type::type_subtype, part_type::letters_only, part_type::letters_only, part_type::any, part_type::digits_hyphens})
             || types_match(parts, {part_type::type_subtype, part_type::letters_only, part_type::letters_only, part_type::any, part_type::digits_hyphens, part_type::any})) {
        // with host or double location
        const auto& locdb = locationdb::get();
        if (const auto loc1 = locdb.find(parts[1].head), loc2 = locdb.find(parts[2].head); loc1.empty() && !loc2.empty()) {
            // A(H3N2)/HUMAN/SINGAPORE/INFIMH-16-0019/2016
            result.subtype = parts[0];
            result.host = parts[1];
            result.location = loc2;
            result.isolation = fix_isolation(parts[3], source, result, messages, message_location);
            result.year = fix_year(parts[4], source, result, messages, message_location);
        }
        else if (const auto loc12 = locdb.find(fmt::format("{} {}", parts[1].head, parts[2].head)); !loc12.empty()) {
            // A(H3N2)/LYON/CHU/19/2016
            result.subtype = parts[0];
            result.location = loc12;
            result.isolation = fix_isolation(parts[3], source, result, messages, message_location);
            result.year = fix_year(parts[4], source, result, messages, message_location);
        }
        else if (!loc1.empty() && loc2.empty()) {
            // A(H3N2)/LYON/XXX/19/2016
            result.subtype = parts[0];
            result.location = loc1;
            result.isolation = fmt::format("{}-{}", parts[2].head, std::string{parts[3]});
            result.year = fix_year(parts[4], source, result, messages, message_location);
        }
        else if (!loc2.empty() && parts[1].head == "TURKEY") {
            // A/turkey/Poland/027/2020
            result.subtype = parts[0];
            result.host = parts[1];
            result.location = loc2;
            result.isolation = fix_isolation(parts[3], source, result, messages, message_location);
            result.year = fix_year(parts[4], source, result, messages, message_location);
        }
        else {                  // unrecognized_location
            result.subtype = parts[0];
            result.location = fmt::format("{}/{}", parts[1].head, parts[2].head);
            result.issues.add(Parts::issue::unrecognized_location);
            messages.add(Message::unrecognized_location, result.location, source, message_location);
            result.isolation = fix_isolation(parts[3], source, result, messages, message_location);
            result.year = fix_year(parts[4], source, result, messages, message_location);
        }
        if (!parts[5].empty())
            result.extra = parts[5];
    }
    // IVR-153 (A/CALIFORNIA/07/2009)
    // (H3N2) at the end
    // A/swine/Chachoengsao/2003
    // A/chicken/Iran221/2001
    // A/chicken/Yunnan/Kunming/2007 -> A/chicken/Yunnan Kunming/?/2007
    // extra at the end
    else
        messages.add(Message::unhandled_virus_name, fmt::format("{}", parts), source, message_location);
    return result;
}

// ----------------------------------------------------------------------
