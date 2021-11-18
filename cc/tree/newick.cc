#include "tree/newick.hh"
#include "ext/lexy.hh"

// https://en.wikipedia.org/wiki/Newick_format

// ======================================================================

namespace ae::tree::newick
{
    namespace grammar
    {
        namespace dsl = lexy::dsl;

        static constexpr auto WS = dsl::whitespace(dsl::ascii::space);
        static constexpr auto OPEN = dsl::lit_c<'('>;
        static constexpr auto CLOSE = dsl::lit_c<')'>;
        static constexpr auto PLUS = dsl::lit_c<'+'>;
        static constexpr auto E = dsl::lit_c<'E'> / dsl::lit_c<'e'>;

        class invalid_input : public std::runtime_error
        {
          public:
            using std::runtime_error::runtime_error;
        };

        using tree_t = std::shared_ptr<ae::tree::Tree>;

        struct number : lexy::token_production
        {
            static constexpr auto rule = dsl::peek(dsl::lit_c<'-'> / dsl::digit<>) >> dsl::capture(dsl::while_(dsl::hyphen / PLUS / dsl::period / dsl::digit<> / E));
            static constexpr auto value = lexy::callback<double>([](const std::string& data) { return std::stod(data); });
        };

        struct length
        {
            static constexpr auto whitespace = dsl::ascii::space / dsl::ascii::newline;
            static constexpr auto rule = dsl::opt(dsl::colon >> dsl::p<number>);
            static constexpr auto value = lexy::callback<double>([](double dd) { return dd; }, [](lexy::nullopt) { return 0.0; });
        };

        struct name
        {
            static constexpr auto whitespace = dsl::ascii::space / dsl::ascii::newline;
            static constexpr auto lit_not = OPEN /CLOSE / dsl::semicolon / dsl::comma / dsl::colon;
            static constexpr auto rule = dsl::capture(dsl::while_(dsl::code_point - lit_not - whitespace));
        };

        struct internal_node;

        struct subtree
        {
            static constexpr auto whitespace = dsl::ascii::space / dsl::ascii::newline;
            static constexpr auto rule = (dsl::peek(OPEN) >> dsl::recurse<internal_node> | dsl::else_ >> dsl::p<name>);
            // static constexpr auto value = lexy::callback<tree_t>();
        };

        struct branch_set
        {
            static constexpr auto whitespace = dsl::ascii::space / dsl::ascii::newline;
            static constexpr auto rule = dsl::list(dsl::p<subtree> + dsl::p<length>, dsl::sep(dsl::comma));
        };

        struct internal_node
        {
            static constexpr auto whitespace = dsl::ascii::space / dsl::ascii::newline;
            static constexpr auto rule = OPEN + dsl::p<branch_set> + CLOSE + dsl::p<name>;
        };

        struct tree
        {
            static constexpr auto whitespace = dsl::ascii::space / dsl::ascii::newline;
            static constexpr auto rule = dsl::p<internal_node> + dsl::semicolon + dsl::eof;

            static constexpr auto value = lexy::forward<tree_t>;
        };

    } // namespace grammar

    // ----------------------------------------------------------------------

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

    };

} // namespace ae::tree

// ----------------------------------------------------------------------

std::shared_ptr<ae::tree::Tree> ae::tree::load_newick(const std::string& source)
{
    const bool trace { false };

   if (trace) {
        fmt::print(">>> parsing newick\n");
        lexy::trace<newick::grammar::tree>(stderr, lexy::string_input<lexy::utf8_encoding>{source});
    }

    try {
        const auto parsing_result = lexy::parse<newick::grammar::tree>(lexy::string_input<lexy::utf8_encoding>{source}, newick::report_error{});
        return parsing_result.value();
    }
    catch (newick::grammar::invalid_input& err) {
        fmt::print(">> newick parsing error: {}\n", err.what());
        return nullptr;
    }

} // ae::tree::load_newick

// ----------------------------------------------------------------------

void ae::tree::export_newick(const Tree& tree, const std::filesystem::path& filename)
{

} // ae::tree::export_newick

// ----------------------------------------------------------------------
