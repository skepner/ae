#pragma once

// ----------------------------------------------------------------------
// Before <=> operator can be used
// To avoid warnings about ==  and !=
// ISO C++20 considers use of overloaded operator '==' (with operand types 'rjson::v2::value' and 'rjson::v2::value') to be ambiguous despite

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wambiguous-reversed-operator"
#endif

// ----------------------------------------------------------------------

#include <variant>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <limits>

#include "ad/sfinae.hh"
#include "ad/format-double.hh"
#include "utils/log.hh"
#include "utils/string.hh"
#include "ad/enumerate.hh"
#include "utils/float.hh"
#include "ext/fmt.hh"
#include "ad/rjson-forward.hh"

// ----------------------------------------------------------------------

namespace rjson::inline v2
{
    class value;
    class array;
    class object;
    class PrettyHandler;

    enum class emacs_indent { no, yes };
    enum class space_after_comma { no, yes };
    enum class show_empty_values { no, yes };

    // --------------------------------------------------

    class error : public std::runtime_error
    {
      public:
        using std::runtime_error::runtime_error;
    };

    class value_type_mismatch : public error
    {
      public:
        value_type_mismatch(std::string_view requested_type, std::string_view actual_type, const char* file = __builtin_FILE(), int line = __builtin_LINE(),
                            const char* function = __builtin_FUNCTION())
            : error{fmt::format("value type mismatch, requested: {}, stored: {} @@ {}:{}: {}", requested_type, actual_type, file, line, function)}
        {
        }
    };

    class merge_error : public error
    {
      public:
        using error::error;
    };

    class array_index_out_of_range : public error
    {
      public:
        array_index_out_of_range() : error{"array_index_out_of_range"} {}
    };

    class const_null_modification_attempt : public error
    {
      public:
        const_null_modification_attempt() : error{"ConstNull modification attempt"} {}
    };

    // --------------------------------------------------

    class null
    {
    };

    class const_null
    {
    };

    // --------------------------------------------------

    class object
    {
      public:
        using content_t = std::map<std::string, value>;
        using value_type = typename content_t::value_type;
        using value_type_init = std::pair<std::string_view, value>;

        object() = default;
        object(std::initializer_list<value_type_init> key_values);

        bool empty() const noexcept { return content_.empty(); }
        size_t size() const noexcept { return content_.size(); }

        template <typename S> const value& get(S key) const noexcept;
        template <typename S> value& operator[](S key) noexcept;
        size_t max_index() const;

        void insert(value&& aKey, value&& aValue);
        template <typename S> void insert(S aKey, const value& aValue);
        template <typename S> void remove(S key);
        void update(const object& to_merge);
        void clear();

        void remove_comments();

        template <typename Func> inline bool all_of(Func func) const { return std::all_of(content_.begin(), content_.end(), func); }

        template <typename T> void copy_to(T&& target) const;
        template <typename T, typename F> void transform_to(T&& target, F&& transformer) const;
        template <typename F> void for_each(F&& func) const;
        template <typename F> void for_each(F&& func);

      private:
        content_t content_;

        friend std::string format(const object& val, space_after_comma, const PrettyHandler&, show_empty_values);
        friend std::string pretty(const object& val, emacs_indent, const PrettyHandler&, size_t prefix);
        friend class PrettyHandler;

    }; // class object

    using key_value_t = object::value_type;

    // --------------------------------------------------

    class array
    {
      public:
        array() = default;
        array(std::initializer_list<value> init) : content_(init) {}
        template <typename Iterator> array(Iterator first, Iterator last) : content_(first, last) {}

        bool empty() const noexcept { return content_.empty(); }
        size_t size() const noexcept { return content_.size(); }

        const value& get(size_t index) const noexcept; // if index out of range, returns ConstNull
        value& operator[](size_t index) noexcept;      // if index out of range, returns ConstNull
        size_t max_index() const { return content_.size() - 1; }

        value& append(value&& aValue); // returns ref to inserted
        void replace(const array& to_replace);
        void remove(size_t index);
        void clear();

        void remove_comments();

        template <typename Func> inline bool all_of(Func func) const { return std::all_of(content_.begin(), content_.end(), func); }

        template <typename T> void copy_to(T&& target) const;
        template <typename T, typename F> void transform_to(T&& target, F&& transformer) const;
        template <typename F> array map(F&& func) const;
        template <typename F> void for_each(F&& func) const;
        template <typename F> void for_each(F&& func);
        template <typename Func> const value& find_if(Func&& func) const; // returns ConstNull if not found, Func: bool (const value&)
        template <typename Func> value& find_if(Func&& func);             // returns ConstNull if not found, Func: bool (value&)
        template <typename Func> std::optional<size_t> find_index_if(Func&& func) const;

      private:
        std::vector<value> content_;

        friend std::string format(const array& val, space_after_comma, const PrettyHandler&, show_empty_values);
        friend std::string pretty(const array& val, emacs_indent, const PrettyHandler&, size_t prefix);
        friend class PrettyHandler;

    }; // class array

    // --------------------------------------------------

    using number = std::variant<long, double, std::string>;

    // --------------------------------------------------

    class value
    {
      private:
        template <typename T, typename = std::enable_if_t<std::is_same_v<std::decay_t<T>, char>>> value& assign(acmacs::sfinae::dispatching_priority<5>, T&& src)
        {
            check_const_null();
            value_ = std::string(1, src);
            return *this;
        }

        template <typename T, typename = std::enable_if_t<(acmacs::sfinae::decay_equiv_v<T, std::string_view> || acmacs::sfinae::is_const_char_ptr_v<T>)>>
        value& assign(acmacs::sfinae::dispatching_priority<4>, T&& src)
        {
            check_const_null();
            value_ = std::string(std::forward<T>(src));
            return *this;
        }

        template <typename Float, typename = std::enable_if_t<std::is_floating_point_v<std::decay_t<Float>>>> value& assign(acmacs::sfinae::dispatching_priority<2>, Float&& src)
        {
            check_const_null();
            value_ = number(static_cast<double>(src));
            return *this;
        }

        template <typename Int, typename = std::enable_if_t<(std::is_integral_v<std::decay_t<Int>> && !std::is_same_v<std::decay_t<Int>, bool> && !std::is_same_v<std::decay_t<Int>, char>)>>
        value& assign(acmacs::sfinae::dispatching_priority<1>, Int&& src)
        {
            check_const_null();
            value_ = number(static_cast<long>(src));
            return *this;
        }

        value& assign(acmacs::sfinae::dispatching_priority<0>, const value& src)
        {
            check_const_null();
            value_ = src.value_;
            return *this;
        }

        value& assign(acmacs::sfinae::dispatching_priority<0>, value&& src)
        {
            check_const_null();
            value_ = std::move(src.value_);
            return *this;
        }

        template <typename T, typename = std::enable_if_t<(acmacs::sfinae::decay_equiv_v<T, bool> || std::is_base_of_v<std::string, std::decay_t<T>> || acmacs::sfinae::decay_equiv_v<T, object> ||
                                                           acmacs::sfinae::decay_equiv_v<T, array> || acmacs::sfinae::decay_equiv_v<T, number> || acmacs::sfinae::decay_equiv_v<T, null> ||
                                                           acmacs::sfinae::decay_equiv_v<T, const_null>)>>
        value& assign(acmacs::sfinae::dispatching_priority<0>, T&& src)
        {
            check_const_null();
            value_ = std::forward<T>(src);
            return *this;
        }

      public:
        value() = default;
        value(const value&) = default;
        value(value&&) = default;
        value(std::string_view src) : value_(std::string(src)) {}
        value(const char* src) : value_(std::string(src)) {}
        // explicit value(char* src) : value_(std::string(src)) {}
        value(bool src) : value_(src) {}
        value(null src) : value_(src) {}
        value(const_null src) : value_(src) {}
        value(const object& src) : value_(src) {}
        value(object&& src) : value_(std::move(src)) {}
        value(const array& src) : value_(src) {}
        value(array&& src) : value_(std::move(src)) {}
        value(const std::string& src) : value_(src) {}
        value(std::string&& src) : value_(std::move(src)) {}
        value(number src) : value_(src) {}

        template <typename Uint, typename std::enable_if<std::is_integral<Uint>::value>::type* = nullptr> value(Uint src) : value_(number(static_cast<long>(src))) {}
        template <typename Dbl, typename std::enable_if<std::is_floating_point<Dbl>::value>::type* = nullptr> value(Dbl src) : value_(number(static_cast<double>(src))) {}

        value& operator=(const value& src) { return assign(acmacs::sfinae::dispatching_priority_top{}, src); }
        value& operator=(value&& src) { return assign(acmacs::sfinae::dispatching_priority_top{}, std::move(src)); }
        template <typename T> value& operator=(T&& src) { return assign(acmacs::sfinae::dispatching_priority_top{}, std::forward<T>(src)); }

        template <typename T> bool operator==(const T& to_compare) const { return to<std::decay_t<T>>() == to_compare; }
        // template <> bool operator==(const value& to_compare) const;
        // bool operator!=(const value& to_compare) const { return !operator==(to_compare); }
        bool operator==(const char* to_compare) const { return operator==(std::string_view{to_compare}); }
        template <typename T> bool operator!=(const T& to_compare) const { return !operator==(to_compare); }

        bool is_null() const noexcept;
        bool is_object() const noexcept;
        bool is_array() const noexcept;
        bool is_string() const noexcept;
        bool is_number() const noexcept;
        bool is_bool() const noexcept;
        bool is_const_null() const noexcept;

        bool empty() const noexcept;
        size_t size() const noexcept; // returns 0 if neither array nor object nor string
        template <typename S, typename = std::enable_if_t<acmacs::sfinae::is_string_v<S>>>
        value& operator[](S field_name);               // if this is not object, throws value_type_mismatch; if field not present, inserts field with null value and returns it
        const value& get(size_t index) const noexcept; // if this is not array or index out of range, returns ConstNull
        template <typename S, typename... Args, typename = std::enable_if_t<acmacs::sfinae::is_string_v<S>>> const value& get(S field_name, Args&&... args) const noexcept;
        template <typename S, typename... Args, typename = std::enable_if_t<acmacs::sfinae::is_string_v<S>>> value& set(S field_name, Args&&... args); // creates intermediate objects, if necessary
        template <typename S, typename = std::enable_if_t<acmacs::sfinae::is_string_v<S>>> void remove(S field_name); // does nothing if field is not present, throws if this is not an object
        value& operator[](size_t index); // if this is neither array nor object or index is out of range, throws value_type_mismatch
        const value& operator[](size_t index) const noexcept { return get(index); }
        void remove(size_t index); // if this is not array or index out of range, throws
        void clear();              // if this is neither array nor object, throws
        template <typename S, typename = std::enable_if_t<acmacs::sfinae::is_string_v<S>>> const value& operator[](S field_name) const noexcept { return get(field_name); }
        value& append(value&& aValue); // for array only, returns ref to inserted
        value& append(double aValue) { return append(number(aValue)); }
        size_t max_index() const; // returns (size-1) for array, assumes object keys are size_t and returns max of them

        template <typename T> T to() const;

        template <typename R> R get_or_default(R&& dflt) const;
        std::string get_or_default(std::string_view dflt) const { return get_or_default(std::string(dflt)); }
        std::string get_or_default(const char* dflt) const { return get_or_default(std::string(dflt)); }

        value& update(const value& to_merge);
        void remove_comments();

        std::string actual_type() const;

        constexpr auto& val_() { return value_; }
        constexpr const auto& val_() const { return value_; }

      private:
        using value_base = std::variant<null, const_null, object, array, std::string, number, bool>; // null must be the first alternative, it is the default value;

        value_base value_;

        template <typename S> const value& get1(S field_name) const noexcept; // if this is not object or field not present, returns ConstNull
//        template <typename T> T to_integer() const;
        void check_const_null() const
        {
            if (is_const_null())
                throw const_null_modification_attempt{};
        }

    }; // class value

    // --------------------------------------------------

    extern value ConstNull, EmptyArray, EmptyObject;

    // --------------------------------------------------

} // namespace rjson::inlinev2

// ----------------------------------------------------------------------

namespace rjson::inline v2
{
    // ----------------------------------------------------------------------
    // rjson::to - assign T from rjson::value, specify to classes, e.g. Color
    // ----------------------------------------------------------------------

    enum class ignore_null { no, yes };

    // using value_base = std::variant<null, const_null, object, array, std::string, number, bool>; // null must be the first alternative, it is the default value;

    template <typename Target> constexpr void to(null, Target& target)
    {
        if constexpr (std::is_assignable_v<Target, const char*>)
            target = "null";
        else
            throw value_type_mismatch(typeid(Target).name(), "rjson::null");
    }

    template <typename Target> constexpr void to(const_null, Target& target)
    {
        if constexpr (std::is_assignable_v<Target, const char*>)
            target = "*ConstNull";
        else
            throw value_type_mismatch(typeid(Target).name(), "rjson::const_null");
    }

    std::string format(const object& val, show_empty_values a_show_empty_values = show_empty_values::yes);

    template <typename Target> constexpr void to(const object& source, Target& target)
    {
        if constexpr (std::is_assignable_v<Target, std::string>)
            target = format(source, show_empty_values::yes);
        else
            throw value_type_mismatch(typeid(Target).name(), "rjson::object");
    }

    std::string format(const array& val, show_empty_values a_show_empty_values = show_empty_values::yes);

    template <typename Target> void to(const array& source, Target& target)
    {
        if constexpr (std::is_assignable_v<Target, std::string>)
            target = format(source, show_empty_values::yes);
        else if constexpr (acmacs::sfinae::container_has_iterator_v<Target>)
            source.copy_to(target);
        else
            throw value_type_mismatch(typeid(Target).name(), "rjson::array");
    }

    template <typename Target> void to(const std::string& source, Target& target)
    {
        // if constexpr (std::is_same_v<Target, std::string_view>)
        //     target = source;
        // else
        if constexpr (std::is_assignable_v<Target, std::string>)
            target = source; // fmt::format("\"{}\"", source);
        else
            throw value_type_mismatch(typeid(Target).name(), "rjson::string");
    }

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wimplicit-int-float-conversion"
#endif
    template <typename Target> void to(const number& source, Target& target)
    {
        std::visit(
            [&]<typename Number>(const Number& from) {
                if constexpr (std::is_same_v<Target, std::string>) {
                    if constexpr (std::is_same_v<Number, std::string>)
                        target = from;
                    else if constexpr (std::is_same_v<Number, double>)
                        target = acmacs::format_double(from);
                    else
                        target = fmt::format("{}", from);
                }
                else if constexpr (std::is_same_v<Target, std::string_view>)
                    throw value_type_mismatch("std::string_view", "rjson::number");
                else if constexpr (std::is_assignable_v<Target, Number> || std::is_convertible_v<Number, Target>)
                    target = static_cast<Target>(from);
                else if constexpr (std::is_same_v<Number, std::string>) {
                    if constexpr (std::is_integral_v<Target>)
                        target = static_cast<Target>(std::stoul(from));
                    else if constexpr (std::is_floating_point_v<Target> || std::is_assignable_v<Target, double>)
                        target = static_cast<Target>(std::stod(from));
                    else if constexpr (std::is_constructible_v<Target, double>)
                        target = Target{std::stod(from)};
                    else if constexpr (std::is_constructible_v<Target, long>)
                        target = Target{std::stol(from)};
                    else if constexpr (std::is_constructible_v<Target, Number>)
                        target = Target{from}; // for named_string_t
                    else
                        throw value_type_mismatch(typeid(Target).name(), fmt::format("rjson::number<std::string>({})", from));
                }
                else
                    throw value_type_mismatch(typeid(Target).name(), fmt::format(fmt::runtime("rjson::number({})"), source));
            },
            source);
    }
#pragma GCC diagnostic pop

    template <typename Target> void to(bool source, Target& target)
    {
        if constexpr (std::is_integral_v<Target>)
            target = source;
        else if constexpr (std::is_assignable_v<Target, std::string>)
            target = fmt::format("{}", source);
        else
            throw value_type_mismatch(typeid(Target).name(), fmt::format("rjson::bool({})", source));
    }

    template <typename Target> void to(const value& source, Target& target, ignore_null ign = ignore_null::no)
    {
        std::visit(
            [&target,ign]<typename Value>(const Value& arg) {
                if constexpr (std::is_same_v<Value, null> || std::is_same_v<Value, const_null>) {
                    if (ign == ignore_null::no)
                        to(arg, target);
                }
                else {
                    to(arg, target);
                }
            },
            source.val_());
    }

    template <typename To, typename From = value> To to(const From& source)
    {
        To target;
        to(source, target, ignore_null::no);
        return target;
    }

    template <> inline value to<value, value>(const value& source) { return source; }

    // ----------------------------------------------------------------------

    template <typename Target> inline Target value::to() const { return rjson::to<Target>(*this); }

    // ----------------------------------------------------------------------

    inline std::string format(const value& val, show_empty_values a_show_empty_values = show_empty_values::yes)
    {
        return std::visit(
            [=]<typename T>(const T& arg) -> std::string {
                if constexpr (std::is_same_v<T, null> || std::is_same_v<T, const_null> || std::is_same_v<T, number> || std::is_same_v<T, bool>)
                    return to<std::string>(arg);
                else if constexpr (std::is_same_v<T, std::string>)
                    return fmt::format("\"{}\"", arg);
                else
                    return format(arg, a_show_empty_values);
            },
            val.val_());
    }

} // namespace rjson::inlinev2

// ======================================================================

template <typename T> struct fmt::formatter<T, std::enable_if_t<
                                                      std::is_base_of_v<rjson::object, T>
                                                   || std::is_base_of_v<rjson::array, T>
                                                   || std::is_base_of_v<rjson::null, T>
                                                   || std::is_base_of_v<rjson::const_null, T>
                                                   || std::is_base_of_v<rjson::number, T>
                                                   || std::is_base_of_v<bool, T>
                                                   , char>> : fmt::formatter<std::string> {
    template <typename FormatCtx> auto format(const T& val, FormatCtx& ctx) const { return fmt::formatter<std::string>::format(rjson::to<std::string>(val), ctx); }
};

template <typename T> struct fmt::formatter<T, std::enable_if_t<std::is_same_v<rjson::value, T>, char>> : fmt::formatter<std::string>
// template <typename T> requires std::is_same_v<rjson::value, T> struct fmt::formatter<T> : fmt::formatter<std::string>
{
    template <typename FormatCtx> auto format(const T& val, FormatCtx& ctx) const { return fmt::formatter<std::string>::format(rjson::format(val), ctx); }
};

// ======================================================================

namespace rjson::inline v2
{
    inline const value& array::get(size_t index) const noexcept // if index out of range, returns ConstNull
    {
        if (index < content_.size())
            return content_[index];
        else
            return ConstNull;
    }

    inline value& array::operator[](size_t index) noexcept // if index out of range, returns ConstNull
    {
        if (index < content_.size())
            return content_[index];
        else
            return ConstNull;
    }

    inline value& array::append(value && aValue)
    {
        content_.push_back(std::move(aValue));
        return content_.back();
    }

    inline void array::remove(size_t index)
    {
        if (index >= content_.size())
            throw array_index_out_of_range{};
        content_.erase(content_.begin() + static_cast<decltype(content_.begin() - content_.begin())>(index));
    }

    inline void array::replace(const array& to_replace)
    {
        content_.resize(to_replace.size());
        std::copy(to_replace.content_.begin(), to_replace.content_.end(), content_.begin());
    }

    inline void array::clear() { content_.clear(); }

    inline void array::remove_comments()
    {
        std::for_each(content_.begin(), content_.end(), [](auto& val) { val.remove_comments(); });
    }

    template <typename T> inline void array::copy_to(T&& target) const
    {
        if constexpr (acmacs::sfinae::container_has_iterator_v<T>) {
            if constexpr (acmacs::sfinae::container_has_resize_v<T>)
                target.resize(size());
            using dest_t = decltype(*target.begin());
            if constexpr (std::is_convertible_v<const value&, dest_t>)
                std::transform(content_.begin(), content_.end(), target.begin(), [](const value& val) -> dest_t { return val; });
            else
                std::transform(content_.begin(), content_.end(), target.begin(), [](const value& val) -> std::decay_t<dest_t> { return val.to<std::decay_t<dest_t>>(); });
        }
        else if constexpr (acmacs::sfinae::is_iterator_v<T>) {
            std::transform(content_.begin(), content_.end(), std::forward<T>(target),
                           [](const value& val) -> std::remove_reference_t<decltype(*target)> { return rjson::to<std::remove_reference_t<decltype(*target)>>(val); });
        }
        else {
            std::transform(content_.begin(), content_.end(), std::forward<T>(target),
                           [](const value& val) -> std::remove_reference_t<decltype(target)> { return rjson::to<std::remove_reference_t<decltype(target)>>(val); });
        }
    }

    template <typename F> inline void array::for_each(F && func) const
    {
        for (auto [no, val] : acmacs::enumerate(content_)) {
            if constexpr (std::is_invocable_v<F, const value&>)
                func(val);
            else if constexpr (std::is_invocable_v<F, const value&, size_t>)
                func(val, no);
            else
                static_assert(std::is_invocable_v<F, const value&>, "array::for_each: unsupported func signature");
        }
    }

    template <typename F> inline void array::for_each(F && func)
    {
        for (auto [no, val] : acmacs::enumerate(content_)) {
            if constexpr (std::is_invocable_v<F, value&>)
                func(val);
            else if constexpr (std::is_invocable_v<F, value&, size_t>)
                func(val, no);
            else
                static_assert(std::is_invocable_v<F, value&>, "array::for_each: unsupported func signature");
        }
    }

    template <typename F> inline array array::map(F && func) const
    {
        array result;
        for (auto [no, val] : acmacs::enumerate(content_)) {
            if constexpr (std::is_invocable_v<F, value&>)
                result.append(func(val));
            else if constexpr (std::is_invocable_v<F, value&, size_t>)
                result.append(func(val, no));
            else
                static_assert(std::is_invocable_v<F, value&>, "array::for_each: unsupported func signature");
        }
        return result;
    }

    template <typename T, typename F> inline void array::transform_to(T && target, F && transformer) const
    {
        if constexpr (std::is_invocable_v<F, const value&>) {
            std::transform(content_.begin(), content_.end(), std::forward<T>(target), std::forward<F>(transformer));
        }
        else if constexpr (std::is_invocable_v<F, const value&, size_t>) {
            for (auto [no, src] : acmacs::enumerate(content_))
                *target++ = transformer(src, no);
        }
        else {
            static_assert(std::is_invocable_v<F, const value&>, "rjson::array::transform_to: unsupported transformer signature");
        }
    }

    template <typename Func> inline const value& array::find_if(Func && func) const
    {
        if (const auto found = std::find_if(content_.begin(), content_.end(), std::forward<Func>(func)); found != content_.end())
            return *found;
        else
            return ConstNull;
    }

    template <typename Func> inline value& array::find_if(Func && func)
    {
        if (const auto found = std::find_if(content_.begin(), content_.end(), std::forward<Func>(func)); found != content_.end())
            return *found;
        else
            return ConstNull;
    }

    template <typename Func> inline std::optional<size_t> array::find_index_if(Func && func) const
    {
        if (auto found = std::find_if(content_.begin(), content_.end(), std::forward<Func>(func)); found != content_.end())
            return static_cast<size_t>(found - content_.begin());
        else

#pragma GCC diagnostic push
#if __GNUC__ == 8 || __GNUC__ == 9
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
            return std::optional<size_t>{};
#pragma GCC diagnostic pop
    }

    // --------------------------------------------------

    // template <> inline double value::to<double>() const
    // {
    //     return std::visit(
    //         [this]<typename T>(T&& arg) -> double {
    //             if constexpr (std::is_same_v<std::decay_t<T>, number>)
    //                 return to_double(arg);
    //             else
    //                 throw value_type_mismatch("number", actual_type());
    //         },
    //         value_);
    // }

    // template <> inline std::string value::to<std::string>() const
    // {
    //     return std::visit(
    //         [this]<typename T>(T&& arg) -> std::string {
    //             if constexpr (std::is_same_v<std::decay_t<T>, std::string>)
    //                 return arg;
    //             else
    //                 throw value_type_mismatch("std::string", actual_type());
    //         },
    //         value_);
    // }

    // template <> inline std::string_view value::to<std::string_view>() const
    // {
    //     return std::visit(
    //         [this]<typename T>(T&& arg) -> std::string_view {
    //             if constexpr (std::is_same_v<std::decay_t<T>, std::string>)
    //                 return std::string_view{arg};
    //             else
    //                 throw value_type_mismatch("std::string_view", actual_type());
    //         },
    //         value_);
    // }

    // template <typename T> inline T value::to_integer() const
    // {
    //     return std::visit(
    //         [this]<typename TT>(TT&& arg) -> T {
    //             if constexpr (std::is_same_v<std::decay_t<TT>, number>)
    //                 return rjson::to_integer<T>(arg);
    //             else
    //                 throw value_type_mismatch("number", actual_type());
    //         },
    //         value_);
    // }

    // template <> inline size_t value::to<size_t>() const { return to_integer<size_t>(); }
    // template <> inline long value::to<long>() const { return to_integer<long>(); }
    // template <> inline int value::to<int>() const { return to_integer<int>(); }
    // template <> inline unsigned value::to<unsigned>() const { return to_integer<unsigned>(); }
    // template <> inline short value::to<short>() const { return to_integer<short>(); }
    // template <> inline unsigned short value::to<unsigned short>() const { return to_integer<unsigned short>(); }

    // template <> inline bool value::to<bool>() const
    // {
    //     return std::visit(
    //         [this]<typename T>(T&& arg) -> bool {
    //             if constexpr (std::is_same_v<std::decay_t<T>, bool>)
    //                 return arg;
    //             else if constexpr (std::is_same_v<std::decay_t<T>, number>) {
    //                 const auto val = rjson::to_integer<int>(arg);
    //                 if (val != 0 && val != 1)
    //                     fmt::print(stderr, "WARNING: requested bool, stored number: {} {}\n", to_string(arg));
    //                 return val;
    //             }
    //             else
    //                 throw value_type_mismatch("bool", actual_type());
    //         },
    //         value_);
    // }

    // template <> inline std::vector<std::string_view> value::to<std::vector<std::string_view>>() const
    // {
    //     return std::visit(
    //         [this]<typename T>(T && arg)->std::vector<std::string_view> {
    //             if constexpr (std::is_same_v<std::decay_t<T>, array>) {
    //                 try {
    //                     std::vector<std::string_view> result;
    //                     arg.for_each([&result](const rjson::value& elt) { result.push_back(elt.to<std::string_view>()); });
    //                     return result;
    //                 }
    //                 catch (std::exception& /*err*/) {
    //                     throw value_type_mismatch("array of strings", actual_type());
    //                 }
    //             }
    //             else
    //                 throw value_type_mismatch("array of strings", actual_type());
    //         },
    //         value_);
    // }

    // template <> inline value value::to<value>() const { return *this; }

    // --------------------------------------------------

    inline object::object(std::initializer_list<value_type_init> key_values) : content_(std::begin(key_values), std::end(key_values)) {}

    template <typename S> inline const value& object::get(S key) const noexcept
    {
        if (const auto found = content_.find(fmt::format("{}", key)); found != content_.end())
            return found->second;
        else
            return ConstNull;
    }

    template <typename S> inline value& object::operator[](S key) noexcept { return content_.emplace(fmt::format("{}", key), value{}).first->second; }

    inline size_t object::max_index() const // assumes keys are size_t
    {
        size_t result = 0;
        for ([[maybe_unused]] const auto& [key, _] : content_)
            result = std::max(std::stoul(key), result);
        return result;
    }

    inline void object::insert(value&& aKey, value&& aValue) { content_.emplace(to<std::string>(aKey), std::move(aValue)); }

    template <typename S> inline void object::insert(S key, const value& aValue) { content_.emplace(fmt::format("{}", key), aValue); }

    template <typename S> inline void object::remove(S key)
    {
        if (const auto found = content_.find(fmt::format("{}", key)); found != content_.end())
            content_.erase(found);
    }

    inline void object::update(const object& to_merge)
    {
        for (const auto& [new_key, new_value] : to_merge.content_)
            operator[](new_key).update(new_value);
    }

    inline void object::clear() { content_.clear(); }

    inline void object::remove_comments()
    {
        auto is_comment_key = [](std::string_view key) -> bool { return !key.empty() && (key.front() == '?' || key.back() == '?'); };
        for (auto it = content_.begin(); it != content_.end(); /* no increment! */) {
            if (is_comment_key(it->first)) {
                it = content_.erase(it);
            }
            else {
                it->second.remove_comments();
                ++it;
            }
        }
    }

    template <typename F> inline void object::for_each(F && func) const
    {
        if constexpr (std::is_invocable_v<F, std::string_view, const value&> || std::is_invocable_v<F, const std::string&, const value&>)
            std::for_each(content_.begin(), content_.end(), [&func](const auto& kv) { func(kv.first, kv.second); });
        else
            std::for_each(content_.begin(), content_.end(), std::forward<F>(func));
    }

    template <typename F> inline void object::for_each(F && func) { std::for_each(content_.begin(), content_.end(), std::forward<F>(func)); }

    template <typename T, typename F> inline void object::transform_to(T && target, F && transformer) const
    {
        if constexpr (acmacs::sfinae::container_has_iterator_v<T>) {
            if constexpr (acmacs::sfinae::container_has_resize_v<T>)
                target.resize(content_.size());
            std::transform(content_.begin(), content_.end(), target.begin(), std::forward<F>(transformer));
        }
        else {
            std::transform(content_.begin(), content_.end(), std::forward<T>(target), std::forward<F>(transformer));
        }
    }

    // --------------------------------------------------

    class parse_error : public std::exception
    {
      public:
        parse_error(size_t line, size_t column, std::string_view message) : message_{fmt::format("{}:{}: {}", line, column, message)} {}
        const char* what() const noexcept override { return message_.data(); }

      private:
        std::string message_;

    }; // class parse_error

    enum class remove_comments { no, yes };

    // value parse_string(std::string data, remove_comments rc = remove_comments::yes);
    value parse_string(std::string_view data, remove_comments rc = remove_comments::yes);
    // value parse_string(const char* data, remove_comments rc = remove_comments::yes);
    value parse_file(std::string_view filename, remove_comments rc = remove_comments::yes);

    inline std::string value::actual_type() const
    {
        return std::visit(
            [](auto&& arg) -> std::string {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, null>)
                    return "null";
                else if constexpr (std::is_same_v<T, const_null>)
                    return "ConstNull";
                else if (std::is_same_v<T, object>)
                    return "object";
                else if (std::is_same_v<T, array>)
                    return "array";
                else if (std::is_same_v<T, std::string>)
                    return "std::string";
                else if (std::is_same_v<T, number>)
                    return "number";
                else if (std::is_same_v<T, bool>)
                    return "bool";
                else
                    return "*unknown*";
            },
            value_);
    }

    inline bool value::empty() const noexcept
    {
        return std::visit(
            [](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, object> || std::is_same_v<T, array> || std::is_same_v<T, std::string>)
                    return arg.empty();
                else if (std::is_same_v<T, null> || std::is_same_v<T, const_null>)
                    return true;
                else
                    return false;
            },
            value_);
    }

    inline size_t value::size() const noexcept
    {
        return std::visit(
            [](auto&& arg) -> size_t {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, object> || std::is_same_v<T, array> || std::is_same_v<T, std::string>)
                    return arg.size();
                else
                    return 0;
            },
            value_);
    }

    inline bool value::is_null() const noexcept
    {
        return std::visit(
            [](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, null> || std::is_same_v<T, const_null>)
                    return true;
                else
                    return false;
            },
            value_);
    }

    inline bool value::is_const_null() const noexcept
    {
        return std::visit(
            [](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, const_null>)
                    return true;
                else
                    return false;
            },
            value_);
    }

    inline bool value::is_object() const noexcept
    {
        return std::visit(
            []<typename T>(T&&) {
                if constexpr (std::is_same_v<std::decay_t<T>, object>)
                    return true;
                else
                    return false;
            },
            value_);
    }

    inline bool value::is_array() const noexcept
    {
        return std::visit(
            []<typename T>(T&&) {
                if constexpr (std::is_same_v<std::decay_t<T>, array>)
                    return true;
                else
                    return false;
            },
            value_);
    }

    inline bool value::is_string() const noexcept
    {
        return std::visit(
            []<typename T>(T&&) {
                if constexpr (std::is_same_v<std::decay_t<T>, std::string>)
                    return true;
                else
                    return false;
            },
            value_);
    }

    inline bool value::is_number() const noexcept
    {
        return std::visit(
            []<typename T>(T&&) {
                if constexpr (std::is_same_v<std::decay_t<T>, number>)
                    return true;
                else
                    return false;
            },
            value_);
    }

    inline bool value::is_bool() const noexcept
    {
        return std::visit(
            []<typename T>(T&&) {
                if constexpr (std::is_same_v<std::decay_t<T>, bool>)
                    return true;
                else
                    return false;
            },
            value_);
    }

    // does nothing if field is not present, throws if this is not an object
    template <typename S, typename> inline void value::remove(S field_name)
    {
        std::visit(
            [&field_name, this]<typename T>(T&& arg) -> void {
                if constexpr (std::is_same_v<std::decay_t<T>, object>)
                    arg.remove(field_name);
                else
                    throw value_type_mismatch("object", actual_type());
            },
            value_);
    }

    // if this is not array or index out of range, throws
    inline void value::remove(size_t index)
    {
        std::visit(
            [index, this]<typename T>(T&& arg) -> void {
                if constexpr (std::is_same_v<std::decay_t<T>, array>)
                    arg.remove(index);
                else
                    throw value_type_mismatch("array", actual_type());
            },
            value_);
    }

    // if this is neither array nor object, throws
    inline void value::clear()
    {
        std::visit(
            [this]<typename T>(T&& arg) -> void {
                if constexpr (std::is_same_v<std::decay_t<T>, array> || std::is_same_v<std::decay_t<T>, object>)
                    arg.clear();
                else if constexpr (std::is_same_v<std::decay_t<T>, null>)
                    ;
                else
                    throw value_type_mismatch("array or object", actual_type());
            },
            value_);
    }

    template <typename S, typename>
    inline value& value::operator[](S field_name) // if this is not object, throws value_type_mismatch; if field not present, inserts field with null value and returns it
    {
        return std::visit(
            [&field_name, this]<typename T>(T&& arg) -> value& {
                if constexpr (std::is_same_v<std::decay_t<T>, object>)
                    return arg[field_name];
                else
                    throw value_type_mismatch("object", actual_type());
            },
            value_);
    }

    template <typename S> inline const value& value::get1(S field_name) const noexcept // if this is not object or field not present, returns ConstNull
    {
        return std::visit(
            [&field_name]<typename T>(T&& arg) -> const value& {
                if constexpr (std::is_same_v<std::decay_t<T>, object>)
                    return arg.get(field_name);
                else
                    return ConstNull;
            },
            value_);
    }

    template <typename S, typename... Args, typename> inline const value& value::get(S field_name, Args && ... args) const noexcept
    {
        if (const auto& r1 = get1(field_name); !r1.is_null()) {
            if constexpr (sizeof...(args) > 0)
                return r1.get(args...);
            else
                return r1;
        }
        else
            return ConstNull;
    }

    // creates intermediate objects, if necessary.
    // if final key does not present, inserts null
    // if intermediate value exists and it is neither object nor null, throws value_type_mismatch
    template <typename S, typename... Args, typename> inline value& value::set(S field_name, Args && ... args)
    {
        auto& r1 = operator[](field_name);
        if constexpr (sizeof...(args) > 0) { // intermediate object expected
            if (r1.is_null())
                r1 = object{};
            else if (!r1.is_object())
                throw value_type_mismatch("object", actual_type());
            return r1.set(args...);
        }
        else {
            return r1;
        }
    }

    inline const value& value::get(size_t index) const noexcept // if this is not object or field not present, returns ConstNull
    {
        return std::visit(
            [index](auto&& arg) -> const value& {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, array>)
                    return arg.get(index);
                else if constexpr (std::is_same_v<T, object>)
                    return arg.get(std::to_string(index));
                else
                    return ConstNull;
            },
            value_);
    }

    inline value& value::operator[](size_t index)
    {
        return std::visit(
            [index, this](auto&& arg) -> value& {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, array>)
                    return arg[index];
                else if constexpr (std::is_same_v<T, object>)
                    return arg[std::to_string(index)];
                else
                    throw value_type_mismatch("array or object", actual_type());
            },
            value_);
    }

    inline value& value::append(value && aValue)
    {
        return std::visit(
            [&aValue, this]<typename T>(T&& arg) -> value& {
                if constexpr (std::is_same_v<std::decay_t<T>, array>)
                    return arg.append(std::move(aValue));
                else
                    throw value_type_mismatch("array", actual_type());
            },
            value_);
    }

    // ----------------------------------------------------------------------

    template <typename R> inline R value::get_or_default(R&& dflt) const
    {
        return std::visit(
            [this,&dflt]<typename T>(T&&) -> R {
                if constexpr (std::is_same_v<std::decay_t<T>, null> || std::is_same_v<std::decay_t<T>, const_null>)
                    return dflt;
                else
                    return this->to<R>();
            },
            value_);
    }

    inline value& value::update(const value& to_merge)
    {
        auto visitor = [this]<typename TT1, typename TT2>(TT1& arg1, TT2&& arg2) {
            using T1 = std::decay_t<TT1>;
            using T2 = std::decay_t<TT2>;
            if constexpr (std::is_same_v<T1, T2>) {
                if constexpr (std::is_same_v<T1, object>)
                    arg1.update(std::forward<decltype(arg2)>(arg2));
                else if constexpr (std::is_same_v<T1, array>)
                    arg1.replace(std::forward<decltype(arg2)>(arg2));
                else
                    arg1 = std::forward<decltype(arg2)>(arg2);
            }
            else if constexpr (std::is_same_v<T1, null>)
                *this = arg2;
            else if constexpr (std::is_same_v<T2, null> || std::is_same_v<T2, const_null>)
                ; // updating with null: do nothing
            else if constexpr (std::is_same_v<T1, const_null>)
                throw merge_error("cannot update ConstNull");
            else
                throw merge_error(fmt::format("cannot merge two rjson values of different types: {} and {}", arg1, arg2));
        };

        std::visit(visitor, value_, to_merge.value_);
        return *this;
    }

    inline void value::remove_comments()
    {
        std::visit(
            [](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, object> || std::is_same_v<T, array>)
                    arg.remove_comments();
            },
            value_);
    }

    inline size_t value::max_index() const
    {
        return std::visit(
            [this](auto&& arg) -> size_t {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, object> || std::is_same_v<T, array>)
                    return arg.max_index();
                else
                    throw value_type_mismatch("object or array", this->actual_type());
            },
            value_);
    }

    // ----------------------------------------------------------------------
    // ----------------------------------------------------------------------

    template <typename T> inline value to_value(T && source) { return std::forward<T>(source); }

    // ----------------------------------------------------------------------

    template <typename T> inline void copy(const value& source, T&& target)
    {
        std::visit(
            [&target, &source](auto&& arg) {
                using TT = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<TT, object>)
                    throw value_type_mismatch("array", source.actual_type());
                else if constexpr (std::is_same_v<TT, array>)
                    arg.copy_to(std::forward<T>(target));
                else if constexpr (!std::is_same_v<TT, null> && !std::is_same_v<TT, const_null>)
                    throw value_type_mismatch("object or array", source.actual_type());
            },
            source.val_());
    }

    template <typename Target> inline void copy_if_not_null(const value& source, Target& target)
    {
        to(source, target, ignore_null::yes);
    }
//        std::visit(
//            [&target, &source](auto&& arg) {
//                using TT = std::decay_t<decltype(arg)>;
//                if constexpr (std::is_same_v<TT, null> || std::is_same_v<TT, const_null>)
//                    ; // ignore nulls
//                else if constexpr (std::is_same_v<Target, TT>)
//                    target = arg;
//                else if constexpr (std::is_same_v<TT, number>) {
//                    if constexpr (std::is_same_v<Target, std::string>)
//                        target = to_string(arg);
//                    else if constexpr (std::is_same_v<Target, bool>)
//                        throw value_type_mismatch("bool", source.actual_type());
//                    else if constexpr (std::is_integral_v<Target>)
//                        target = Target{to_integer<Target>(arg)};
//                    else if constexpr (std::is_floating_point_v<Target>)
//                        target = Target{to_double(arg)};
//                    else if constexpr (std::is_constructible_v<Target, double>)
//                        target = Target{to_double(arg)};
//                    else if constexpr (std::is_constructible_v<Target, long> || std::is_constructible_v<Target, unsigned long>)
//                        target = Target{to_integer<Target>(arg)};
//                    else
//                        throw value_type_mismatch("unknown", source.actual_type());
//                }
//                else if constexpr (std::is_same_v<TT, std::string> && std::is_constructible_v<Target, TT>) {
//                    target = Target{arg};
//                }
//                else if constexpr (std::is_same_v<TT, array>) {
//                    if constexpr (std::is_same_v<Target, std::vector<double>>) {
//                        target.resize(arg.size());
//                        arg.transform_to(std::begin(target), [](const auto& val) { return val.template to<double>(); });
//                    }
//                    else if constexpr (std::is_same_v<Target, std::array<double, 2>> || std::is_same_v<Target, std::array<double, 3>> || std::is_same_v<Target, std::array<double, 4>>) {
//                        if (arg.size() != target.size())
//                            throw value_type_mismatch(fmt::format("array<double, {}>", target.size()), source.actual_type());
//                        arg.transform_to(std::begin(target), [](const auto& val) { return val.template to<double>(); });
//                    }
//                    else
//                        throw value_type_mismatch("array/vector", source.actual_type());
//                }
//                else
//                    throw value_type_mismatch("scalar", source.actual_type());
//            },
//            source.val_());
//    }

    template <typename ExtractedType, typename F> inline void call_if_not_null(const value& source, F&& callback)
    {
        if (!source.is_null())
            callback(source.template to<ExtractedType>());
    }

    template <typename T, typename F> inline void transform(const value& source, T&& target, F&& transformer)
    {
        static_assert(std::is_invocable_v<F, const key_value_t&> || std::is_invocable_v<F, std::string_view, const value&> || std::is_invocable_v<F, const value&> ||
                          std::is_invocable_v<F, const value&, size_t>,
                      "rjson::transform: unsupported transformer signature");

        // std::is_const<typename std::remove_reference<const int&>::type>::value
        std::visit(
            [&target, &transformer, &source](auto&& arg) {
                using TT = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<TT, object>) {
                    if constexpr (std::is_invocable_v<F, const key_value_t&>)
                        arg.transform_to(std::forward<T>(target), std::forward<F>(transformer));
                    else if constexpr (std::is_invocable_v<F, const std::string_view, const value&>)
                        arg.transform_to(std::forward<T>(target), [&transformer](const key_value_t& kv) { return transformer(kv.first, kv.second); });
                    else
                        throw value_type_mismatch("object and corresponding transformer", source.actual_type());
                }
                else if constexpr (std::is_same_v<TT, array> && (std::is_invocable_v<F, const value&> || std::is_invocable_v<F, const value&, size_t>))
                    arg.transform_to(std::forward<T>(target), std::forward<F>(transformer));
                else if constexpr (!std::is_same_v<TT, null> && !std::is_same_v<TT, const_null>) // do not remove, essential!
                    throw value_type_mismatch("object or array and corresponding transformer", source.actual_type());
            },
            source.val_());
    }

    template <typename Value, typename F> inline void for_each(Value && val, F && func)
    {
        static_assert(std::is_invocable_v<F, const key_value_t&> || std::is_invocable_v<F, key_value_t&> || std::is_invocable_v<F, std::string_view, const value&> ||
                          std::is_invocable_v<F, const value&> || std::is_invocable_v<F, value&> || std::is_invocable_v<F, const value&, size_t> || std::is_invocable_v<F, value&, size_t>,
                      "rjson::for_each: unsupported func signature");

        // std::is_const<typename std::remove_reference<const int&>::type>::value
        std::visit(
            [&func, &val](auto&& arg) {
                using TT = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<TT, object> && (std::is_invocable_v<F, const key_value_t&> || std::is_invocable_v<F, key_value_t&>))
                    arg.for_each(func);
                else if constexpr (std::is_same_v<TT, object> && std::is_invocable_v<F, std::string_view, const value&>)
                    arg.for_each(func);
                else if constexpr (std::is_same_v<TT, array> &&
                                   (std::is_invocable_v<F, const value&> || std::is_invocable_v<F, value&> || std::is_invocable_v<F, const value&, size_t> || std::is_invocable_v<F, value&, size_t>))
                    arg.for_each(func);
                else if constexpr (!std::is_same_v<TT, null> && !std::is_same_v<TT, const_null>) // do not remove, essential!
                    throw value_type_mismatch("object or array and corresponding callback", val.actual_type());
            },
            val.val_());
    }

    template <typename Func> inline value& find_if(value & val, Func && func) // returns ConstNull if not found, Func: bool (value&&), throws if not array
    {
        return std::visit(
            [&func, &val](auto&& arg) -> value& {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, array>)
                    return arg.find_if(std::forward<Func>(func));
                else
                    throw value_type_mismatch("array", val.actual_type());
            },
            val.val_());
    }

    template <typename Func> inline const value& find_if(const value& val, Func&& func) // returns ConstNull if not found, Func: bool (value&&), throws if not array
    {
        return std::visit(
            [&func, &val](auto&& arg) -> const value& {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, array>)
                    return arg.find_if(std::forward<Func>(func));
                else
                    throw value_type_mismatch("array", val.actual_type());
            },
            val.val_());
    }

    template <typename Value, typename Func> inline std::optional<size_t> find_index_if(const Value& val, Func&& func) // Func: bool (value&&), throws if not array
    {
        return std::visit(
            [&func, &val](auto&& arg) -> std::optional<size_t> {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, array>)
                    return arg.find_index_if(std::forward<Func>(func));
                else
                    throw value_type_mismatch("array", val.actual_type());
            },
            val.val_());
    }

    template <typename T> inline std::vector<T> as_vector(const value& source)
    {
        std::vector<T> result;
        copy(source, result);
        return result;
    }

    // ----------------------------------------------------------------------

    inline void set_field_if_not_empty(value & target, std::string_view field_name, std::string_view source)
    {
        if (!source.empty())
            target[field_name] = source;
    }

    namespace detail
    {
        template <typename T> constexpr bool equal(T first, T second)
        {
            if constexpr (std::is_same_v<T, double>)
                return float_equal(first, second);
            else
                return first == second;
        }
    } // namespace detail

    template <typename T> inline void set_field_if_not_default(value & target, std::string_view field_name, T aValue, T aDefault)
    {
        if (!detail::equal(aValue, aDefault))
            target[field_name] = aValue;
    }

    inline void set_field_if_not_default(value & target, std::string_view field_name, double aValue, double aDefault, size_t precision)
    {
        if (!detail::equal(aValue, aDefault))
            target[field_name] = number(fmt::format("{:.{}g}", aValue, precision));
    }

    template <typename Iterator> inline void set_array_field_if_not_empty(value & target, std::string_view key, Iterator first, Iterator last)
    {
        if (first != last) {
            auto& ar = target[key] = array{};
            for (; first != last; ++first)
#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wimplicit-int-float-conversion"
#endif
                ar.append(*first);
#pragma GCC diagnostic pop
        }
    }

    template <typename Container> inline void set_array_field_if_not_empty(value & target, std::string_view key, Container && container)
    {
        set_array_field_if_not_empty(target, key, std::begin(container), std::end(container));
    }

    // ----------------------------------------------------------------------

    template <typename T, typename = std::enable_if_t<!acmacs::sfinae::is_const_char_ptr_v<T>>> inline T get_or(const value& source, std::string_view field_name, T default_value)
    {
        if (source.is_null())
            return default_value;
        else if (const auto& val = source[field_name]; !val.is_null())
            if constexpr (std::is_same_v<T, std::string_view>)
                return val.to<std::string_view>();
            else
                return val.to<T>();
        else
            return default_value;
    }

    inline std::string_view get_or(const value& source, std::string_view field_name, const char* default_value)
    {
        if (source.is_null())
            return default_value;
        else if (const auto& val = source[field_name]; !val.is_null())
            return val.to<std::string_view>();
        else
            return default_value;
    }

    template <typename T> inline T get_or(const value& source, const T& default_value)
    {
        if (source.is_null())
            return default_value;
        else
            return source.to<T>();
    }

    inline std::string_view get_or(const value& source, const char* default_value)
    {
        if (source.is_null())
            return default_value;
        else
            return source.to<std::string_view>();
    }

    template <typename... Args> inline const value& one_of(const value& source, std::string_view field_name, Args... args)
    {
        if (const auto& val = source[field_name]; !val.is_null())
            return val;
        if constexpr (sizeof...(args) > 0)
            return one_of(source, args...);
        else
            return ConstNull;
    }

    template <typename T> inline void assign_if_not_null(const value& source, T& target)
    {
        if (!source.is_null())
            target = source;
    }

    template <typename T> inline void assign_string_if_not_null(const value& source, T& target)
    {
        if (!source.is_null()) {
            if constexpr (std::is_assignable_v<T, std::string_view>)
                target = source.to<std::string_view>();
            else
                target = T{source.to<std::string>()};
        }
    }

    template <typename T, typename Converter> inline void assign_if_not_null(const value& source, T& target, Converter&& converter)
    {
        if (!source.is_null())
            target = converter(source);
    }

    // ----------------------------------------------------------------------
    // to string and pretty
    // ----------------------------------------------------------------------

    class PrettyHandler
    {
      public:
        enum class dive { no, yes };
        PrettyHandler() = default;
        PrettyHandler(size_t indent) : indent_{indent} {}
        virtual ~PrettyHandler() = default;

        size_t indent() const { return indent_; }

        virtual bool is_simple(const object& val, dive a_dive) const;
        virtual bool is_simple(const array& val, dive a_dive) const;
        virtual std::vector<object::content_t::const_iterator> sorted(const object& val) const;

      protected:
        virtual bool is_simple(const value& val, dive a_dive) const;

      private:
        size_t indent_ = 2;
    };

    // ----------------------------------------------------------------------

    // ----------------------------------------------------------------------

    std::string pretty(const value& val, emacs_indent emacs_indent = emacs_indent::yes, const PrettyHandler& pretty_handler = PrettyHandler{});

    template<> inline bool value::operator==(const value& to_compare) const { return rjson::to<std::string>(*this) == rjson::to<std::string>(to_compare); }

} // namespace rjson::inlinev2

// ----------------------------------------------------------------------
