#include <vector>

#include "tree/newick.hh"
#include "ext/lexy.hh"

// https://en.wikipedia.org/wiki/Newick_format

// ======================================================================

namespace ae::tree::newick
{
    struct result_t
    {
        result_t() { fmt::print(">>>> result_t{{}}\n"); }
        result_t(int ii) { fmt::print(">>>> result_t{{{}}}\n", ii); }
        result_t(std::initializer_list<result_t>) { fmt::print(">>>> result_t{{initializer_list}}\n"); }
        result_t(const char* text) { fmt::print(">>>> result_t{{\"{}\"}}\n", text); }
        result_t(const result_t&) { fmt::print(">>>> result_t(copy)\n"); }
        result_t(result_t&&) { fmt::print(">>>> result_t(move)\n"); }
        result_t& operator=(const result_t&) { fmt::print(">>>> =(copy)\n"); return *this; }
        result_t& operator=(result_t&&) { fmt::print(">>>> =(move)\n"); return *this; }
    };

    struct state_t
    {
        state_t() = default;
        // template <typename ... Args> state_t& self(Args&& ... args) {
        //     fmt::print(">>>> state_t::self {}\n", sizeof...(args));
        //     return *this;
        // }
        // state_t& self() { fmt::print(">>>> state_t::self\n"); return *this; }
        result_t self() const { fmt::print(">>>> state_t::self()\n");
            return result_t {};
        }
        result_t self(const result_t& res) const { fmt::print(">>>> state_t::self(const result_t&)\n"); return res; }
    };

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

        struct number
        {
            static constexpr auto rule = dsl::peek(dsl::lit_c<'-'> / dsl::digit<>) >> dsl::capture(dsl::while_(dsl::hyphen / PLUS / dsl::period / dsl::digit<> / E));
            static constexpr auto value = lexy::callback<double>( //
                [](const std::string& data) { return std::stod(data); }, //
                [](auto lex) { fmt::print(">>>> number(auto) {}\n", std::string(lex.begin(), lex.end())); return -1967; }//
            );
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
            static constexpr auto lit_not = OPEN / CLOSE / dsl::semicolon / dsl::comma / dsl::colon;
            static constexpr auto rule = dsl::capture(dsl::while_(dsl::code_point - lit_not - whitespace));
            static constexpr auto value = lexy::as_string<std::string>;
        };

        struct internal_node;

        struct subtree
        {
            static constexpr auto whitespace = dsl::ascii::space / dsl::ascii::newline;
            static constexpr auto rule = (dsl::peek(OPEN) >> dsl::recurse<internal_node> | dsl::else_ >> dsl::p<name>);

            static constexpr auto value = lexy::callback<result_t>( //
                [](int) {
                    fmt::print(">>>> subtree int");
                    return result_t{};
                },
                [](const char*) {
                    fmt::print(">>>> subtree const char*");
                    return result_t{};
                },
                [](const std::vector<result_t>& res) {
                    fmt::print(">>>> subtree<-{}\n", res.size());
                    return result_t{};
                }, //
                [](const std::string& name) {
                    fmt::print(">>>> subtree<-name \"{}\"\n", name);
                    return result_t{};
                }, //
                [](const result_t&) {
                    fmt::print(">>>> subtree<-result_t\n");
                    return result_t{};
                } //
            );
        };

        struct branch_set
        {
            static constexpr auto whitespace = dsl::ascii::space / dsl::ascii::newline;
            static constexpr auto rule = dsl::list(dsl::p<subtree> + dsl::p<length>, dsl::sep(dsl::comma));
            // static constexpr auto value = lexy::as_list<std::vector<result_t>>;
            static constexpr auto value = lexy::fold_inplace<result_t>(
                "fold-init-branch_set" /* std::initializer_list<result_t> {} */, [](result_t& res, const result_t&, double) { fmt::print(">>>> fold_inplace branch_set\n");});
        };

        struct internal_node
        {
            static constexpr auto whitespace = dsl::ascii::space / dsl::ascii::newline;
            static constexpr auto rule = OPEN + dsl::p<branch_set> + CLOSE + dsl::p<name>;

            // static constexpr auto cb = lexy::callback<int>([](const state_t& state, const result_t& br_set, const std::string& name) { fmt::print(">>>> internal_node::cb\n"); return 0; });
            // static constexpr auto value = lexy::bind(cb, lexy::parse_state, lexy::values);

            // static constexpr auto value = lexy::bind(lexy::callback(&state_t::self), lexy::parse_state, lexy::values);
            // static constexpr auto value = lexy::bind(lexy::callback([](const state_t& state, const result_t& br_set) { state.self(br_set); return 0; }), lexy::parse_state, lexy::values);
            // static constexpr auto value = lexy::bind(lexy::callback([](const state_t& state) { state.self(); return 0; }), lexy::parse_state);

            static constexpr auto value = lexy::callback<result_t>([](const result_t&, const std::string&) { return result_t{}; });
            // static constexpr auto value = lexy::fold_inplace<result_t>(
            //     "fold-init-internal_node", [](result_t& res, const result_t&, const std::string&) { fmt::print(">>>> fold_inplace internal_node");});
        };

        // struct xtree
        // {
        //     using return_type = xtree;
        //     constexpr xtree()
        //     { /*fmt::print(">>>> xtree\n");*/
        //     }
        //     constexpr xtree operator()(const result_t&) { return *this; }
        //     constexpr xtree operator()(result_t&&) { return *this; }
        // };

        struct tree
        {
            static constexpr auto whitespace = dsl::ascii::space / dsl::ascii::newline;
            static constexpr auto rule = dsl::p<internal_node> + dsl::semicolon + dsl::eof;
            static constexpr auto max_recursion_depth = 100;

            static constexpr auto value = lexy::forward<result_t>;
            // static constexpr auto value = lexy::callback<tree_t>([](const result_t&) { return std::make_shared<Tree>(); });
            // static constexpr auto value = xtree{};
            // static constexpr auto value = lexy::fold_inplace<result_t>(
            //     "fold-init-tree", [](result_t& res, const result_t&) { fmt::print(">>>> tree->fold");});
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

} // namespace ae::tree::newick

// ----------------------------------------------------------------------

std::shared_ptr<ae::tree::Tree> ae::tree::load_newick(const std::string& source)
{
   //  const bool trace { false };

   // if (trace) {
   //      fmt::print(">>> parsing newick\n");
   //      lexy::trace<newick::grammar::tree>(stderr, lexy::string_input<lexy::utf8_encoding>{source});
   //  }

    try {
        newick::state_t state;
        const auto parsing_result = lexy::parse<newick::grammar::tree>(lexy::string_input<lexy::utf8_encoding>{source}, state, newick::report_error{});
        // return parsing_result.value();
        return std::make_shared<Tree>();
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
