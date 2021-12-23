// #if defined(__cpp_impl_three_way_comparison) && __cpp_impl_three_way_comparison >= 201907L && defined(__cpp_concepts) && __cpp_concepts >= 201907L

#include <stack>
#include <memory>

#include "utils/file.hh"
#include "ad/rjson-v3.hh"
#include "ad/to-json.hh"

// ----------------------------------------------------------------------

#pragma GCC diagnostic push

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

const rjson::v3::value rjson::v3::const_null;
const rjson::v3::value rjson::v3::const_true{rjson::v3::detail::boolean{true}};
const rjson::v3::value rjson::v3::const_false{rjson::v3::detail::boolean{false}};
const rjson::v3::value rjson::v3::const_empty_string{rjson::v3::detail::string{std::string_view{""}}};
const rjson::v3::detail::array rjson::v3::const_empty_array{};
const rjson::v3::detail::object rjson::v3::const_empty_object{};

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

rjson::v3::value::~value()
{

} // rjson::v3::~value

// ----------------------------------------------------------------------

namespace parser_pop
{
    class SymbolHandler;

    class Parser
    {
      public:
        Parser(std::string_view filename);

        rjson::v3::value result_move();
        void parse(std::string_view data);

        constexpr auto pos() const noexcept { return pos_; }
        constexpr auto line() const noexcept { return line_; }
        constexpr auto column() const noexcept { return column_; }
        constexpr std::string_view filename() const noexcept { return filename_; }
        constexpr std::string_view data(size_t aBegin, size_t aEnd) const { return {source_.data() + aBegin, aEnd - aBegin}; }

        constexpr void back() noexcept
        {
            --pos_;
            if (--column_ == 0)
                --line_;
        }

        constexpr void newline() noexcept
        {
            ++line_;
            column_ = 0;
        }

      private:
        std::string_view source_{};
        std::string_view filename_{};
        size_t pos_{0}, line_{1}, column_{1};
        std::stack<std::unique_ptr<SymbolHandler>> handlers_{};

        void pop() noexcept;

    }; // class Parser

    // ----------------------------------------------------------------------

    class SymbolHandler;

    class StateTransitionNone
    {
    };
    class StateTransitionPop
    {
    };

    using HandlingResult = std::variant<StateTransitionNone, StateTransitionPop, std::unique_ptr<SymbolHandler>>;

    // ----------------------------------------------------------------------

    class SymbolHandler
    {
      public:
        virtual ~SymbolHandler() = default;

        [[noreturn]] void unexpected(std::string_view::value_type aSymbol, Parser& aParser) const
        {
            throw rjson::v3::parse_error(aParser.filename(), aParser.line(), aParser.column(), fmt::format("unexpected symbol: '{0}' (0x{0:02X})", aSymbol));
        }

        [[noreturn]] void error(Parser& aParser, std::string_view aMessage) const { throw rjson::v3::parse_error(aParser.filename(), aParser.line(), aParser.column(), aMessage); }

        [[noreturn]] void internal_error(Parser& aParser) const { throw rjson::v3::parse_error(aParser.filename(), aParser.line(), aParser.column(), "internal error"); }

        void newline(Parser& aParser) const { aParser.newline(); }

        virtual HandlingResult handle(std::string_view::value_type aSymbol, Parser& aParser)
        {
            switch (aSymbol) {
                case ' ':
                case '\t':
                case '\r':
                    break;
                case '\n':
                    newline(aParser);
                    break;
                case '#': // JSON extension: comment until end of line
                    return make_comment_handler();
                default:
                    unexpected(aSymbol, aParser);
            }
            return StateTransitionNone{};
        }

        virtual rjson::v3::value value_move() = 0;
        virtual void subvalue(rjson::v3::value&& /*aSubvalue*/, Parser& /*aParser*/) {}

      private:
        HandlingResult make_comment_handler();

    }; // class SymbolHandler

    // --------------------------------------------------

    class CommentHandler : public SymbolHandler
    {
      public:
        HandlingResult handle(std::string_view::value_type aSymbol, Parser& aParser) override
        {
            if (aSymbol == '\n') {
                newline(aParser);
                return StateTransitionPop{};
            }
            else
                return StateTransitionNone{};
        }

        rjson::v3::value value_move() override { return {}; }

    }; // class StringEscapeHandler

    inline HandlingResult SymbolHandler::make_comment_handler() { return std::make_unique<CommentHandler>(); }

    // ----------------------------------------------------------------------

    class ValueHandler : public SymbolHandler
    {
      public:
        HandlingResult handle(std::string_view::value_type aSymbol, Parser& aParser) override;

        void subvalue(rjson::v3::value&& subvalue, Parser& /*aParser*/) override
        {
            value_ = std::move(subvalue);
            value_read_ = true;
        }

        rjson::v3::value value_move() override { return std::move(value_); }
        constexpr auto& value_ref() { return value_; }

      protected:
        constexpr bool value_read() const { return value_read_; }

      private:
        bool value_read_{false};
        rjson::v3::value value_{};

    }; // class ValueHandler

    // --------------------------------------------------

    class ToplevelHandler : public ValueHandler
    {
      public:
        HandlingResult handle(std::string_view::value_type aSymbol, Parser& aParser) override
        {
            HandlingResult result;
            if (value_read())
                result = SymbolHandler::handle(aSymbol, aParser);
            else
                result = ValueHandler::handle(aSymbol, aParser);
            return result;
        }

    }; // class ToplevelHandler

    // --------------------------------------------------

    class StringHandler : public SymbolHandler
    {
      public:
        StringHandler(Parser& aParser) : parser_{aParser}, begin_{aParser.pos() + 1}, end_{0} {}

        HandlingResult handle(std::string_view::value_type aSymbol, Parser& aParser) override
        {
            HandlingResult result = StateTransitionNone{};
            switch (aSymbol) {
                case '"':
                    if (!escaped) {
                        result = StateTransitionPop{};
                        end_ = aParser.pos();
                    }
                    else
                        escaped = false;
                    break;
                case '\\':
                    escaped = true;
                    break;
                case '\n':
                    newline(aParser);
                    escaped = false;
                    break;
                default:
                    escaped = false;
                    break;
            }
            return result;
        }

        rjson::v3::value value_move() override { return rjson::v3::detail::string{parser_.data(begin_, end_)}; }

      private:
        Parser& parser_;
        bool escaped{false};
        size_t begin_{0}, end_{0};

    }; // class StringHandler

    // ----------------------------------------------------------------------

    class NumberHandler : public SymbolHandler
    {
      public:
        NumberHandler(Parser& aParser) : parser_{aParser}, begin_{aParser.pos()}, end_{0} {}

        HandlingResult handle(std::string_view::value_type aSymbol, Parser& aParser) override
        {
            HandlingResult result = StateTransitionNone{};
            end_ = aParser.pos() + 1;
            switch (aSymbol) {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    sign_allowed_ = false;
                    break;
                case '.':
                    if (exponent_)
                        unexpected(aSymbol, aParser);
                    // integer_ = false;
                    sign_allowed_ = false;
                    break;
                case 'e':
                case 'E':
                    // integer_ = false;
                    exponent_ = true;
                    sign_allowed_ = true;
                    break;
                case '-':
                case '+':
                    if (!sign_allowed_)
                        unexpected(aSymbol, aParser);
                    sign_allowed_ = false;
                    break;
                default:
                    result = StateTransitionPop{};
                    end_ = aParser.pos();
                    aParser.back();
                    break;
            }
            return result;
        }

        rjson::v3::value value_move() override { return rjson::v3::detail::number{parser_.data(begin_, end_)}; }

      private:
        Parser& parser_;
        size_t begin_{0}, end_{0};
        bool sign_allowed_ = true;
        bool exponent_ = false;

    }; // class NumberHandler

    // ----------------------------------------------------------------------

    class BoolNullHandler : public SymbolHandler
    {
      public:
        BoolNullHandler(const char* aExpected, rjson::v3::value&& aValue) : expected_{aExpected}, value_{std::move(aValue)} {}
        BoolNullHandler(const BoolNullHandler&) = default;
        BoolNullHandler& operator=(const BoolNullHandler&) = default;

        HandlingResult handle(std::string_view::value_type aSymbol, Parser& aParser) override
        {
            HandlingResult result = StateTransitionNone{};
            switch (aSymbol) {
                case 'r':
                case 'u':
                case 'e':
                case 'a':
                case 'l':
                case 's':
                    if (expected_[pos_] != aSymbol)
                        unexpected(aSymbol, aParser);
                    break;
                default:
                    if (expected_[pos_] == 0) {
                        result = StateTransitionPop{};
                        aParser.back();
                    }
                    else {
                        result = SymbolHandler::handle(aSymbol, aParser);
                    }
            }
            ++pos_;
            return result;
        }

        rjson::v3::value value_move() override { return std::move(value_); }

      private:
        const char* expected_{nullptr};
        rjson::v3::value value_{};

        size_t pos_ = 0;

    }; // class BoolNullHandler

    // ----------------------------------------------------------------------

    class ObjectHandler : public SymbolHandler
    {
      private:
        enum class Expected { Key, KeyAfterComma, Value, Colon, Comma };

      public:
        ObjectHandler() {}

        HandlingResult handle(std::string_view::value_type aSymbol, Parser& aParser) override
        {
            HandlingResult result = StateTransitionNone{};
            switch (aSymbol) {
                case '}':
                    switch (expected_) {
                        case Expected::Key:
                        case Expected::Comma:
                            result = StateTransitionPop{};
                            break;
                        case Expected::KeyAfterComma:
                            // error(aParser, "unexpected " + std::string{aSymbol} + " -- did you forget to remove last comma?");
                            result = StateTransitionPop{}; // JSON extension: allow comma at the end of object
                            break;
                        case Expected::Value:
                        case Expected::Colon:
                            unexpected(aSymbol, aParser);
                    }
                    break;
                case ',':
                    switch (expected_) {
                        case Expected::Comma:
                            expected_ = Expected::KeyAfterComma;
                            break;
                        case Expected::Key:
                            error(aParser, "unexpected comma right after the beginning of an object");
                        case Expected::KeyAfterComma:
                            error(aParser, "unexpected comma -- two successive commas?");
                        case Expected::Value:
                            error(aParser, "unexpected comma after colon"); // never comes here (processed by ValueHandler)
                        case Expected::Colon:
                            error(aParser, "unexpected comma, colon is expected there");
                    }
                    break;
                case ':':
                    if (expected_ == Expected::Colon) {
                        expected_ = Expected::Value;
                        result = std::make_unique<ValueHandler>();
                    }
                    else
                        unexpected(aSymbol, aParser);
                    break;
                case '"':
                    switch (expected_) {
                        case Expected::Key:
                        case Expected::KeyAfterComma:
                        case Expected::Value:
                            result = std::make_unique<StringHandler>(aParser);
                            break;
                        case Expected::Comma:
                            error(aParser, fmt::format("unexpected {} -- did you forget comma?", aSymbol));
                        case Expected::Colon:
                            unexpected(aSymbol, aParser);
                    }
                    break;
                default:
                    result = SymbolHandler::handle(aSymbol, aParser);
                    break;
            }
            return result;
        }

        rjson::v3::value value_move() override { return std::move(value_); }

        void subvalue(rjson::v3::value&& aSubvalue, Parser& aParser) override
        {
            switch (expected_) {
                case Expected::Key:
                case Expected::KeyAfterComma:
                    key_ = std::move(aSubvalue);
                    expected_ = Expected::Colon;
                    break;
                case Expected::Value:
                    value_.insert(key_._content(), std::move(aSubvalue));
                    expected_ = Expected::Comma;
                    break;
                case Expected::Comma:
                case Expected::Colon:
                    internal_error(aParser);
            }
        }

      private:
        rjson::v3::detail::object value_{};
        rjson::v3::value key_{};
        Expected expected_ = Expected::Key;

    }; // class ObjectHandler

    // ----------------------------------------------------------------------

    class ArrayHandler : public SymbolHandler
    {
      private:
        enum class Expected { Value, ValueAfterComma, Comma };

      public:
        ArrayHandler() {}

        HandlingResult handle(std::string_view::value_type aSymbol, Parser& aParser) override
        {
            HandlingResult result = StateTransitionNone{};
            switch (aSymbol) {
                case ']':
                    switch (expected_) {
                        case Expected::Value:
                        case Expected::Comma:
                            result = StateTransitionPop{};
                            break;
                        case Expected::ValueAfterComma:
                            // error(aParser, "unexpected " + std::string{aSymbol} + " -- did you forget to remove last comma?");
                            result = StateTransitionPop{}; // JSON extension: allow comma at the end of array
                            break;
                    }
                    break;
                case ',':
                    switch (expected_) {
                        case Expected::Value:
                            error(aParser, "unexpected comma right after the beginning of an array");
                        case Expected::Comma:
                            expected_ = Expected::ValueAfterComma;
                            break;
                        case Expected::ValueAfterComma:
                            error(aParser, "unexpected comma -- two successive commas?");
                    }
                    break;
                case '\n':
                    newline(aParser);
                    break;
                case ' ':
                case '\t':
                    break;
                case '#': // JSON extension: comment until end of line
                    result = SymbolHandler::handle(aSymbol, aParser);
                    break;
                default:
                    switch (expected_) {
                        case Expected::Value:
                        case Expected::ValueAfterComma:
                            result = std::make_unique<ValueHandler>();
                            aParser.back();
                            expected_ = Expected::Comma;
                            break;
                        case Expected::Comma:
                            error(aParser, fmt::format("unexpected {} -- did you forget comma?", aSymbol));
                    }
                    break;
            }
            return result;
        }

        rjson::v3::value value_move() override { return std::move(value_); }

        void subvalue(rjson::v3::value&& aSubvalue, Parser& /*aParser*/) override { value_.append(std::move(aSubvalue)); }

      private:
        rjson::v3::detail::array value_{};
        Expected expected_ = Expected::Value;

    }; // class ArrayHandler

    // ----------------------------------------------------------------------

    HandlingResult ValueHandler::handle(std::string_view::value_type aSymbol, Parser& aParser)
    {
        HandlingResult result = StateTransitionNone{};
        if (value_read()) {
            aParser.back();
            result = StateTransitionPop{};
        }
        else {
            switch (aSymbol) {
                case '"':
                    result = std::make_unique<StringHandler>(aParser);
                    break;
                case '-':
                case '+':
                case '.':
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    result = std::make_unique<NumberHandler>(aParser);
                    aParser.back();
                    break;
                case 't':
                    result = std::make_unique<BoolNullHandler>("rue", rjson::v3::detail::boolean{true});
                    break;
                case 'f':
                    result = std::make_unique<BoolNullHandler>("alse", rjson::v3::detail::boolean{false});
                    break;
                case 'n':
                    result = std::make_unique<BoolNullHandler>("ull", rjson::v3::detail::null{});
                    break;
                case '{':
                    result = std::make_unique<ObjectHandler>();
                    break;
                case '[':
                    result = std::make_unique<ArrayHandler>();
                    break;
                default:
                    result = SymbolHandler::handle(aSymbol, aParser);
                    break;
            }
        }
        return result;
    }

    // --------------------------------------------------

    inline Parser::Parser(std::string_view filename) : filename_{filename} { handlers_.emplace(std::make_unique<ToplevelHandler>()); }

    inline rjson::v3::value Parser::result_move() { return handlers_.top()->value_move(); }

    inline void Parser::parse(std::string_view data)
    {
        source_ = data;
        for (pos_ = 0; pos_ < source_.size(); ++pos_, ++column_) {
            const auto symbol = source_[pos_];
            auto handling_result = handlers_.top()->handle(symbol, *this);
            std::visit(
                [this](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, std::unique_ptr<SymbolHandler>>) {
                        this->handlers_.push(std::forward<T>(arg));
                    }
                    else if constexpr (std::is_same_v<T, StateTransitionPop>) {
                        pop();
                    }
                },
                handling_result);
        }
        if (handlers_.size() > 1)
            pop();
    }

    inline void Parser::pop() noexcept
    {
        auto value{handlers_.top()->value_move()};
        handlers_.pop();
        handlers_.top()->subvalue(std::move(value), *this);
    }

} // namespace parser_pop

// ----------------------------------------------------------------------

namespace rjson::v3
{
    value_read parse(std::string&& data, std::string_view filename)
    {
        value_read result{std::move(data)};
        parser_pop::Parser parser{filename};
        parser.parse(result.buffer_);

        // parser.remove_emacs_indent();
        // parser.remove_comments();

        result = parser.result_move();
        return result;
    }
}

// ----------------------------------------------------------------------

rjson::v3::value_read rjson::v3::parse_string(std::string_view data)
{
    return parse(std::string{data}, std::string_view{});

} // rjson::v3::parse_string

// ----------------------------------------------------------------------

rjson::v3::value rjson::v3::parse_string_no_keep(std::string_view data) // assume data is kept somewhere, do not copy it
{
    parser_pop::Parser parser{std::string_view{}};
    parser.parse(data);
    return parser.result_move();

} // rjson::v3::parse_string_no_keep

// ----------------------------------------------------------------------

rjson::v3::value_read rjson::v3::parse_file(std::string_view filename)
{
    return parse(ae::file::read(filename), filename);

} // rjson::v3::parse_file

// ----------------------------------------------------------------------

inline to_json::json format_to_json(const rjson::v3::value& val) noexcept
{
    using namespace std::string_view_literals;
    return val.visit([]<typename Content>(Content&& arg) -> to_json::json {
        if constexpr (std::is_same_v<std::decay_t<Content>, rjson::v3::detail::null>) {
            return to_json::raw("null"sv);
        }
        else if constexpr (std::is_same_v<std::decay_t<Content>, rjson::v3::detail::object>) {
            to_json::object obj;
            for (const auto& [obj_key, obj_val] : arg)
                obj << to_json::key_val(obj_key, format_to_json(obj_val));
#if defined(__clang_major__) && __clang_major__ < 13
            return std::move(obj);
#else
            return obj;
#endif
        }
        else if constexpr (std::is_same_v<std::decay_t<Content>, rjson::v3::detail::array>) {
            to_json::array arr;
            for (const auto& arr_val : arg)
                arr << format_to_json(arr_val);
#if defined(__clang_major__) && __clang_major__ < 13
            return std::move(arr);
#else
            return arr;
#endif
        }
        else if constexpr (std::is_same_v<std::decay_t<Content>, rjson::v3::detail::string>) {
            return to_json::val(arg._content());
        }
        else if constexpr (std::is_same_v<std::decay_t<Content>, rjson::v3::detail::number>) {
            return to_json::val(arg.template to<double>());
        }
        else if constexpr (std::is_same_v<std::decay_t<Content>, rjson::v3::detail::boolean>) {
            return to_json::val(arg.template to<bool>());
        }
        else
            static_assert(std::is_same_v<Content, void>);
    });
}

std::string rjson::v3::format(const value& val, output outp, size_t indent) noexcept
{
    const auto res = ::format_to_json(val);
    if (indent > 0)
        return res.pretty(indent);
    switch (outp) {
        case output::compact:
            return res.compact(to_json::json::embed_space::no);
        case output::compact_with_spaces:
            return res.compact(to_json::json::embed_space::yes);
        case output::pretty1:
            return res.pretty(1);
        case output::pretty:
        case output::pretty2:
            return res.pretty(2);
        case output::pretty4:
            return res.pretty(4);
      case output::pretty8:
              return res.pretty(8);
    }
    return res.compact(to_json::json::embed_space::yes);

} // rjson::v3::format

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------

// #else
// #warning rjson::v3 is not yet supported by the compiler
// #endif

// ----------------------------------------------------------------------
