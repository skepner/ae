#pragma once

#include <variant>
#include <string>
#include <vector>
#include <stdexcept>
#include <typeinfo>

#include "utils/string.hh"
#include "ext/fmt.hh"
#include "utils/named-type.hh"

// ----------------------------------------------------------------------

namespace acmacs::lispmds
{
    class error : public std::runtime_error { public: using std::runtime_error::runtime_error; };
    class type_mismatch : public error { public: using error::error; };
    class keyword_no_found : public error { public: using error::error; };
    class keyword_has_no_value : public error { public: using error::error; };

    class nil
    {
     public:
        nil() = default;

    }; // class nil

    class boolean
    {
     public:
        boolean(bool aValue = false) : mValue{aValue} {}

        operator bool() const { return mValue; }

     private:
        bool mValue;

    }; // class boolean

    class number
    {
     public:
        number() = default;
        number(std::string aValue) : mValue(aValue)
            {
                for (auto& c: mValue) {
                    switch (c) {
                      case 'd': case 'D':
                          c = 'e';
                          break;
                      default:
                          break;
                    }
                }
            }
        number(std::string_view aValue) : number(std::string{aValue}) {}

        operator double() const { return std::stod(mValue); }
        operator float() const { return std::stof(mValue); }
        operator unsigned long() const { return std::stoul(mValue); }
        operator long() const { return std::stol(mValue); }

     private:
        std::string mValue;

    }; // class number

    class string : public ae::named_string_t<std::string, struct lispmds_string_tag_t>
    {
      public:
        using ae::named_string_t<std::string, struct lispmds_string_tag_t>::named_string_t;
    };

    class symbol : public ae::named_string_t<std::string, struct lispmds_symbol_tag_t>
    {
      public:
        using ae::named_string_t<std::string, struct lispmds_symbol_tag_t>::named_string_t;
    };

    class keyword : public ae::named_string_t<std::string, struct lispmds_keyword_tag_t>
    {
      public:
        using ae::named_string_t<std::string, struct lispmds_keyword_tag_t>::named_string_t;
    };

    class list;

    using value = std::variant<nil, boolean, number, string, symbol, keyword, list>; // nil must be the first alternative, it is the default value;

    class list
    {
     public:
        list() = default;

        using iterator = decltype(std::declval<const std::vector<value>>().begin());
        using reverse_iterator = decltype(std::declval<const std::vector<value>>().rbegin());
        iterator begin() const { return mContent.begin(); }
        iterator end() const { return mContent.end(); }
        iterator begin() { return mContent.begin(); }
        iterator end() { return mContent.end(); }
        reverse_iterator rbegin() const { return mContent.rbegin(); }

        value& append(value&& to_add)
            {
                mContent.push_back(std::move(to_add));
                return mContent.back();
            }

        const value& operator[](size_t aIndex) const
            {
                return mContent.at(aIndex);
            }

        const value& operator[](std::string_view aKeyword) const;

        size_t size() const { return mContent.size(); }
        bool empty() const { return mContent.empty(); }

     private:
        std::vector<value> mContent{};

    }; // class list


// ----------------------------------------------------------------------

    inline value& append(value& target, value&& to_add)
    {
        return std::visit([&](auto&& arg) -> value& {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, list>)
                return arg.append(std::move(to_add));
            else
                throw type_mismatch{"not a lispmds::list, cannot append value"};
        }, target);
    }

    inline const value& get_(const value& val, size_t aIndex)
    {
        using namespace std::string_literals;
        return std::visit([aIndex](const auto& arg) -> const value& {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, list>)
                return arg[aIndex];
            else
                throw type_mismatch{"not a lispmds::list, cannot use [index]: "s + typeid(arg).name()};
        }, val);
    }

    inline const value& get_(const value& val, int aIndex)
    {
        return get_(val, static_cast<size_t>(aIndex));
    }

    inline const value& get_(const value& val, std::string aKeyword)
    {
        return std::visit([aKeyword](auto&& arg) -> const value& {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, list>)
                return arg[aKeyword];
            else
                throw type_mismatch{"not a lispmds::list, cannot use [keyword]"};
        }, val);
    }

    template <typename Arg, typename... Args> inline const value& get(const value& val, Arg&& arg, Args&&... args)
    {
        if constexpr (sizeof...(args) == 0) {
            return get_(val, std::forward<Arg>(arg));
        }
        else {
            return get(get_(val, std::forward<Arg>(arg)), args...);
        }
    }

    inline size_t size(const value& val)
    {
        return std::visit([](auto&& arg) -> size_t {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, list>)
                return arg.size();
            else if constexpr (std::is_same_v<T, nil>)
                return 0;
            else
                throw type_mismatch{"not a lispmds::list, cannot use size()"};
        }, val);
    }

    template <typename... Args> inline size_t size(const value& val, Args&&... args)
    {
        return size(get(val, args...));
    }

    inline bool empty(const value& val)
    {
        return std::visit([](auto&& arg) -> bool {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, list>)
                return arg.empty();
            else if constexpr (std::is_same_v<T, nil>)
                return true;
            else
                throw type_mismatch{"not a lispmds::list, cannot use empty()"};
        }, val);
    }

    template <typename... Args> inline bool empty(const value& val, Args&&... args)
    {
        return empty(get(val, args...));
    }

// ----------------------------------------------------------------------

    inline const value& list::operator[](std::string_view aKeyword) const
    {
        auto p = mContent.begin();
        while (p != mContent.end() && (!std::get_if<keyword>(&*p) || std::get<keyword>(*p) != aKeyword))
            ++p;
        if (p == mContent.end())
            throw keyword_no_found{std::string{aKeyword}};
        ++p;
        if (p == mContent.end())
            throw keyword_has_no_value{std::string{aKeyword}};
        return *p;
    }

// ----------------------------------------------------------------------

    value parse_string(std::string_view aData);

} // namespace acmacs::lispmds

// ----------------------------------------------------------------------

namespace acmacs
{
    inline std::string to_string(const lispmds::nil&) { return "nil"; }

    inline std::string to_string(const lispmds::boolean& val) { return val ? "t" : "f"; }

    inline std::string to_string(const lispmds::number& val) { return fmt::format("{}", static_cast<double>(val)); }

    inline std::string to_string(const lispmds::string& val) { return '"' + static_cast<std::string>(val) + '"'; }

    inline std::string to_string(const lispmds::symbol& val) { return '\'' + static_cast<std::string>(val); }

    inline std::string to_string(const lispmds::keyword& val) { return static_cast<std::string>(val); }

    std::string to_string(const lispmds::value& val);

    inline std::string to_string(const lispmds::list& list)
    {
        std::string result{"(\n"};
        for (const lispmds::value& val : list) {
            result.append(to_string(val));
            result.append(1, '\n');
        }
        result.append(")\n");
        return result;
    }

    inline std::string to_string(const lispmds::value& val)
    {
        return std::visit([](auto&& arg) -> std::string { return to_string(arg); }, val);
    }

} // namespace acmacs

// ----------------------------------------------------------------------

// inline std::ostream& operator<<(std::ostream& s, const acmacs::lispmds::value& val)
// {
//     return s << acmacs::to_string(val);
// }

template <typename T> struct fmt::formatter<T, std::enable_if_t<std::is_same_v<decltype(acmacs::to_string(std::declval<T>())), std::string>, std::string>> : fmt::formatter<std::string> {
        template <typename FormatCtx> auto format(const T& val, FormatCtx& ctx) { return fmt::formatter<std::string>::format(acmacs::to_string(val), ctx); }
};


// ----------------------------------------------------------------------
