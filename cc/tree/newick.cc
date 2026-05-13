#include <vector>
#include <stack>

#include "ext/from_chars.hh"
#include "ext/lexy.hh"
#include "utils/timeit.hh"
#include "tree/newick.hh"
#include "tree/tree.hh"

// https://en.wikipedia.org/wiki/Newick_format

// ======================================================================

namespace ae::tree::newick
{
    struct tree_builder_t
    {
        tree_builder_t(Tree& a_tree) : tree{a_tree} {}
        tree_builder_t(Tree& a_tree, node_index_t join_at) : tree{a_tree}, debug{true} { parents.push(join_at); }

        void subtree_begin() const
        {
            if (parents.empty())
                parents.push(tree.root_index());
            else
                parents.push(tree.add_inode(parents.top()).first);
            // if (debug)
            //     fmt::print(stderr, ">>>> subtree_begin {} leaves: {}\n", parents.size(), leaves);
            max_nesting_level = std::max(max_nesting_level, parents.size());
        }

        void subtree_end(std::string_view name, double edge) const
        {
            auto& inode = tree.inode(parents.top());
            inode.edge = EdgeLength{edge};
            inode.name = name;
            parents.pop();
            // if (debug)
            //     fmt::print(stderr, ">>>> subtree_end {} leaves: {}\n", parents.size(), leaves);
        }

        void leaf(std::string_view name, double edge) const
        {
            tree.add_leaf(parents.top(), name, EdgeLength{edge});
            ++leaves;
            // fmt::print(">>>> {:4d} \"{}\" {} parents: {}\n", leaves, name, edge, parents.size());
        }

        Tree& tree;
        mutable std::stack<node_index_t> parents{};
        const bool debug{false};
        mutable size_t leaves{0};
        mutable size_t max_nesting_level{0};
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
            static constexpr auto exclude = lit_not / whitespace;
            static constexpr auto rule = dsl::capture(dsl::while_(dsl::code_point - exclude));
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
            static constexpr auto max_recursion_depth = 10000;

            static constexpr auto value = lexy::forward<result_t>;
        };

    } // namespace grammar

    // ----------------------------------------------------------------------

    struct report_error
    {
        struct error_sink
        {
            using return_type = std::size_t;

            template <typename Production, typename Input, typename Reader, typename Tag>
            void operator()(const lexy::error_context<Production, Input>& /*context*/, const lexy::error<Reader, Tag>& /*error*/)
            {
                // _detail::write_error(lexy::cfile_output_iterator{stderr}, context, error, {lexy::visualize_fancy});
                ++_count;
            }

            return_type finish() &&
            {
                // if (_count != 0)
                //     std::fputs("\n", stderr);
                return _count;
            }

            const report_error& report_error_;
            std::size_t _count{0};
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
        if (tree_builder.max_nesting_level >= newick::grammar::tree::max_recursion_depth)
            fmt::print("> Tree read partially, increase newick::grammar::tree::max_recursion_depth, current value: {}, number of leaves read: {}\n", newick::grammar::tree::max_recursion_depth, tree_builder.leaves);
        // fmt::print(">>>> {} leaves in the tree read from newick, meax nesting level: {}\n", tree_builder.leaves, tree_builder.max_nesting_level);
        return tree;
    }
    catch (newick::grammar::invalid_input& err) {
        fmt::print(">> newick parsing error: {}\n", err.what());
        return nullptr;
    }

} // ae::tree::load_newick

// ----------------------------------------------------------------------

void ae::tree::load_join_newick(const std::string& source, Tree& tree, node_index_t join_at)
{
    try {
        newick::tree_builder_t tree_builder{tree, join_at};
        if (!lexy::parse<newick::grammar::tree>(lexy::string_input<lexy::utf8_encoding>{source}, tree_builder, newick::report_error{}))
            throw newick::grammar::invalid_input{"parsing failed"};
        if (tree_builder.max_nesting_level >= newick::grammar::tree::max_recursion_depth)
            fmt::print("> Tree read partially, increase newick::grammar::tree::max_recursion_depth, current value: {}, number of leaves read: {}\n", newick::grammar::tree::max_recursion_depth, tree_builder.leaves);
        // fmt::print(">>>> {} leaves in the tree read from newick, meax nesting level: {}\n", tree_builder.leaves, tree_builder.max_nesting_level);
    }
    catch (newick::grammar::invalid_input& err) {
        fmt::print(">> newick parsing error: {}\n", err.what());
    }

} // ae::tree::load_join_newick

// ----------------------------------------------------------------------

std::string ae::tree::export_newick(const Tree& tree, const Inode& root, size_t indent)
{
    Timeit ti{"tree::export_newick", std::chrono::milliseconds{100}};

    fmt::memory_buffer text;
    std::vector<bool> commas{false};
    commas.reserve(tree.depth());
    size_t current_indent = 0;

    const auto format_prefix = [&text, &current_indent, indent]() {
        if (indent)
            fmt::format_to(std::back_inserter(text), "\n{:{}s}", " ", current_indent);
    };

    const auto format_comma = [&text, &commas, format_prefix]() {
        if (commas.back()) {
            fmt::format_to(std::back_inserter(text), ",");
            format_prefix();
        }
        else
            commas.back() = true;
    };

    const auto format_edge = [&text](auto edge) {
        if (edge != 0.0)
            fmt::format_to(std::back_inserter(text), ":{:.10g}", *edge);
    };

    const auto format_inode_pre = [&text, &commas, format_comma, &current_indent, indent, format_prefix](const Inode*) {
        format_comma();
        fmt::format_to(std::back_inserter(text), "(");
        current_indent += indent;
        format_prefix();
        commas.push_back(false);
    };

    const auto format_inode_post = [&text, &commas, format_edge, &current_indent, indent, format_prefix](const Inode* inode) {
        commas.pop_back();
        if (current_indent >= indent)
            current_indent -= indent;
        format_prefix();
        fmt::format_to(std::back_inserter(text), "){}", inode->name);
        format_edge(inode->edge);
    };

    const auto format_leaf = [&text, format_comma, format_edge](const Leaf* leaf) {
        format_comma();
        fmt::format_to(std::back_inserter(text), "{}", leaf->name);
        format_edge(leaf->edge);
    };

    const auto format_leaf_post = [](const Leaf* leaf) {
        fmt::print("> export_newick format_leaf_post \"{}\"\n", leaf->name);
    };

    bool within_subtree = false;
    for (const auto ref : tree.visit(tree_visiting::all_pre_post)) {
        if (ref.pre()) {
            if (!within_subtree && root.node_id_ == ref.node_id())
                within_subtree = true;
            if (within_subtree)
                ref.visit(format_inode_pre, format_leaf);
        }
        else if (within_subtree) {
            ref.visit(format_inode_post, format_leaf_post);
            if (root.node_id_ == ref.node_id())
                within_subtree = false;
        }
    }
    fmt::format_to(std::back_inserter(text), ";");

    return fmt::to_string(text);

} // ae::tree::export_newick

// ----------------------------------------------------------------------
