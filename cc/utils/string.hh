#pragma once

#include <string_view>
#include <string>
#include <vector>
#include <iterator>
#include <cctype>
#include <algorithm>

#include "ext/fmt.hh"

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

} // namespace ae::string

// ======================================================================

namespace ae::string
{
    inline bool endswith(std::string_view source, std::string_view suffix) { return source.size() >= suffix.size() && source.substr(source.size() - suffix.size()) == suffix; }

    inline bool startswith(std::string_view source, std::string_view prefix) { return source.size() >= prefix.size() && source.substr(0, prefix.size()) == prefix; }

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

} // namespace ae::string

// ----------------------------------------------------------------------
