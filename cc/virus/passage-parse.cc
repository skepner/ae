#include "ext/lexy.hh"
#include "ext/fmt.hh"
#include "virus/passage-parse.hh"

// ======================================================================

namespace ae::virus::passage
{
    namespace grammar
    {
        namespace dsl = lexy::dsl;

        static constexpr auto X = dsl::lit_c<'X'> / dsl::lit_c<'x'>;
        static constexpr auto PLUS = dsl::lit_c<'+'>;
        static constexpr auto OPEN = dsl::lit_c<'('>;
        static constexpr auto CLOSE = dsl::lit_c<')'>;
        static constexpr auto WS = dsl::while_(dsl::ascii::blank);

        struct passage_name
        {
            static constexpr auto cond = dsl::peek(dsl::ascii::alpha);
            static constexpr auto rule = cond >> dsl::capture(dsl::while_(dsl::ascii::alpha));
            static constexpr auto value = lexy::as_string<std::string>;
        };

        struct passage_number
        {
            static constexpr auto symbol = dsl::digit<> / X / dsl::question_mark;
            static constexpr auto rule = dsl::peek(symbol) >> dsl::capture(dsl::while_(symbol));
            static constexpr auto value = lexy::as_string<std::string>;
        };

        struct passage_separator
        {
            static constexpr auto symbol = dsl::slash / dsl::comma / PLUS;
            static constexpr auto rule = symbol >> dsl::capture(dsl::while_(symbol));
            static constexpr auto value = lexy::as_string<std::string>;
        };

        struct passage_date
        {
            static constexpr auto rule = dsl::capture(dsl::while_(OPEN / CLOSE / dsl::digit<> / dsl::hyphen / dsl::slash));
            static constexpr auto value = lexy::as_string<std::string>;
        };

        struct part
        {
            static constexpr auto rule = dsl::p<passage_name> + WS + dsl::opt(dsl::p<passage_number>) + WS + dsl::opt(dsl::p<passage_separator>);
            static constexpr auto value = lexy::callback<std::string>(
                [](const std::string& name, auto number, auto separator) {
                return name;
                });
        };

        struct passages
        {
            static constexpr auto rule = dsl::list(passage_name::cond >> dsl::p<part>);
            static constexpr auto value = lexy::as_string<std::string>;
        };

        struct whole
        {
            static constexpr auto rule = WS + dsl::p<passages> + WS + dsl::opt(OPEN >> dsl::p<passage_date>) + dsl::eof;
            static constexpr auto value = lexy::callback<std::string>([](const std::string& passages, const std::string& date) { return fmt::format("{} {}", passages, date); },
                                                                      [](const std::string& passages, lexy::nullopt) { return passages; });
        };

    } // namespace grammar

} // namespace ae::virus::passage

// ======================================================================

std::string ae::virus::passage::parse(std::string_view source, parse_settings& settings, Messages& messages, const MessageLocation& location)
{
    if (settings.trace()) {
        fmt::print(">>> parsing \"{}\"\n", source);
        lexy::trace<grammar::whole>(stderr, lexy::string_input<lexy::utf8_encoding>{source});
    }

    const auto parsing_result = lexy::parse<grammar::whole>(lexy::string_input<lexy::utf8_encoding>{source}, lexy_ext::report_error);

    return std::string{source};

} // ae::virus::passage::parse

// ----------------------------------------------------------------------
