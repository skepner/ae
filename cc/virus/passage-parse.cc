#include <array>
#include <stdexcept>

#include "ext/lexy.hh"
#include "utils/named-type.hh"
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
            C{"MDKC", "MDCK"}, //
            C{"S", "SIAT"},    //
            C{"EGG", "E"},     //
            C{"ORIGINAL", "OR"},     //
        };

#pragma GCC diagnostic pop

        inline const std::string apply(const std::string& look_for)
        {
            if (const auto found = std::find_if(std::begin(table), std::end(table), [&look_for](const auto& cc) { return cc.trigger == look_for; }); found != std::end(table))
                return std::string{found->replacement};
            else if (look_for.size() > 1 && (look_for.back() == 'X' || look_for.back() == 'x'))
                return look_for.substr(0, look_for.size() - 1);
            else
                return look_for;
        }

    } // namespace conversion

    namespace grammar
    {
        namespace dsl = lexy::dsl;

        class invalid_input : public std::runtime_error { public: using std::runtime_error::runtime_error; };

        static constexpr auto A = dsl::lit_c<'A'> / dsl::lit_c<'a'>;
        // static constexpr auto B = dsl::lit_c<'B'> / dsl::lit_c<'b'>;
        static constexpr auto C = dsl::lit_c<'C'> / dsl::lit_c<'c'>;
        static constexpr auto D = dsl::lit_c<'D'> / dsl::lit_c<'d'>;
        static constexpr auto E = dsl::lit_c<'E'> / dsl::lit_c<'e'>;
        // static constexpr auto F = dsl::lit_c<'F'> / dsl::lit_c<'f'>;
        static constexpr auto G = dsl::lit_c<'G'> / dsl::lit_c<'g'>;
        // static constexpr auto H = dsl::lit_c<'H'> / dsl::lit_c<'h'>;
        static constexpr auto I = dsl::lit_c<'I'> / dsl::lit_c<'i'>;
        // static constexpr auto J = dsl::lit_c<'J'> / dsl::lit_c<'j'>;
        // static constexpr auto K = dsl::lit_c<'K'> / dsl::lit_c<'k'>;
        static constexpr auto L = dsl::lit_c<'L'> / dsl::lit_c<'l'>;
        // static constexpr auto M = dsl::lit_c<'M'> / dsl::lit_c<'m'>;
        // static constexpr auto N = dsl::lit_c<'N'> / dsl::lit_c<'n'>;
        static constexpr auto O = dsl::lit_c<'O'> / dsl::lit_c<'o'>;
        static constexpr auto P = dsl::lit_c<'P'> / dsl::lit_c<'p'>;
        // static constexpr auto Q = dsl::lit_c<'Q'> / dsl::lit_c<'q'>;
        static constexpr auto R = dsl::lit_c<'R'> / dsl::lit_c<'r'>;
        static constexpr auto S = dsl::lit_c<'S'> / dsl::lit_c<'s'>;
        static constexpr auto T = dsl::lit_c<'T'> / dsl::lit_c<'t'>;
        // static constexpr auto U = dsl::lit_c<'U'> / dsl::lit_c<'u'>;
        // static constexpr auto V = dsl::lit_c<'V'> / dsl::lit_c<'v'>;
        // static constexpr auto W = dsl::lit_c<'W'> / dsl::lit_c<'w'>;
        static constexpr auto X = dsl::lit_c<'X'> / dsl::lit_c<'x'>;
        // static constexpr auto Y = dsl::lit_c<'Y'> / dsl::lit_c<'y'>;
        // static constexpr auto Z = dsl::lit_c<'Z'> / dsl::lit_c<'z'>;

        static constexpr auto PLUS = dsl::lit_c<'+'>;
        static constexpr auto OPEN = dsl::lit_c<'('>;
        static constexpr auto CLOSE = dsl::lit_c<')'>;
        static constexpr auto WS = dsl::whitespace(dsl::ascii::space);

        struct passage_or_cs
        {
            static constexpr auto just_or = O + R;
            static constexpr auto ori = just_or + I;
            static constexpr auto just_cs = C + S;
            static constexpr auto cli = C + L + I;
            static constexpr auto cond = dsl::peek(just_or) | dsl::peek(just_cs) | dsl::peek(cli);

            static constexpr auto rule = //
                (dsl::peek(ori) >> ori + dsl::any) //
                | (dsl::peek(just_or) >> just_or) //
                | (dsl::peek(cli) >> cli + dsl::any) //
                | (dsl::peek(just_cs) >> just_cs) //
                ;

            static constexpr auto value = lexy::callback<std::string>([]() { return "OR"; });
        };

        struct passage_name
        {
            static constexpr auto cond = dsl::peek(dsl::ascii::alpha);
            static constexpr auto rule = dsl::capture(dsl::while_one(dsl::ascii::alpha));
            static constexpr auto value = lexy::callback<std::string>([](auto captured) {
                if ((captured.end() - captured.begin()) > 5)
                    throw invalid_input{"passage name too long"};
                return std::string{conversion::apply(string::uppercase(captured.begin(), captured.end()))};
            });
        };

        struct passage_number
        {
            static constexpr auto symbol = dsl::digit<> / X / dsl::question_mark;
            static constexpr auto cond = dsl::peek(dsl::digit<>);
            static constexpr auto rule = dsl::peek(symbol) >> dsl::capture(dsl::while_(symbol));
            static constexpr auto value = lexy::as_string<std::string>;
        };

        struct passage_separator
        {
            static constexpr auto symbol = dsl::slash / dsl::backslash / dsl::comma / PLUS;
            static constexpr auto rule = symbol >> dsl::capture(dsl::while_(symbol));
            static constexpr auto value = lexy::as_string<std::string>;
        };

        struct passage_date
        {
            static constexpr auto rule = dsl::capture(dsl::while_(dsl::digit<> / dsl::hyphen / dsl::slash)) + dsl::if_(dsl::peek(CLOSE) >> CLOSE);
            static constexpr auto value = lexy::as_string<std::string>;
        };

        struct passage_prefix
        {
            static constexpr auto prefix = P + A + S + S + A + G + E;
            static constexpr auto prefix_details = D + E + T + A + I + L + S;
            static constexpr auto cond = dsl::peek(prefix);
            static constexpr auto rule = prefix + WS + dsl::opt(dsl::peek(prefix_details) >> prefix_details + WS) + dsl::colon + WS;
            static constexpr auto value = lexy::callback<lexy::nullopt>([](lexy::nullopt) { return lexy::nullopt{}; }, []() { return lexy::nullopt{}; });
        };

        struct part
        {
            static constexpr auto rule = //
                dsl::opt(passage_prefix::cond >> dsl::p<passage_prefix>) +
                ((passage_or_cs::cond >> dsl::p<passage_or_cs> + dsl::nullopt + dsl::nullopt) // nullopt to match value callback arg list
                 | (passage_name::cond >> dsl::p<passage_name> + WS + dsl::opt(dsl::p<passage_number>) + WS + dsl::opt(dsl::p<passage_separator>) + WS));

            static constexpr auto value = lexy::callback<passage_deconstructed_t::element_t>([](lexy::nullopt, const std::string& name, auto number, auto separator) {
                passage_deconstructed_t::element_t result{.name = name};
                if constexpr (!std::is_same_v<decltype(number), lexy::nullopt>)
                    result.count.append(number.begin(), number.end());
                else if (name != "OR" && name != "CS")
                    result.count.append(1, '?');
                if constexpr (!std::is_same_v<decltype(separator), lexy::nullopt>)
                    result.new_lab = true;
                return result;
            });
        };

        using part_without_name_t = named_t<passage_deconstructed_t::element_t, struct part_without_name_t_tag>;

        struct part_without_name
        {
            static constexpr auto rule = dsl::p<passage_number> + WS + dsl::opt(dsl::p<passage_separator>) + WS;

            static constexpr auto value = lexy::callback<part_without_name_t>([](const std::string& number, auto separator) {
                passage_deconstructed_t::element_t result{.count = number};
                if constexpr (!std::is_same_v<decltype(separator), lexy::nullopt>)
                    result.new_lab = true;
                return result;
            });
        };

        struct passages
        {
            static constexpr auto rule = dsl::list((passage_name::cond >> dsl::p<part>) | (passage_number::cond >> dsl::p<part_without_name>));

            static constexpr auto value = lexy::fold_inplace<passage_deconstructed_t>(0, [](passage_deconstructed_t& target, const auto& val) {
                if constexpr (std::is_same_v<decltype(val), const part_without_name_t&>) {
                    if (!target.elements.empty())
                        target.elements.push_back({.name = target.elements.back().name, .count = val->count, .new_lab = val->new_lab});
                    // else
                    //     fmt::print("> ae::virus::passage::grammar::passages: adding part_without_name_t to an empty passage\n");
                }
                else {
                    target.elements.push_back(val);
                }
                return target;
            });

        };

        struct whole
        {
            // static constexpr auto rule = WS + dsl::try_(dsl::p<passages> + dsl::opt(OPEN >> dsl::p<passage_date>), dsl::nullopt) + dsl::eof;
            static constexpr auto rule = WS + dsl::try_(dsl::p<passages> + dsl::opt(OPEN >> dsl::p<passage_date>), dsl::recover(dsl::eof)) + dsl::eof;

            static constexpr auto value = lexy::callback<passage_deconstructed_t>(
                [](const passage_deconstructed_t& passages, const std::string& date) {
                    return passage_deconstructed_t{passages.elements, date};
                },                                                                               //
                [](const passage_deconstructed_t& passages, lexy::nullopt) { return passages; }, //
                [](lexy::nullopt) { return passage_deconstructed_t{}; },                          //
                []() { return passage_deconstructed_t{}; }                          //
            );
        };

    } // namespace grammar

    struct report_error
    {
        struct error_sink
        {
            // std::size_t _count;
            using return_type = std::size_t;

            template <typename Production, typename Input, typename Reader, typename Tag>
            void operator()(const lexy::error_context<Production, Input>& /*context*/, const lexy::error<Reader, Tag>& /*error*/)
            {
                // _detail::write_error(lexy::cfile_output_iterator{stderr}, context, error, {lexy::visualize_fancy});
                // ++_count;
            }

            return_type finish() &&
            {
                // if (_count != 0)
                //     std::fputs("\n", stderr);
                // return _count;
                return 0;
            }

            const report_error& report_error_;
        };

        constexpr auto sink() const { return error_sink{*this}; }

        Messages& messages;
        const MessageLocation& location;
    };

} // namespace ae::virus::passage

// ======================================================================

ae::virus::passage::passage_deconstructed_t ae::virus::passage::parse(std::string_view source, parse_settings& settings, Messages& messages, const MessageLocation& location)
{
    // fmt::print(">>> parsing \"{}\"\n", source);
    if (settings.trace()) {
        fmt::print(">>> parsing \"{}\"\n", source);
        lexy::trace<grammar::whole>(stderr, lexy::string_input<lexy::utf8_encoding>{source});
    }

    try {
        const auto parsing_result = lexy::parse<grammar::whole>(lexy::string_input<lexy::utf8_encoding>{source}, report_error{messages, location});
        if (parsing_result.value().empty())
            throw grammar::invalid_input{"empty result"};
        return parsing_result.value();
    }
    catch (grammar::invalid_input& err) {
        // fmt::print(">> not parsed: {} <- \"{}\"\n", err.what(), source);
        // !!! add message
        passage_deconstructed_t result;
        result.elements.push_back(passage_deconstructed_t::element_t{.name = fmt::format("*{}", source)});
        return result;
    }

} // ae::virus::passage::parse

// ----------------------------------------------------------------------
