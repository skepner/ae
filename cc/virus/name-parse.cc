#include <array>
#include <vector>
#include <cctype>
#include <algorithm>

#include "virus/name-parse.hh"
#include "ext/fmt.hh"
#include "ext/lexy.hh"
#include "locdb/locdb.hh"

// ======================================================================

namespace ae::virus::name::inline v1
{

    template <class T> concept Lexeme = requires(T a) {
        { a.begin() };
        { a.end() };
    };

    template <typename Iter> inline std::string uppercase_strip(Iter first, Iter last) {
        while (first != last && std::isspace(*first)) ++first; // remove leading spaces
        std::string res(static_cast<size_t>(last - first), '?');
        auto output_end = std::transform(first, last, res.begin(), [](unsigned char cc) { return std::toupper(cc); });
        if ((output_end - res.begin()) > 1) { // output is not empty
            --output_end;
            while (output_end != res.begin() && std::isspace(*output_end)) --output_end; // remove trailing spaces
            res.resize(static_cast<size_t>(output_end - res.begin()) + 1);
        }
        return res;
    }

    template <Lexeme Lex> inline std::string uppercase_strip(Lex&& lexeme) { return uppercase_strip(lexeme.begin(), lexeme.end()); }

    struct part_t
    {
        enum type { undef, subtype, letters, letter_mixed, digits, digit_mixed, rest };

        std::string head{};
        std::string tail{};
        enum type type { undef };
        int _padding{0}; // avoid warning

        part_t() = default;
        part_t(const part_t&) = default;
        part_t(part_t&&) = default;
        part_t(std::string&& text, enum type typ) : head{std::move(text)}, type{typ} {}
        part_t(const char* text, enum type typ) : head{text}, type{typ} {}
        part_t(enum type typ) : type{typ} {}
        template <Lexeme Lex> part_t(Lex&& lexeme, enum type typ) : head{uppercase_strip(lexeme)}, type{typ} {}
        template <Lexeme Lex> part_t(Lex&& lex1, Lex&& lex2, enum type typ) : head{uppercase_strip(lex1)}, tail{uppercase_strip(lex2)}, type{typ} {}
        part_t& operator=(const part_t&) = default;
        part_t& operator=(part_t&&) = default;
        constexpr bool empty() const { return head.empty(); }
    };

    using parts_t = std::array<part_t, 8>;

    inline bool types_match(const parts_t& p1, const parts_t& p2)
    {
        return std::equal(p1.begin(), p1.end(), p2.begin(), [](const auto& e1, const auto& e2) { return e1.type == e2.type; });
    }

    // ======================================================================

    namespace grammar
    {
        namespace dsl = lexy::dsl;

        // ----------------------------------------------------------------------

        struct letter_extra_predicate
        {
            constexpr bool operator()(lexy::code_point cp) { //
                return //
                    (cp.value() >= 0xC0 && cp.value() <= 0xFF) // Latin-1 Supplement (including math x and math division symbol)
                    || (cp.value() >= 0x100 && cp.value() <= 0x17F) // Latin Extended-A
                    // cyrillic ?
                    || (cp.value() >= 0x4E00 && cp.value() <= 0x9FFF) // CJK Unified Ideographs
                    ;
            }
        };

        static constexpr auto letter_extra = dsl::code_point.if_<letter_extra_predicate>();

        // ----------------------------------------------------------------------

        struct subtype_a
        {
            static constexpr auto A = dsl::lit_c<'A'> / dsl::lit_c<'a'>;
            static constexpr auto H = dsl::lit_c<'H'> / dsl::lit_c<'h'>;
            static constexpr auto N = dsl::lit_c<'N'> / dsl::lit_c<'n'>;
            static constexpr auto OPEN = dsl::lit_c<'('>;
            static constexpr auto CLOSE = dsl::lit_c<')'>;

            // H3N2 | H3
            struct hn
            {
                static constexpr auto rule = dsl::peek(H) >> dsl::capture(H + dsl::digits<>) + dsl::opt(dsl::peek(N) >> dsl::capture(N + dsl::digits<>));
                static constexpr auto value = lexy::callback<std::string>([](auto lex1, auto lex2) {
                    if constexpr (std::is_same_v<decltype(lex2), lexy::nullopt>)
                        return uppercase_strip(lex1);
                    else
                        return uppercase_strip(lex1.begin(), lex2.end());
                });
            };

            // A | AH3 | AH3N2 | A(H3N2) | A(H3)
            static constexpr auto rule = A >> dsl::opt(dsl::p<hn> | OPEN >> dsl::p<hn> + CLOSE);
            static constexpr auto value = lexy::callback<part_t>( //
                [](lexy::nullopt) {
                    return part_t{"A", part_t::subtype};
                }, //
                [](const std::string& a1) {
                    return part_t{fmt::format("A({})", a1), part_t::subtype};
                });
        };

        struct subtype_b
        {
            static constexpr auto B = dsl::lit_c<'B'> / dsl::lit_c<'b'>;

            static constexpr auto rule = B;
            static constexpr auto value = lexy::callback<part_t>([]() { return part_t{"B", part_t::subtype}; });
        };

        // ----------------------------------------------------------------------

        // chunk starting with a letter, followed by letters, digits, -, _, :, space
        struct letters
        {
            static constexpr auto whitespace = dsl::ascii::blank; // auto skip whitespaces
            static constexpr auto letters_only = dsl::ascii::alpha / letter_extra / dsl::lit_c<'_'> / dsl::hyphen / dsl::ascii::blank;
            static constexpr auto mixed = letters_only / dsl::ascii::digit / dsl::colon;

            static constexpr auto rule = dsl::peek(dsl::ascii::alpha / letter_extra) >> dsl::capture(dsl::while_(letters_only)) + dsl::opt(dsl::peek_not(dsl::lit_c<'/'>) >> dsl::capture(dsl::while_(mixed)));
            static constexpr auto value = lexy::callback<part_t>([](auto lex1, auto lex2) {
                if constexpr (std::is_same_v<decltype(lex2), lexy::nullopt>)
                    return part_t{lex1, part_t::letters};
                else
                    return part_t{lex1, lex2, part_t::letter_mixed};
            });
        };

        // chunk starting with a digit, followed by letters, digits, -, _, :, (BUT NO space)
        struct digits
        {
            static constexpr auto mixed = dsl::digits<> / dsl::lit_c<'_'> / dsl::hyphen / dsl::ascii::digit / dsl::colon; // NO blank!!

            static constexpr auto rule = dsl::peek(dsl::digit<>) >> dsl::capture(dsl::while_(dsl::digits<>)) + dsl::opt(dsl::peek(mixed) >> dsl::capture(dsl::while_(mixed)));
            static constexpr auto value = lexy::callback<part_t>([](auto lex1, auto lex2) {
                if constexpr (std::is_same_v<decltype(lex2), lexy::nullopt>)
                    return part_t{lex1, part_t::digits};
                else
                    return part_t{lex1, lex2, part_t::digit_mixed};
            });
        };

        struct slash_separated
        {
            static constexpr auto rule = dsl::list(dsl::p<letters> | dsl::p<digits>, dsl::sep(dsl::slash));
            static constexpr auto value = lexy::as_list<std::vector<part_t>>;
        };

        // ----------------------------------------------------------------------

        struct rest
        {
            static constexpr auto rule = dsl::capture(dsl::any);
            static constexpr auto value = lexy::callback<part_t>([](auto lexeme) { return part_t{lexeme, part_t::rest}; });
        };

        // ----------------------------------------------------------------------

        struct parts
        {
            static constexpr auto whitespace = dsl::ascii::blank; // auto skip whitespaces
            static constexpr auto rule =
                (dsl::p<subtype_a> | dsl::p<subtype_b>)
                + dsl::slash
                + dsl::p<slash_separated>
                + dsl::p<rest>
                + dsl::eof;

            static constexpr auto value = lexy::callback<parts_t>([](auto subtype, auto slash_separated, auto rest) {
                parts_t parts{subtype};
                const auto last_output = std::move(std::begin(slash_separated), std::end(slash_separated), std::next(std::begin(parts)));
                if (!rest.empty())
                    *last_output = rest;
                return parts;
            });
        };

    } // namespace grammar
} // namespace ae::virus::name::inline v1

// ----------------------------------------------------------------------

template <> struct fmt::formatter<enum ae::virus::name::part_t::type> : fmt::formatter<eu::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const enum ae::virus::name::part_t::type& value, FormatCtx& ctx)
    {
        using namespace ae::virus::name;
        switch (value) {
            case part_t::undef:
                return format_to(ctx.out(), "undef");
            case part_t::subtype:
                return format_to(ctx.out(), "subtype");
            case part_t::rest:
                return format_to(ctx.out(), "rest");
            case part_t::letters:
                return format_to(ctx.out(), "letters");
            case part_t::letter_mixed:
                return format_to(ctx.out(), "letter_mixed");
            case part_t::digits:
                return format_to(ctx.out(), "digits");
            case part_t::digit_mixed:
                return format_to(ctx.out(), "digit_mixed");
        }
        return ctx.out();
    }
};

template <> struct fmt::formatter<ae::virus::name::part_t> : fmt::formatter<eu::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const ae::virus::name::part_t& value, FormatCtx& ctx)
    {
        if (value.type == ae::virus::name::part_t::undef)
            return ctx.out();
        else
            return format_to(ctx.out(), "<{}>\"{}{}\"", value.type, value.head, value.tail);
    }
};

// ======================================================================

ae::virus::name::v1::Parts ae::virus::name::v1::parse(std::string_view source, parse_tracing tracing)
{
    fmt::print(">>> parsing \"{}\"\n", source);
    if (tracing == parse_tracing::yes)
        lexy::trace<grammar::parts>(stderr, lexy::string_input<lexy::utf8_encoding>{source});
    const auto result = lexy::parse<grammar::parts>(lexy::string_input<lexy::utf8_encoding>{source}, lexy_ext::report_error);
    fmt::print("    {}\n", result.value());
    const auto& locdb = ae::locationdb::get();
    const auto parts = result.value();
    if (types_match(parts, parts_t{part_t::subtype, part_t::letters, part_t::letter_mixed, part_t::digits}) || types_match(parts, parts_t{part_t::subtype, part_t::letters, part_t::digits, part_t::digits})) {
        const auto new_loc = locdb.find(parts[1].head);
        fmt::print("  A/LOC/ISO/YEAR  \"{}\" -> \"{}\"\n", parts[1].head, new_loc);
    }
    return {};
}

// ----------------------------------------------------------------------
