#include <array>
#include <vector>
#include "virus/name-parse.hh"
#include "ext/fmt.hh"
#include "ext/lexy.hh"

// ======================================================================

namespace ae::virus::name::inline v1
{

    struct part_t
    {
        enum type { undef, subtype, rest }; //, alpha, alpha_digits, digits_alpha, digits, other };

        std::string text{};
        enum type type { undef };
        int _padding{0}; // avoid warning
    };

    using parts_t = std::array<part_t, 8>;

    namespace grammar
    {
        namespace dsl = lexy::dsl;

        template <typename T> inline std::string to_string(const lexy::lexeme<T>& lexeme) { return {lexeme.begin(), lexeme.end()}; }

        // ----------------------------------------------------------------------

        struct subtype_a
        {
            static constexpr auto A = dsl::lit_c<'A'>;
            static constexpr auto H = dsl::lit_c<'H'>;
            static constexpr auto N = dsl::lit_c<'N'>;
            static constexpr auto OPEN = dsl::lit_c<'('>;
            static constexpr auto CLOSE = dsl::lit_c<')'>;

            // H3N2 | H3
            struct hn
            {
                static constexpr auto rule = dsl::peek(H) >> dsl::capture(H + dsl::digits<>) + dsl::opt(dsl::peek(N) >> dsl::capture(N + dsl::digits<>));
                static constexpr auto value = lexy::callback<std::string>( //
                    [](auto lex1, lexy::nullopt) { return to_string(lex1); },
                    [](auto lex1, auto lex2) {
                        return std::string{lex1.begin(), lex2.end()};
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
            static constexpr auto B = dsl::lit_c<'B'>;

            static constexpr auto rule = B;
            static constexpr auto value = lexy::callback<part_t>([]() { return part_t{"B", part_t::subtype}; });
        };

        // ----------------------------------------------------------------------

        struct rest
        {
            static constexpr auto rule = dsl::capture(dsl::any);
            static constexpr auto value = lexy::callback<part_t>([](auto lexeme) { return part_t{to_string(lexeme), part_t::rest}; });
        };

        // ----------------------------------------------------------------------

        struct parts
        {
            static constexpr auto whitespace = dsl::ascii::blank; // auto skip whitespaces
            static constexpr auto rule =
                (dsl::p<subtype_a> | dsl::p<subtype_b>)
                + dsl::slash
                // + dsl::p<tail_parts>
                + dsl::p<rest>
                + dsl::eof;

            static constexpr auto value = lexy::callback<parts_t>([](auto subtype, auto rest) {
                parts_t parts{subtype, rest};
                // std::move(std::begin(rest), std::end(rest), std::next(std::begin(parts)));
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
            // case part_t::alpha:
            //     return format_to(ctx.out(), "alpha");
            // case part_t::alpha_digits:
            //     return format_to(ctx.out(), "alpha_digits");
            // case part_t::digits_alpha:
            //     return format_to(ctx.out(), "digits_alpha");
            // case part_t::digits:
            //     return format_to(ctx.out(), "digits");
            // case part_t::other:
            //     return format_to(ctx.out(), "other");
        }
        return ctx.out();
    }
};

template <> struct fmt::formatter<ae::virus::name::part_t> : fmt::formatter<eu::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const ae::virus::name::part_t& value, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "<{}>\"{}\"", value.type, value.text);
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


// ----------------------------------------------------------------------
