#include "ext/lexy.hh"
#include "virus/passage-parse.hh"

// ======================================================================

namespace ae::virus::passage
{
    namespace grammar
    {
        namespace dsl = lexy::dsl;

        static constexpr auto X = dsl::lit_c<'X'> / dsl::lit_c<'x'>;
        static constexpr auto PLUS = dsl::lit_c<'+'>;

        struct passage_name
        {
            static constexpr auto rule = dsl::capture(dsl::while_(dsl::ascii::alpha));
            static constexpr auto value = lexy::as_string<std::string>;
        };

        struct passage_number
        {
            static constexpr auto rule = dsl::capture(dsl::while_(dsl::digit<> / X / dsl::question_mark));
            static constexpr auto value = lexy::as_string<std::string>;
        };

        struct passage_separator
        {
            static constexpr auto rule = dsl::capture(dsl::while_(dsl::slash / dsl::comma / PLUS));
            static constexpr auto value = lexy::as_string<std::string>;
        };

        struct part
        {
            static constexpr auto rule = dsl::p<passage_name> + dsl::opt(dsl::p<passage_number>) + dsl::opt(dsl::p<passage_separator>);
            static constexpr auto value = lexy::callback<std::string>(
                [](const std::string& name, auto number, auto separator) {
                return name;
                });
        };

        struct whole
        {
            static constexpr auto rule = dsl::list(dsl::p<part>) + dsl::eof;
            static constexpr auto value = lexy::as_string<std::string>;
        };
    }


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
