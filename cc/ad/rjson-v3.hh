#pragma once

// ----------------------------------------------------------------------

// /usr/local/Cellar/llvm/10.0.0_3/include/c++/v1/__config
// https://isocpp.org/std/standing-documents/sd-6-sg10-feature-test-recommendations#__cpp_range_based_for
// #if !defined(__cpp_impl_three_way_comparison) || __cpp_impl_three_way_comparison < 201907L
// #error rjson::v3 requires <=> support
// #endif

// #if !defined(__cpp_concepts) || __cpp_concepts < 201907L
// #error rjson::v3 requires concepts support
// #endif

// ----------------------------------------------------------------------

#include <stdexcept>
#include <variant>
#include <string_view>
#include <typeinfo>
#include <optional>

#include "utils/log.hh"
#include "utils/float.hh"
#include "ad/flat-map.hh"
#include "ext/from_chars.hh"

// ----------------------------------------------------------------------

namespace rjson::v3
{
    class value;

    class error : public std::runtime_error
    {
      public:
        using std::runtime_error::runtime_error;
    };

    class value_type_mismatch : public error
    {
      public:
        value_type_mismatch(std::string_view requested_type, const value& val, std::string_view source_ref={});
        value_type_mismatch(std::string_view requested_type, const std::type_info& actual_type, std::string_view source_ref={})
            : error{fmt::format("value type mismatch, requested: {}, stored: {}{}", requested_type, actual_type.name(), source_ref)}
        {
        }
        value_type_mismatch(const std::type_info& requested_type, std::string_view actual_type, std::string_view source_ref={})
            : error{fmt::format("value type mismatch, requested: {}, stored: {}{}", requested_type.name(), actual_type, source_ref)}
        {
        }
    };

    class parse_error : public error
    {
      public:
        parse_error(size_t line, size_t column, std::string_view message) : error{""}, message_{fmt::format("{}:{}: {}", line, column, message)} {}
        parse_error(std::string_view filename, size_t line, size_t column, std::string_view message) : error{""}, message_{fmt::format("{}:{}:{}: {}", filename, line, column, message)} {}
        const char* what() const noexcept override { return message_.data(); }

      private:
        std::string message_{};

    }; // class parse_error

    // ----------------------------------------------------------------------

    namespace detail
    {
        class null
        {
          public:
            template <typename Output> Output to() const
            {
                throw value_type_mismatch{typeid(Output), "null"};
            }
        };

        class object
        {
          public:
            object() = default;
            object(const object&) = default;
            object(object&&) = default;
            object& operator=(object&&) = default;
            object& operator=(const object&) = default;

            /*constexpr*/ bool empty() const noexcept { return content_.empty(); }
            /*constexpr*/ size_t size() const noexcept { return content_.size(); }
            /*constexpr*/ auto begin() const noexcept { return content_.begin(); }
            /*constexpr*/ auto end() const noexcept { return content_.end(); }

            void insert(std::string_view aKey, value&& aValue);
            const value& operator[](std::string_view key) const noexcept; // returns null if not found
            template <typename... Keys> const value& get(std::string_view key, Keys&&... rest) const;

            template <typename Output> Output to() const
            {
                throw value_type_mismatch{typeid(Output), "object"};
            }

          private:
            using content_t = acmacs::small_map_with_unique_keys_t<std::string_view, value>; // to avoid sorting which invalidates string_view's
            content_t content_{};
        };

        class array
        {
          public:
            array() = default;
            array(const array&) = default;
            array(array&&) = default;
            array& operator=(array&&) = default;
            array& operator=(const array&) = default;

            /*constexpr*/ bool empty() const noexcept { return content_.empty(); }
            /*constexpr*/ size_t size() const noexcept { return content_.size(); }
            /*constexpr*/ auto begin() const noexcept { return content_.begin(); }
            /*constexpr*/ auto end() const noexcept { return content_.end(); }

            void append(value&& aValue);
            const value& operator[](size_t index) const noexcept; // returns null if out of range

            template <typename Output> Output to() const
            {
                throw value_type_mismatch{typeid(Output), "array"};
            }

          private:
            std::vector<value> content_{};
        };

        class simple
        {
          public:
            constexpr simple() = default;
            constexpr simple(std::string_view content) : content_{content} {}

            constexpr std::string_view _content() const noexcept { return content_; }

          private:
            std::string_view content_{};
        };

        class string : public simple
        {
          public:
            using simple::simple;

            // for strings created within program, i.e. not read from bigger json, see settings Environment add_to_toplevel(std::string_view key, std::string_view value)
            enum with_content_ { with_content };
            string(with_content_, std::string_view content) : simple{}, scontent_{content} {}
            string(with_content_, std::string&& content) : simple{}, scontent_{std::move(content)} {}

            constexpr std::string_view _content() const noexcept { if (scontent_.has_value()) return *scontent_; else return simple::_content(); }
            constexpr bool empty() const noexcept { return _content().empty(); }
            constexpr size_t size() const noexcept { return _content().size(); }

            template <typename Output> Output to() const
            {
                static_assert(!std::is_convertible_v<Output, const char*>);
                if constexpr (std::is_constructible_v<Output, std::string_view>)
                    return Output{_content()};
                else
                    throw value_type_mismatch{typeid(Output), fmt::format("string{{\"{}\"}}", _content())};
            }

          private:
            std::optional<std::string> scontent_{std::nullopt}; // see constructor with with_content_ above
        };

        class number : public simple
        {
          public:
            template <typename Output> Output to() const
            {
                if constexpr (std::is_same_v<Output, bool>) {
                    throw value_type_mismatch{typeid(Output), fmt::format("number{{{}}}", _content())};
                }
                else if constexpr (std::is_floating_point_v<Output>) { //|| std::is_constructible_v<Output, double>)) {
                    if (const auto val = ae::from_chars<Output>(_content()); !float_equal(val, std::numeric_limits<Output>::max()))
                        return val;
                    else
                        throw value_type_mismatch{typeid(Output), fmt::format("number{{{}}}", _content())};
                }
                else if constexpr (std::is_arithmetic_v<Output>) {
                    if (const auto val = ae::from_chars<Output>(_content()); val != std::numeric_limits<Output>::max())
                        return val;
                    else
                        throw value_type_mismatch{typeid(Output), fmt::format("number{{{}}}", _content())};
                }
                else if constexpr (std::is_constructible_v<Output, double>) {
                    if (const auto val = ae::from_chars<double>(_content()); !float_equal(val, std::numeric_limits<double>::max()))
                        return Output{val};
                    else
                        throw value_type_mismatch{typeid(Output), fmt::format("number{{{}}}", _content())};
                }
                else if constexpr (std::is_constructible_v<Output, long>) {
                    if (const auto val = ae::from_chars<long>(_content()); val != std::numeric_limits<long>::max())
                        return Output{val};
                    else
                        throw value_type_mismatch{typeid(Output), fmt::format("number{{{}}}", _content())};
                }
                else
                    throw value_type_mismatch{typeid(Output), fmt::format("number{{{}}}", _content())};
            }
        };

        class boolean
        {
          public:
            constexpr boolean(bool content) : content_{content} {}
            constexpr explicit operator bool() const noexcept { return content_; }

            template <typename Output> Output to() const
            {
                if constexpr (std::is_constructible_v<Output, bool> && !std::is_floating_point_v<Output>)
                    return Output{content_};
                else
                    throw value_type_mismatch{typeid(Output), fmt::format("boolean{{{}}}", content_)};
            }

          private:
            bool content_{false};
        };

    } // namespace detail

    // ----------------------------------------------------------------------

    class value_read;

    class value
    {
      public:
        value() = default;
        value(value&&) = default;
        value(const value&) = default; // to support Settings and pushing to environament within program
        value& operator=(value&&) = default;
        value& operator=(const value&) = default;
        constexpr value(detail::string&& src) : value_{std::move(src)} {}
        constexpr value(detail::number&& src) : value_{std::move(src)} {}
        constexpr value(detail::null&& src) : value_{std::move(src)} {}
        constexpr value(detail::boolean&& src) : value_{std::move(src)} {}
        constexpr value(detail::object&& src) : value_{std::move(src)} {}
        constexpr value(detail::array&& src) : value_{std::move(src)} {}

        virtual ~value();

        bool operator==(const value& to_compare) const noexcept;

        const value& operator[](std::string_view key) const { return object()[key]; } // throw value_type_mismatch if not object, returns null if not found
        const value& operator[](size_t index) const { return array()[index]; } // throw value_type_mismatch if not array, returns null if out of range
        template <typename... Key> const value& get(Key&&... keys) const { return object().get(keys ...); } // throw value_type_mismatch if not object, returns null if not found

        explicit operator bool() const;

        bool is_null() const noexcept;
        bool is_object() const noexcept;
        bool is_array() const noexcept;
        bool is_string() const noexcept;
        bool is_number() const noexcept;
        bool is_bool() const noexcept;
        const std::type_info& actual_type() const noexcept;

        const detail::object& object() const; // returns const_empty_object if null
        const detail::array& array() const; // returns const_empty_array if null

        template <typename Output> Output to() const; // throws value_type_mismatch
        std::string as_string() const noexcept;

        bool empty() const noexcept;
        size_t size() const noexcept; // returns 0 if neither array nor object nor string

        std::string_view _content() const noexcept;

        template <typename Callback> auto visit(Callback&& callback) const -> decltype(callback(std::declval<detail::null>()))
        {
            return std::visit(std::forward<Callback>(callback), value_);
        }

      private:
        using value_base = std::variant<detail::null, detail::object, detail::array, detail::string, detail::number, detail::boolean>; // null must be the first alternative, it is the default value;

        value_base value_{detail::null{}};

        friend value_read parse(std::string&& data, std::string_view filename);

    }; // class value

    // ----------------------------------------------------------------------

    class value_read : public value
    {
      public:
        value_read(const value_read& val) = default;
        value_read(value_read&& val) = default;
        value_read& operator=(const value_read& val) = default;
        value_read& operator=(value_read&& val) = default;

      private:
        value_read(std::string&& buf) : buffer_{std::move(buf)} {}

        value& operator=(value&& val) // buffer_ untouched
        {
            value::operator=(std::move(val));
            return *this;
        }

        std::string buffer_{};

        friend value_read parse(std::string&& data, std::string_view filename);
    };

    // ======================================================================

    extern const value const_null;
    extern const value const_true;
    extern const value const_false;
    extern const value const_empty_string;
    extern const detail::array const_empty_array;
    extern const detail::object const_empty_object;

    value_read parse_string(std::string_view data);
    value parse_string_no_keep(std::string_view data); // assume data is kept somewhere, do not copy it
    value_read parse_file(std::string_view filename);

    enum class output { compact, compact_with_spaces, pretty, pretty1, pretty2, pretty4, pretty8 };

    std::string format(const value& val, output outp = output::compact_with_spaces, size_t indent = 0) noexcept;

    // ======================================================================

    template <typename Target> inline void copy_if_not_null(const value& source, Target& target)
    {
        // to(source, target, ignore_null::yes);
        source.visit([&target]<typename Value>(const Value& arg) {
            if constexpr (!std::is_same_v<Value, detail::null>)
                target = arg.template to<Target>();
        });
    }

    template <typename Target, typename Callable> inline void set_if_not_null(const value& source, Callable&& callable)
    {
        source.visit([&callable]<typename Value>(const Value& arg) {
            if constexpr (!std::is_same_v<Value, detail::null>) {
                if constexpr (std::is_invocable_v<Callable, Target>)
                    callable(arg.template to<std::decay_t<Target>>());
                else
                    static_assert(std::is_invocable_v<Callable, Target>);
            }
        });
    }

    template <typename T> inline T get_or(const value& source, const T& default_value)
    {
        if (source.is_null())
            return default_value;
        else
            return source.to<T>();
    }

    template <typename T> inline T get_or(const value& source, std::string_view field_name, const T& default_value)
    {
        if (source.is_null())
            return default_value;
        else
            return get_or(source[field_name], default_value);
    }

    // ======================================================================

    inline const std::type_info& value::actual_type() const noexcept
    {
        return std::visit([]<typename Content>(Content&& arg) -> const std::type_info& { return typeid(arg); }, value_);
    }

    inline bool value::operator==(const value& to_compare) const noexcept
    {
        return actual_type().hash_code() == to_compare.actual_type().hash_code() && format(*this, output::compact) == format(to_compare, output::compact);
    }

    inline value::operator bool() const
    {
        return std::visit(
            [this]<typename Content>(const Content& arg) -> bool {
                if constexpr (std::is_same_v<Content, detail::boolean>)
                    return arg.template to<bool>();
                else if constexpr (std::is_same_v<Content, detail::null>)
                    return false;
                else if constexpr (std::is_same_v<Content, detail::string>)
                    return !arg.empty();
                else if constexpr (std::is_same_v<Content, detail::number>)
                    return !float_zero(arg.template to<double>());
                else
                    throw value_type_mismatch{"<convertible-to-bool>", *this};
            },
            value_);
    }

    template <typename Output> inline Output value::to() const
    {
        return std::visit([]<typename Content>(Content&& arg) { return arg.template to<Output>(); }, value_);
    }

    inline std::string value::as_string() const noexcept
    {
        if (is_string())
            return std::string{to<std::string_view>()};
        else
            return format(*this, output::compact);
    }

    inline const detail::object& value::object() const
    {
        return std::visit(
            [this]<typename Content>(Content&& arg) -> const detail::object& {
                if constexpr (std::is_same_v<std::decay_t<Content>, detail::object>)
                    return arg;
                else if constexpr (std::is_same_v<std::decay_t<Content>, detail::null>)
                    return const_empty_object;
                else
                    throw value_type_mismatch{"rjson::v3::object", *this};
            },
            value_);
    }

    inline const detail::array& value::array() const
    {
        return std::visit(
            [this]<typename Content>(Content&& arg) -> const detail::array& {
                if constexpr (std::is_same_v<std::decay_t<Content>, detail::array>)
                    return arg;
                else if constexpr (std::is_same_v<std::decay_t<Content>, detail::null>)
                    return const_empty_array;
                else
                    throw value_type_mismatch{"rjson::v3::array", *this};
            },
            value_);
    }

    inline std::string_view value::_content() const noexcept
    {
        return std::visit(
            []<typename Content>(Content&& arg) -> std::string_view {
                using T = std::decay_t<Content>;
                if constexpr (std::is_same_v<T, detail::string> || std::is_same_v<T, detail::number>)
                    return arg._content();
                else
                    return {};
            },
            value_);
    }

    inline bool value::empty() const noexcept
    {
        return std::visit(
            []<typename Content>(Content&& arg) {
                using T = std::decay_t<Content>;
                if constexpr (std::is_same_v<T, detail::object> || std::is_same_v<T, detail::array> || std::is_same_v<T, detail::string>)
                    return arg.empty();
                else if (std::is_same_v<T, detail::null>)
                    return true;
                else
                    return false;
            },
            value_);
    }

    inline size_t value::size() const noexcept
    {
        return std::visit(
            []<typename Content>(Content&& arg) -> size_t {
                using T = std::decay_t<Content>;
                if constexpr (std::is_same_v<T, detail::object> || std::is_same_v<T, detail::array> || std::is_same_v<T, detail::string>)
                    return arg.size();
                else
                    return 0;
            },
            value_);
    }

    inline bool value::is_null() const noexcept
    {
        return std::visit(
            []<typename Content>(Content&&) {
                if constexpr (std::is_same_v<std::decay_t<Content>, detail::null>)
                    return true;
                else
                    return false;
            },
            value_);
    }

    inline bool value::is_object() const noexcept
    {
        return std::visit(
            []<typename Content>(Content&&) {
                if constexpr (std::is_same_v<std::decay_t<Content>, detail::object>)
                    return true;
                else
                    return false;
            },
            value_);
    }

    inline bool value::is_array() const noexcept
    {
        return std::visit(
            []<typename Content>(Content&&) {
                if constexpr (std::is_same_v<std::decay_t<Content>, detail::array>)
                    return true;
                else
                    return false;
            },
            value_);
    }

    inline bool value::is_string() const noexcept
    {
        return std::visit(
            []<typename Content>(Content&&) {
                if constexpr (std::is_same_v<std::decay_t<Content>, detail::string>)
                    return true;
                else
                    return false;
            },
            value_);
    }

    inline bool value::is_number() const noexcept
    {
        return std::visit(
            []<typename Content>(Content&&) {
                if constexpr (std::is_same_v<std::decay_t<Content>, detail::number>)
                    return true;
                else
                    return false;
            },
            value_);
    }

    inline bool value::is_bool() const noexcept
    {
        return std::visit(
            []<typename Content>(Content&&) {
                if constexpr (std::is_same_v<std::decay_t<Content>, detail::boolean>)
                    return true;
                else
                    return false;
            },
            value_);
    }

    // ----------------------------------------------------------------------

    inline void detail::object::insert(std::string_view aKey, value&& aValue) { content_.emplace_not_replace(aKey, std::move(aValue)); }

    inline const value& detail::object::operator[](std::string_view key) const noexcept
    {
        if (const auto found = content_.find(key); found != content_.end())
            return found->second;
        else
            return const_null;
    }

    template <typename... Keys> inline const value& detail::object::get(std::string_view key, Keys&&... rest) const
    {

        if (const auto& r1 = operator[](key); !r1.is_null()) {
            if constexpr (sizeof...(rest) > 0)
                return r1.get(rest...);
            else
                return r1;
        }
        else
            return r1;
    }

    inline void detail::array::append(value&& aValue) { content_.push_back(std::move(aValue)); }

    inline const value& detail::array::operator[](size_t index) const noexcept
    {
        if (index < content_.size())
            return content_[index];
        else
            return const_null;
    }

} // namespace rjson::v3

// ----------------------------------------------------------------------

// "{}" -> compact with spaces
// "{:4}" -> pretty(4)
// "{:0}" -> compact without spaces
// "{:c}" -> compact with spaces

template <> struct fmt::formatter<rjson::v3::value>
{
    template <typename ParseContext> auto parse(ParseContext& ctx) -> decltype(ctx.begin())
    {
        auto it = ctx.begin();
        if (!it)
        if (it != ctx.end() && *it == ':')
            ++it;
        if (it != ctx.end() && *it != '}') {
            char* end;
            indent_ = std::strtoul(&*it, &end, 10);
            if (&*it > end) {
                it = std::next(it, end - &*it);
                if (indent_ == 0)
                    output_ = rjson::v3::output::compact;
            }
        }
        if (it != ctx.end() && *it == 'c') {
            output_ = rjson::v3::output::compact_with_spaces;
            ++it;
        }
        while (it != ctx.end() && *it != '}')
            ++it;
        return it;
    }

    template <typename FormatCtx> auto format(const rjson::v3::value& value, FormatCtx& ctx) const { return format_to(ctx.out(), "{}", rjson::v3::format(value, output_, indent_)); }

  private:
    rjson::v3::output output_{rjson::v3::output::compact_with_spaces};
    size_t indent_{0};
};

template <> struct fmt::formatter<rjson::v3::value_read> : fmt::formatter<rjson::v3::value>
{
};

template <> struct fmt::formatter<rjson::v3::detail::null> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const rjson::v3::detail::null&, FormatCtx& ctx) const { return format_to(ctx.out(), "null"); }
};

template <> struct fmt::formatter<rjson::v3::detail::object> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const rjson::v3::detail::object& /*value*/, FormatCtx& ctx) const { return format_to(ctx.out(), "object"); }
};

template <> struct fmt::formatter<rjson::v3::detail::array> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const rjson::v3::detail::array& /*value*/, FormatCtx& ctx) const { return format_to(ctx.out(), "array"); }
};

template <> struct fmt::formatter<rjson::v3::detail::string> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const rjson::v3::detail::string& value, FormatCtx& ctx) const { return format_to(ctx.out(), "\"{}\"", value._content()); }
};

template <> struct fmt::formatter<rjson::v3::detail::number> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const rjson::v3::detail::number& value, FormatCtx& ctx) const { return format_to(ctx.out(), "{}", value.to<double>()); }
};

template <> struct fmt::formatter<rjson::v3::detail::boolean> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const rjson::v3::detail::boolean& value, FormatCtx& ctx) const { return format_to(ctx.out(), "{}", value.to<bool>()); }
};

// ----------------------------------------------------------------------

namespace rjson::v3
{
    inline value_type_mismatch::value_type_mismatch(std::string_view requested_type, const value& val, std::string_view source_ref)
        : error{fmt::format("value type mismatch, requested: {}, stored: {}{}", requested_type, val, source_ref)}
    {
    }

} // namespace rjson::v3

// ----------------------------------------------------------------------
