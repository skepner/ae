#pragma once

#include <string_view>
#include <string>
#include <vector>
#include <iterator>
#include <cctype>
#include <algorithm>

#include "ext/fmt.hh"
#include "ext/from_chars.hh"

// ----------------------------------------------------------------------

namespace ae::string
{
    inline std::string_view strip(std::string_view source)
    {
        const std::string_view spaces{" \t\n\r"};
        if (const auto first = source.find_first_not_of(spaces); first == std::string_view::npos)
            return {};
        else
            source.remove_prefix(first);
        source.remove_suffix(source.size() - source.find_last_not_of(spaces));
        return source;
    }

    inline std::string& uppercase_in_place(std::string& source)
    {
        std::transform(std::begin(source), std::end(source), std::begin(source), [](auto cc) { return std::toupper(cc); });
        return source;
    }

    template <typename Iter, typename F> inline std::string transform(Iter first, Iter last, F&& func)
    {
        std::string result(static_cast<size_t>(last - first), '?');
        std::transform(first, last, result.begin(), func);
        return result;
    }

    inline std::string uppercase(std::string_view source) { return transform(source.begin(), source.end(), ::toupper); }
    template <typename Iter> inline std::string uppercase(Iter first, Iter last) { return transform(first, last, ::toupper); }

    inline std::string lowercase(std::string_view source) { return transform(source.begin(), source.end(), ::tolower); }
    template <typename Iter> inline std::string lowercase(Iter first, Iter last) { return transform(first, last, ::tolower); }

    inline std::string remove_spaces(std::string_view source)
    {
        std::string result;
        std::copy_if(source.begin(), source.end(), std::back_inserter(result), [](auto c) -> bool { return !std::isspace(c); });
        return result;
    }

} // namespace ae::string

// ======================================================================

namespace ae::string
{
    inline bool endswith(std::string_view source, std::string_view suffix) { return source.size() >= suffix.size() && source.substr(source.size() - suffix.size()) == suffix; }

    inline bool startswith(std::string_view source, std::string_view prefix) { return source.size() >= prefix.size() && source.substr(0, prefix.size()) == prefix; }

    // assumes s1.size() == s2.size()
    inline bool equals_ignore_case_same_length(std::string_view s1, std::string_view s2)
    {
        for (auto c1 = s1.begin(), c2 = s2.begin(); c1 != s1.end(); ++c1, ++c2) {
            if (std::toupper(*c1) != std::toupper(*c2))
                return false;
        }
        return true;
    }

    inline bool equals_ignore_case(std::string_view s1, std::string_view s2) { return s1.size() == s2.size() && equals_ignore_case_same_length(s1, s2); }

    inline bool endswith_ignore_case(std::string_view source, std::string_view suffix)
    {
        return source.size() >= suffix.size() && equals_ignore_case_same_length(source.substr(source.size() - suffix.size()), suffix);
    }

    inline bool startswith_ignore_case(std::string_view source, std::string_view prefix)
    {
        return source.size() >= prefix.size() && equals_ignore_case_same_length(source.substr(0, prefix.size()), prefix);
    }

} // namespace ae::string

// ======================================================================

namespace ae::string
{
    enum class split_emtpy { remove, keep, strip_remove, strip_keep };

    // ----------------------------------------------------------------------

    class split_iterator
    {
      public:
        // using iterator_category = std::input_iterator_tag;
        // using value_type = std::string_view;
        // using difference_type = typename value_type::difference_type;
        using size_type = typename std::string_view::size_type;
        // using pointer = std::string_view*;
        // using reference = std::string_view&;

        split_iterator() = default;
        split_iterator(std::string_view s, std::string_view delim, split_emtpy keep_empty) : input_end_(&*s.cend()), delim_(delim), handle_empty_{keep_empty}, begin_{s.data()}
        {
            if (delim_.empty()) {
                end_ = input_end_;
            }
            else
                next();
        }

        std::string_view operator*() noexcept
        {
            std::string_view res{begin_, static_cast<size_type>(end_ - begin_)};
            if (handle_empty_ == split_emtpy::strip_keep || handle_empty_ == split_emtpy::strip_remove)
                res = strip(res);
            return res;
        }

        split_iterator& operator++() noexcept
        {
            if (end_ != nullptr) {
                if (delim_.empty())
                    set_end();
                else
                    next();
            }
            return *this;
        }

        split_iterator operator++(int) noexcept = delete;

        bool operator==(const split_iterator& other) const noexcept
        {
            return end_ == other.end_ && (end_ == nullptr || (input_end_ == other.input_end_ && delim_ == other.delim_ && handle_empty_ == other.handle_empty_ && begin_ == other.begin_));
        }

        bool operator!=(const split_iterator& other) const noexcept { return !operator==(other); }

      private:
        const char* input_end_{nullptr};
        const std::string_view delim_{};
        const split_emtpy handle_empty_{split_emtpy::remove};
        const char* begin_{nullptr};
        const char* end_{nullptr};

        void set_end() { begin_ = end_ = nullptr; }

        // http://stackoverflow.com/questions/236129/split-a-string-in-c
        void next()
        {
            for (const char *substart = end_ == nullptr ? begin_ : end_ + delim_.size(), *subend; substart <= input_end_; substart = subend + delim_.size()) {
                subend = std::search(substart, input_end_, delim_.cbegin(), delim_.cend());
                if (substart != subend || handle_empty_ == split_emtpy::keep || handle_empty_ == split_emtpy::strip_keep) {
                    begin_ = substart;
                    end_ = subend;
                    return;
                }
            }
            set_end();
        }

    }; // class split_iterator

    // ----------------------------------------------------------------------

    class split_error : public std::runtime_error { public: using std::runtime_error::runtime_error; };

    namespace detail
    {
        // template <typename S> class split_iterator
        // {
        //  public:
        //     using iterator_category = std::input_iterator_tag;
        //     using value_type = std::string_view;
        //     using difference_type = ssize_t;
        //     using pointer = std::string_view*;
        //     using reference = std::string_view&;

        //     inline split_iterator() : mInputEnd(nullptr), mKeepEmpty(split_emtpy::remove), mBegin(nullptr), mEnd(nullptr) {}
        //     inline split_iterator(const S& s, std::string_view delim, split_emtpy keep_empty)
        //         : mInputEnd(&*s.cend()), mDelim(delim), mKeepEmpty(keep_empty), mBegin(s.data()), mEnd(nullptr)
        //         {
        //             if (mDelim.empty()) {
        //                 mEnd = mInputEnd;
        //             }
        //             else
        //                 next();
        //         }

        //     inline value_type operator*() noexcept
        //         {
        //             value_type res{mBegin, static_cast<typename value_type::size_type>(mEnd - mBegin)};
        //             if (mKeepEmpty == split_emtpy::strip_keep || mKeepEmpty == split_emtpy::strip_remove)
        //                 res = strip(res);
        //             return res;
        //         }

        //     inline split_iterator& operator++() noexcept
        //         {
        //             if (mEnd != nullptr) {
        //                 if (mDelim.empty())
        //                     set_end();
        //                 else
        //                     next();
        //             }
        //             return *this;
        //         }

        //     split_iterator operator++(int) noexcept = delete;

        //     inline bool operator==(const split_iterator& other) const noexcept
        //         {
        //             return mEnd == other.mEnd && (mEnd == nullptr || (mInputEnd == other.mInputEnd && mDelim == other.mDelim && mKeepEmpty == other.mKeepEmpty && mBegin == other.mBegin));
        //         }

        //     inline bool operator!=(const split_iterator& other) const noexcept { return !operator==(other); }

        //  private:
        //     const char* mInputEnd;
        //     const std::string_view mDelim;
        //     const split_emtpy mKeepEmpty;
        //     const char* mBegin;
        //     const char* mEnd;

        //     inline void set_end() { mBegin = mEnd = nullptr; }

        //       // http://stackoverflow.com/questions/236129/split-a-string-in-c
        //     inline void next()
        //         {
        //             for (const char* substart = mEnd == nullptr ? mBegin : mEnd + mDelim.size(), *subend; substart <= mInputEnd; substart = subend + mDelim.size()) {
        //                 subend = std::search(substart, mInputEnd, mDelim.cbegin(), mDelim.cend());
        //                 if (substart != subend || mKeepEmpty == split_emtpy::keep || mKeepEmpty == split_emtpy::strip_keep) {
        //                     mBegin = substart;
        //                     mEnd = subend;
        //                     return;
        //                 }
        //             }
        //             set_end();
        //         }

        // }; // class split_iterator

          // ======================================================================

        template <typename T, typename Extractor> inline std::vector<T> split_into(std::string_view source, std::string_view delim, Extractor extractor, const char* extractor_name, split_emtpy keep_empty = split_emtpy::keep)
        {
            auto extract = [&](auto chunk) -> T {
                               try {
                                   size_t pos = 0; // g++-9 warn about uninitialized otherwise
                                   const T result = extractor(chunk, &pos);
                                   if (pos != chunk.size())
                                       throw split_error{fmt::format("cannot read {} from \"{}\"", extractor_name, chunk)};
                                   return result;
                               }
                               catch (split_error&) {
                                   throw;
                               }
                               catch (std::exception& err) {
                                   throw split_error{fmt::format("cannot read {} from \"{}\": {}", extractor_name, chunk, err.what())};
                               }
                           };

            std::vector<T> result;
            std::transform(split_iterator(source, delim, keep_empty), split_iterator(), std::back_inserter(result), extract);
            return result;
        }

        template <typename T, typename Extractor> inline std::vector<T> split_into(std::string_view source, Extractor extractor, const char* extractor_name, split_emtpy keep_empty = split_emtpy::keep)
        {
            using namespace std::string_view_literals;
            for (auto delim : {","sv, " "sv, ", "sv, ":"sv, ";"sv}) {
                try {
                    return detail::split_into<T>(source, delim, extractor, extractor_name, keep_empty);
                }
                catch (split_error&) {
                }
            }
            throw split_error{fmt::format("cannot extract {}'s from \"{}\"", extractor_name, source)};
        }

    } // namespace detail

    template <typename T> inline std::vector<T> split_into_uint(std::string_view source, std::string_view delim)
    {
        return detail::split_into<T>(source, delim, [](const auto& chunk, size_t* pos) -> T { return T{ae::from_chars<size_t>(chunk, *pos)}; }, "unsigned", split_emtpy::remove);
    }

    template <typename T> inline std::vector<T> split_into_uint(std::string_view source)
    {
        return detail::split_into<T>(source, [](const auto& chunk, size_t* pos) -> T { return T{ae::from_chars<size_t>(chunk, *pos)}; }, "unsigned", split_emtpy::remove);
    }

    inline std::vector<size_t> split_into_size_t(std::string_view source, std::string_view delim)
    {
        return detail::split_into<size_t>(source, delim, [](const auto& chunk, size_t* pos) -> size_t { return ae::from_chars<size_t>(chunk, *pos); }, "unsigned", split_emtpy::remove);
    }

    inline std::vector<size_t> split_into_size_t(std::string_view source)
    {
        return detail::split_into<size_t>(source, [](const auto& chunk, size_t* pos) -> size_t { return ae::from_chars<size_t>(chunk, *pos); }, "unsigned", split_emtpy::remove);
    }

    inline std::vector<double> split_into_double(std::string_view source, std::string_view delim)
    {
        return detail::split_into<double>(source, delim, [](const auto& chunk, size_t* pos) -> double { return ae::from_chars<double>(chunk, *pos); }, "double", split_emtpy::remove);
    }

    inline std::vector<double> split_into_double(std::string_view source)
    {
        return detail::split_into<double>(source, [](const auto& chunk, size_t* pos) -> double { return ae::from_chars<double>(chunk, *pos); }, "double", split_emtpy::remove);
    }

}

template <> struct std::iterator_traits<ae::string::split_iterator>
{
    using iterator_category = std::input_iterator_tag;
    using value_type = std::string_view;
    using reference = std::string_view&;
};

// ======================================================================

namespace ae::string
{
    inline std::vector<std::string_view> split(std::string_view source, std::string_view delim, split_emtpy handle_empty = split_emtpy::keep)
    {
        return {split_iterator(source, delim, handle_empty), split_iterator{}};
    }

    inline std::vector<std::string_view> split(std::string_view source, split_emtpy handle_empty = split_emtpy::keep)
    {
        using namespace std::string_view_literals;

        if (source.find(',') != std::string_view::npos)
            return split(source, ","sv, handle_empty);
        else if (source.find(' ') != std::string_view::npos)
            return split(source, " "sv, handle_empty);
        else
            return split(source, "\n"sv, handle_empty);
    }

    // ----------------------------------------------------------------------

    // changes subsequent spaces into one space
    inline std::string collapse_spaces(std::string_view source)
    {
        std::string result;
        std::copy_if(source.begin(), source.end(), std::back_inserter(result), [prev_was_space = false](auto c) mutable -> bool {
            const auto space = std::isspace(c);
            const auto res = !(prev_was_space && space);
            prev_was_space = space;
            return res;
        });
        return result;
    }
    inline std::string collapse_spaces(const char* source) { return collapse_spaces(std::string_view(source, std::strlen(source))); }
    inline std::string collapse_spaces(char* source) { return collapse_spaces(std::string_view(source, std::strlen(source))); }

    // ----------------------------------------------------------------------

    // join collection using separator, empty elements in collection are omitted
    template <typename Collection> inline std::string join(std::string_view separator, Collection&& collection)
    {
        fmt::memory_buffer out;
        bool sep{false};
        for (const auto& en : collection) {
            if (!en.empty()) {
                if (sep)
                    fmt::format_to(std::back_inserter(out), "{}", separator);
                else
                    sep = true;
                fmt::format_to(std::back_inserter(out), "{}", en);
            }
        }
        return fmt::to_string(out);
    }

    namespace detail
    {
        inline std::string_view stringify(std::string_view source) { return source; }
        inline std::string_view stringify(const char* source) { return source; }
        inline const std::string& stringify(const std::string& source) { return source; }
        template <typename Arg> requires(!std::is_same_v<Arg, std::string>) inline std::string stringify(const Arg& arg) { return fmt::format("{}", arg); }

    } // namespace detail

    template <typename Arg1, typename Arg2, typename... Args> inline std::string join(std::string_view separator, Arg1&& arg1, Arg2&& arg2, Args&&... rest)
    {
        if constexpr (sizeof...(rest) == 0) {
            const auto as1{detail::stringify(arg1)};
            const auto as2{detail::stringify(arg2)};
            if (!as1.empty()) {
                if (!as2.empty()) {
                    std::string res(as1.size() + as2.size() + separator.size(), '#');
                    std::copy(as1.begin(), as1.end(), res.begin());
                    std::copy(separator.begin(), separator.end(), std::next(res.begin(), static_cast<ssize_t>(as1.size())));
                    std::copy(as2.begin(), as2.end(), std::next(res.begin(), static_cast<ssize_t>(as1.size() + separator.size())));
                    return res;
                }
                else
                    return std::string{as1};
            }
            else
                return std::string{as2};
        }
        else {
            return join(separator, join(separator, std::forward<Arg1>(arg1), std::forward<Arg2>(arg2)), std::forward<Args>(rest)...);
        }
    }

    // ----------------------------------------------------------------------

    inline std::string first_letter_of_words(std::string_view source)
    {
        std::string result;
        bool add = true;
        for (char c : source) {
            if (c == ' ') {
                add = true;
            }
            else if (add) {
                result.push_back(c);
                add = false;
            }
        }
        return result;
    }

    // ----------------------------------------------------------------------

    inline std::string replace(std::string_view source, std::string_view look_for, std::string_view replace_with)
    {
        std::string result;
        std::string::size_type start = 0;
        while (true) {
            const auto pos = source.find(look_for, start);
            if (pos != std::string::npos) {
                result.append(source.begin() + static_cast<std::string::difference_type>(start), source.begin() + static_cast<std::string::difference_type>(pos));
                result.append(replace_with);
                start = pos + look_for.size();
            }
            else {
                result.append(source.begin() + static_cast<std::string::difference_type>(start), source.end());
                break;
            }
        }
        return result;
    }

    inline std::string replace(std::string_view source, char look_for, char replace_with)
    {
        std::string result(source.size(), ' ');
        std::transform(std::begin(source), std::end(source), std::begin(result), [=](char c) { if (c == look_for) return replace_with; else return c; });
        return result;
    }

    inline std::string& replace_in_place(std::string& source, char look_for, char replace_with)
    {
        std::transform(std::begin(source), std::end(source), std::begin(source), [=](char c) { if (c == look_for) return replace_with; else return c; });
        return source;
    }

    template <typename ... Args> inline std::string replace(std::string_view source, std::string_view l1, std::string_view r1, std::string_view l2, std::string_view r2, Args ... args)
    {
        return replace(replace(source, l1, r1), l2, r2, args ...);
    }

    inline std::string replace_spaces(std::string_view source, char replacement)
    {
        std::string result(source.size(), replacement);
        std::transform(source.begin(), source.end(), result.begin(), [replacement](char c) {
            if (std::isspace(c))
                c = replacement;
            return c;
        });
        return result;
    }

} // namespace ae::string

// ----------------------------------------------------------------------
