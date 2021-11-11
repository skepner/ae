#include <array>

#include "ext/lexy.hh"
#include "ext/fmt.hh"
#include "virus/passage-parse.hh"

// ======================================================================

namespace ae::virus::passage
{
    namespace conversion
    {
        struct C
        {
            std::string_view trigger;
            std::string_view replacement;
        };

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

        const static std::array table{
            C{"C", "MDCK"},    //
            C{"CELL", "MDCK"}, //
            C{"S", "SIAT"},    //
            C{"EGG", "E"},     //
        };

#pragma GCC diagnostic pop

        inline const std::string_view apply(const std::string& look_for)
        {
            if (const auto found = std::find_if(std::begin(table), std::end(table), [&look_for](const auto& cc) { return cc.trigger == look_for; }); found != std::end(table))
                return found->replacement;
            else if (look_for.size() > 1 && look_for.back() == 'X')
                return std::string_view(look_for.data(), look_for.size() - 1);
            else
                return look_for;
        }

    } // namespace conversion

    namespace grammar
    {
        namespace dsl = lexy::dsl;

        static constexpr auto X = dsl::lit_c<'X'> / dsl::lit_c<'x'>;
        static constexpr auto PLUS = dsl::lit_c<'+'>;
        static constexpr auto OPEN = dsl::lit_c<'('>;
        static constexpr auto CLOSE = dsl::lit_c<')'>;
        static constexpr auto WS = dsl::whitespace(dsl::ascii::space);

        struct passage_name
        {
            static constexpr auto cond = dsl::peek(dsl::ascii::alpha);
            static constexpr auto rule = cond >> dsl::capture(dsl::while_one(dsl::ascii::alpha));
            static constexpr auto value = lexy::callback<std::string>([](auto captured) { return std::string{conversion::apply(std::string(captured.begin(), captured.end()))}; });
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
            static constexpr auto rule = dsl::p<passage_name> + WS + dsl::opt(dsl::p<passage_number>) + WS + dsl::opt(dsl::p<passage_separator>) + WS;
            static constexpr auto value = lexy::callback<std::string>([](const std::string& name, auto number, auto separator) {
                std::string result{name};
                if constexpr (!std::is_same_v<decltype(number), lexy::nullopt>)
                    result.append(number.begin(), number.end());
                else
                    result.append(1, '?');
                if constexpr (!std::is_same_v<decltype(separator), lexy::nullopt>)
                    result.append(1, '/');
                return result;
            });
        };

        struct passages
        {
            static constexpr auto rule = dsl::list(passage_name::cond >> dsl::p<part>);
            static constexpr auto value = lexy::as_string<std::string>;
        };

        struct whole
        {
            static constexpr auto rule = WS + dsl::p<passages> + dsl::opt(OPEN >> dsl::p<passage_date>) + dsl::eof;
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

    return parsing_result.value();

} // ae::virus::passage::parse

// ----------------------------------------------------------------------
