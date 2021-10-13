#include <array>
#include <vector>
#include "virus/name-parse.hh"
#include "ext/fmt.hh"
#include "ext/lexy.hh"

// ======================================================================

namespace ae::virus::name::inline v1
{

    template <class T> concept Lexeme = requires(T a) {
        { a.begin() };
        { a.end() };
    };

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
        template <Lexeme Lex> part_t(Lex&& lexeme, enum type typ) : head{lexeme.begin(), lexeme.end()}, type{typ} {}
        template <Lexeme Lex> part_t(Lex&& lex1, Lex&& lex2, enum type typ) : head{lex1.begin(), lex1.end()}, tail{lex2.begin(), lex2.end()}, type{typ} {}
        part_t& operator=(const part_t&) = default;
        part_t& operator=(part_t&&) = default;
        constexpr bool empty() const { return head.empty(); }
    };

    using parts_t = std::array<part_t, 8>;

    namespace grammar
    {
        namespace dsl = lexy::dsl;

        // template <typename T> inline std::string to_string(const lexy::lexeme<T>& lexeme) { return {lexeme.begin(), lexeme.end()}; }

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
                        return std::string{lex1.begin(), lex1.end()};
                    else
                        return std::string{lex1.begin(), lex2.end()};
                });
                // [](auto lex1, lexy::nullopt) { return std::string{lex1.begin(), lex1.end()}; },
                // [](auto lex1, auto lex2) {
                //     return std::string{lex1.begin(), lex2.end()};
                // });
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
            static constexpr auto letters_only = dsl::ascii::alpha / dsl::lit_c<'_'> / dsl::hyphen / dsl::ascii::blank;
            static constexpr auto mixed = letters_only / dsl::ascii::digit / dsl::colon;

            static constexpr auto rule = dsl::peek(dsl::ascii::alpha) >> dsl::capture(dsl::while_(letters_only)) + dsl::opt(dsl::peek_not(dsl::lit_c<'/'>) >> dsl::capture(dsl::while_(mixed)));
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
        lexy::trace<grammar::parts>(stderr, lexy::string_input{source});
    const auto result = lexy::parse<grammar::parts>(lexy::string_input{source}, lexy_ext::report_error);
    fmt::print("    {}\n", result.value());
    return {};
}

// ----------------------------------------------------------------------
