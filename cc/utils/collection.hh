#pragma once

// Dynamic data collection which can be read from json and exported to json

#include <variant>
#include <unordered_map>
#include <vector>

#include "ext/fmt.hh"
#include "utils/string-hash.hh"

// ----------------------------------------------------------------------

namespace ae
{
    namespace dynamic
    {
        struct invalid_value : public std::runtime_error { using std::runtime_error::runtime_error; };

        class value;
        class object;
        class array;

        class null
        {
          public:
            // null() = default;
            bool operator==(const null&) const = default;
        };

        class object
        {
          public:
            // object() = default;
            bool operator==(const object&) const;
            void add(std::string&& key, value&& val);
            void add(std::string_view key, value&& val);
            bool has_key(std::string_view key) const;

            auto begin() const { return data_.begin(); }
            auto end() const { return data_.end(); }
            const auto& data() const { return data_; }

            value& operator[](std::string_view key) { return data_[std::string{key}]; }
            const value& operator[](std::string_view key) const;

          private:
            std::unordered_map<std::string, value, string_hash_for_unordered_map, std::equal_to<>> data_{};
        };

        class array
        {
          public:
            array() = default;
            bool operator==(const array&) const = default;
            void add(value&& src) { data_.push_back(std::move(src)); }
            bool contains(const value& val) const;

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
            value(value&& arg): data_{std::move(arg.data_)} {}
            value(const value& arg) : data_{arg.data_} {}
            template <typename T> requires (!std::is_same_v<T, value>) value(T&& arg) : data_{std::move(arg)} {}
            value(std::string_view arg) : data_{std::string{arg}} {}
            value& operator=(value&& arg) = default; // { data_ = std::move(arg.data_); return *this; }
            value& operator=(const value& arg) = default;
            template <typename T> requires (!std::is_same_v<T, value>) value& operator=(T&& arg) { data_ = arg; return *this; }
            bool operator==(const value&) const = default;
            bool empty() const { return std::holds_alternative<null>(data_); }
            bool is_null() const { return std::holds_alternative<null>(data_); }

            using value_t = std::variant<long, double, bool, std::string, object, array, null>;

            value& operator[](std::string_view key)
            {
                return std::visit(
                    [key]<typename T>(T& content) -> value& {
                        if constexpr (std::is_same_v<T, object>)
                            // throw dynamic::invalid_value{fmt::format("dynamic::value::operator[] cannot be used with variant value of type {}", typeid(content).name())};
                            return content[key];
                        else
                            throw invalid_value{fmt::format("dynamic::value::operator[] cannot be used with variant value of type {}", typeid(content).name())};
                    },
                    data_);
            }

            const value& operator[](std::string_view key) const
            {
                return std::visit(
                    [key]<typename T>(const T& content) -> const value& {
                        if constexpr (std::is_same_v<T, object>)
                            return content[key];
                        else
                            throw invalid_value{fmt::format("dynamic::value::operator[] cannot be used with variant value of type {}", typeid(content).name())};
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
                            throw invalid_value{fmt::format("dynamic::value::add(value) cannot be used with variant value of type {}", typeid(content).name())};
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
                            throw invalid_value{fmt::format("dynamic::value::contains() cannot be used with variant value of type {}", typeid(content).name())};
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

        inline bool object::operator==(const object& rhs) const { return data_ == rhs.data_; }
        inline void object::add(std::string&& key, value&& val) { data_.insert_or_assign(std::move(key), std::move(val)); }
        inline void object::add(std::string_view key, value&& val) { data_.insert_or_assign(std::string{key}, std::move(val)); }
        inline bool object::has_key(std::string_view key) const { return data_.find(key) != data_.end(); }

        inline const value& object::operator[](std::string_view key) const
        {
            if (const auto found = data_.find(key); found != data_.end())
                return found->second;
            else
                return static_null;
        }

        // inline array::array() {}
        inline bool array::contains(const value& val) const { return std::find(data_.begin(), data_.end(), val) != data_.end(); }

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
                format_to(ctx.out(), ", ");
            else
                comma = true;
            format_to(ctx.out(), "\"{}\": {}", field.first, field.second);
        }
        format_to(ctx.out(), "}}");
        return ctx.out();
    }
};

template <> struct fmt::formatter<ae::dynamic::array> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::dynamic::array& arr, FormatCtx& ctx)
    {
        return format_to(ctx.out(), fmt::runtime("[{}]"), arr.data());
    }
};

template <> struct fmt::formatter<ae::dynamic::null> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> constexpr auto format(const ae::dynamic::null&, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "null");
    }
};

// ======================================================================
