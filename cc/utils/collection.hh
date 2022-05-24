#pragma once

// Dynamic data collection which can be read from json and exported to json

#include <variant>
#include <vector>

#include "ext/fmt.hh"

// ----------------------------------------------------------------------

namespace ae
{
    namespace dynamic
    {
        struct invalid_value : public std::runtime_error
        {
            invalid_value(std::string_view text) : std::runtime_error{std::string{text}} {}
            invalid_value(const std::string& text) : std::runtime_error{text} {}
            invalid_value(std::string&& text) : std::runtime_error{std::move(text)} {}
            template <typename... Ts> invalid_value(fmt::format_string<Ts...> format, Ts&&... ts) : std::runtime_error{fmt::format(format, std::forward<Ts>(ts)...)} {}
        };

        class value;
        class object;
        class array;

        class null
        {
          public:
            // null() = default;
            bool operator==(const null&) const = default;
        };

        class string
        {
          public:
            string() = default;
            string(const string&) = default;
            string(std::string_view src) : data_{src} {}
            string& operator=(const string&) = default;
            string& operator=(std::string_view src)
            {
                data_ = std::string{src};
                return *this;
            }

            bool operator==(const string&) const = default;
            operator std::string_view() const { return data_; }

          private:
            std::string data_{};
        };

        class object
        {
          public:
            object();
            bool operator==(const object&) const;
            void add(std::string_view key, value&& val) { insert_or_assign(key, std::move(val)); }
            bool has_key(std::string_view key) const;

            auto begin() const { return data_.begin(); }
            auto end() const { return data_.end(); }
            const auto& data() const { return data_; }

            value& operator[](std::string_view key) { return find_or_add(key); }
            const value& operator[](std::string_view key) const { return find_or_null(key); }

          private:
            // std::unordered_map<std::string, value, string_hash_for_unordered_map, std::equal_to<>> data_; // g++11 cannot handle value declared after this line
            std::vector<std::pair<std::string, value>> data_;

            value& find_or_add(std::string_view key);
            const value& find_or_null(std::string_view key) const;
            void insert_or_assign(std::string_view key, value&& val);
        };

        class array
        {
          public:
            array() = default;
            bool operator==(const array&) const = default;
            void add(value&& src) { data_.push_back(std::move(src)); }
            void add_if_not_present(value&& src) { if (!contains(src)) data_.push_back(std::move(src)); }
            bool contains(const value& val) const;
            // void sort_unique();

            auto begin() const { return data_.begin(); }
            auto end() const { return data_.end(); }
            const auto& data() const { return data_; }

          private:
            std::vector<value> data_{};
        };

        class value
        {
          public:
            value() noexcept : data_{null{}} {}
            value(value&& arg) : data_{std::move(arg.data_)} {}
            value(const value& arg) : data_{arg.data_} {}
            template <typename T>
            requires(!std::is_same_v<T, value>) value(T&& arg) : data_{std::move(arg)} {}
            value(std::string_view arg) : data_{string{arg}} {}
            value(const std::string& arg) : data_{string{arg}} {}
            value& operator=(value&& arg) = default; // { data_ = std::move(arg.data_); return *this; }
            value& operator=(const value& arg) = default;
            template <typename T>
            requires(!std::is_same_v<T, value>) value& operator=(T&& arg)
            {
                data_ = arg;
                return *this;
            }
            bool operator==(const value&) const = default;
            bool empty() const { return std::holds_alternative<null>(data_); }
            bool is_null() const { return std::holds_alternative<null>(data_); }
            bool is_object() const { return std::holds_alternative<object>(data_); }
            bool is_array() const { return std::holds_alternative<array>(data_); }
            const char* typeid_content() const { return std::visit([](auto&& content) { return typeid(content).name(); }, data_); }

            using value_t = std::variant<long, double, bool, string, object, array, null>;

            value& as_object()
            {
                if (is_null())
                    data_ = object{};
                else if (!is_object())
                    throw invalid_value{"dynamic::value::as_object() requires this to be object or null, but this is {}", typeid_content()};
                return *this;
            }

            // if this is null, make it object and add array under key
            // if this[key] is null, make it array
            // if this is neither null nor object or this[key] is neither null nor array, throw invalid_value
            value& as_array(std::string_view key)
            {
                as_object();
                auto& val = operator[](key);
                if (val.is_null())
                    val = array{};
                else if (!val.is_array())
                    throw invalid_value{"dynamic::value::as_array(key) requires this[\"{}\"] to be array or null, but this[\"{}\"] is {}", key, key, val.typeid_content()};
                return val;
            }

            // if this is null, make it object and add array under key
            // if this[key] is null, make it object
            // if this is neither null nor object or this[key] is neither null nor object, throw invalid_value
            value& as_object(std::string_view key)
            {
                as_object();
                auto& val = operator[](key);
                if (val.is_null())
                    val = object{};
                else if (!val.is_object())
                    throw invalid_value{"dynamic::value::as_object(key) requires this[\"{}\"] to be object or null, but this[\"{}\"] is {}", key, key, val.typeid_content()};
                return val;
            }

            value& operator[](std::string_view key)
            {
                as_object();
                return std::visit(
                    [key]<typename T>(T& content) -> value& {
                        if constexpr (std::is_same_v<T, object>)
                            return content[key];
                        else
                            throw invalid_value{"dynamic::value::operator[] cannot be used with variant value of type {}", typeid(content).name()};
                    },
                    data_);
            }

            const value& operator[](std::string_view key) const
            {
                return std::visit(
                    [key, this]<typename T>(const T& content) -> const value& {
                        if constexpr (std::is_same_v<T, object>)
                            return content[key];
                        else if constexpr (std::is_same_v<T, null>)
                            return *this;
                        else
                            throw invalid_value{"dynamic::value::operator[] const cannot be used with variant value of type {}", typeid(content).name()};
                    },
                    data_);
            }

            void add(value&& to_add)
            {
                std::visit(
                    [&to_add]<typename T>(T& content) {
                        if constexpr (std::is_same_v<T, array>)
                            return content.add(std::move(to_add));
                        else
                            throw invalid_value{"dynamic::value::add(value) cannot be used with variant value of type {}", typeid(content).name()};
                    },
                    data_);
            }

            void add_if_not_present(value&& to_add)
            {
                std::visit(
                    [&to_add]<typename T>(T& content) {
                        if constexpr (std::is_same_v<T, array>)
                            return content.add_if_not_present(std::move(to_add));
                        else
                            throw invalid_value{"dynamic::value::add_if_not_present(value) cannot be used with variant value of type {}", typeid(content).name()};
                    },
                    data_);
            }

            template <typename T> bool contains(const T& val) const
            {
                return std::visit(
                    [&val]<typename C>(const C& content) -> bool {
                        if constexpr (std::is_same_v<C, object>)
                            return content.has_key(val);
                        else if constexpr (std::is_same_v<C, array>)
                            return content.contains(value{val});
                        else if constexpr (std::is_same_v<C, null>)
                            return false;
                        else
                            throw invalid_value{"dynamic::value::contains() const cannot be used with variant value of type {}", typeid(content).name()};
                    },
                    data_);
            }

            std::string_view as_string_or_empty() const
                {
                return std::visit(
                    []<typename C>(const C& content) -> std::string_view {
                        if constexpr (std::is_same_v<C, string>)
                            return content;
                        else if constexpr (std::is_same_v<C, null>)
                            return std::string_view{};
                        else
                            throw invalid_value{"dynamic::value::as_string_or_empty() const cannot be used with variant value of type {}", typeid(content).name()};
                    },
                    data_);
                }

            const value_t& data() const { return data_; }
            operator const value_t&() const { return data_; }

          private:
            value_t data_;

            friend class DynamicCollectionJsonLoader;
        };

        // ----------------------------------------------------------------------

        class value_null : public value
        {
          public:
            value_null() = default;
        };

        extern value_null static_null;

        // ----------------------------------------------------------------------

        inline object::object() : data_{} {} // g++11 wants this
        inline bool object::operator==(const object& rhs) const { return data_ == rhs.data_; }

        inline bool object::has_key(std::string_view key) const
        {
            return std::find_if(std::begin(data_), std::end(data_), [key](const auto& en) { return en.first == key; }) != std::end(data_);
        }

        inline value& object::find_or_add(std::string_view key)
        {
            if (const auto found = std::find_if(std::begin(data_), std::end(data_), [key](const auto& en) { return en.first == key; }); found != std::end(data_))
                return found->second;
            return data_.emplace_back(key, null{}).second;
        }

        inline const value& object::find_or_null(std::string_view key) const
        {
            if (const auto found = std::find_if(std::begin(data_), std::end(data_), [key](const auto& en) { return en.first == key; }); found != std::end(data_))
                return found->second;
            return static_null;
        }

        inline void object::insert_or_assign(std::string_view key, value&& val)
        {
            if (const auto found = std::find_if(std::begin(data_), std::end(data_), [key](const auto& en) { return en.first == key; }); found != std::end(data_))
                found->second = std::move(val);
            data_.emplace_back(key, std::move(val));
        }

        // inline array::array() {}
        inline bool array::contains(const value& val) const { return std::find(data_.begin(), data_.end(), val) != data_.end(); }

        // inline void array::sort_unique() { std::sort(data_.begin(), data_.end()); data_.erase(std::unique(data_.begin(), data_.end()), data_.end()); }

    } // namespace dynamic

    // ----------------------------------------------------------------------

    using DynamicCollection = dynamic::value;

} // namespace ae

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::dynamic::value> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::dynamic::value& val, FormatCtx& ctx)
    {
        return std::visit([&ctx](const auto& content) { return format_to(ctx.out(), "{}", content); }, val.data());
    }
};

template <> struct fmt::formatter<ae::dynamic::object> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::dynamic::object& obj, FormatCtx& ctx)
    {
        format_to(ctx.out(), "{{");
        bool comma = false;
        for (const auto& field : obj) {
            if (comma)
                format_to(ctx.out(), ",");
            else
                comma = true;
            format_to(ctx.out(), "\"{}\":{}", field.first, field.second);
        }
        format_to(ctx.out(), "}}");
        return ctx.out();
    }
};

template <> struct fmt::formatter<ae::dynamic::array> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::dynamic::array& arr, FormatCtx& ctx)
    {
        return format_to(ctx.out(), fmt::runtime("[{}]"), fmt::join(arr.data(), ","));
    }
};

template <> struct fmt::formatter<ae::dynamic::null> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> constexpr auto format(const ae::dynamic::null&, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "null");
    }
};

template <> struct fmt::formatter<ae::dynamic::string> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> constexpr auto format(const ae::dynamic::string& str, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "\"{}\"", static_cast<std::string_view>(str));
    }
};

// ======================================================================
