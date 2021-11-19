#include <vector>
#include <stack>

#include "tree/newick.hh"
#include "ext/from_chars.hh"
#include "ext/lexy.hh"

// https://en.wikipedia.org/wiki/Newick_format

// ======================================================================

namespace ae::tree::newick
{
    struct tree_builder_t
    {
        tree_builder_t(Tree& a_tree) : tree{a_tree} {}

        void subtree_begin() const
        {
            if (parents.empty())
                parents.push(tree.root_index());
            else
                parents.push(tree.add_inode(parents.top()).first);
        }

        void subtree_end(std::string_view name, double edge) const
        {
            auto& inode = tree.inode(parents.top());
            inode.edge = EdgeLength{edge};
            inode.name = name;
            parents.pop();
        }

        void leaf(std::string_view name, double edge) const { tree.add_leaf(parents.top(), name, EdgeLength{edge}); }

        Tree& tree;
        mutable std::stack<node_index_t> parents{};
    };

    // ----------------------------------------------------------------------

    struct result_t
    {
    };

    // ----------------------------------------------------------------------

    template <typename Callback> struct fold_branch_t
    {
        Callback callback;

        using return_type = result_t;

        struct sink_callback
        {
            const tree_builder_t& tree_builder;
            Callback callback;

            using return_type = result_t;

            template <typename... Args> constexpr void operator()(Args&&... args) { std::invoke(callback, tree_builder, std::forward<Args>(args)...); }

            constexpr return_type finish() && { return {}; }
        };

        constexpr auto sink(const tree_builder_t& tree_builder) const
        {
            tree_builder.subtree_begin();
            return sink_callback{tree_builder, callback};
        }
    };

    template <typename Callback> constexpr auto fold_branch(Callback&& callback) { return fold_branch_t<Callback>{std::forward<Callback>(callback)}; }

    // ----------------------------------------------------------------------

    namespace grammar
    {
        namespace dsl = lexy::dsl;

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
            static constexpr auto value = lexy::callback<double>([](auto lex) { return ae::from_chars<double>(lex.begin(), lex.end()); });
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

        struct name_edge
        {
            static constexpr auto whitespace = dsl::ascii::space / dsl::ascii::newline;
            static constexpr auto rule = dsl::p<name> + dsl::p<length>;

            static constexpr auto value = lexy::bind(lexy::callback<result_t>([](const tree_builder_t& tree_builder, const std::string& name, double edge) {
                                                         tree_builder.leaf(name, edge);
                                                         return result_t{};
                                                     }),
                                                     lexy::parse_state, lexy::values);
        };

        struct internal_node;

        struct subtree
        {
            static constexpr auto whitespace = dsl::ascii::space / dsl::ascii::newline;
            static constexpr auto rule = (dsl::peek(OPEN) >> dsl::recurse<internal_node> | dsl::else_ >> dsl::p<name_edge>);
            static constexpr auto value = lexy::forward<result_t>;
        };

        struct branch_set
        {
            static constexpr auto whitespace = dsl::ascii::space / dsl::ascii::newline;
            static constexpr auto rule = dsl::list(dsl::p<subtree>, dsl::sep(dsl::comma));

            static constexpr auto value = lexy::bind_sink(fold_branch([](const tree_builder_t&, result_t) {}), lexy::parse_state);
        };

        struct internal_node
        {
            static constexpr auto whitespace = dsl::ascii::space / dsl::ascii::newline;
            static constexpr auto rule = OPEN + dsl::p<branch_set> + CLOSE + dsl::p<name> + dsl::p<length>;

            static constexpr auto value = lexy::bind(lexy::callback<result_t>([](const tree_builder_t& tree_builder, result_t, const std::string& name, double edge) {
                                                         tree_builder.subtree_end(name, edge);
                                                         return result_t{};
                                                     }),
                                                     lexy::parse_state, lexy::values);
        };

        struct tree
        {
            static constexpr auto whitespace = dsl::ascii::space / dsl::ascii::newline;
            static constexpr auto rule = dsl::p<internal_node> + dsl::semicolon + dsl::eof;
            static constexpr auto max_recursion_depth = 100;

            static constexpr auto value = lexy::forward<result_t>;
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
    const bool trace { false };

   if (trace) {
        fmt::print(">>> parsing newick\n");
        lexy::trace<newick::grammar::tree>(stderr, lexy::string_input<lexy::utf8_encoding>{source});
    }

    try {
        auto tree = std::make_shared<Tree>();
        newick::tree_builder_t tree_builder{*tree};
        if (!lexy::parse<newick::grammar::tree>(lexy::string_input<lexy::utf8_encoding>{source}, tree_builder, newick::report_error{}))
            throw newick::grammar::invalid_input{"parsing failed"};
        return tree;
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
