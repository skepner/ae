#pragma once

#include <array>
#include <vector>
#include <numeric>

#include "ext/fmt.hh"

// ======================================================================

namespace ae
{
    template <unsigned first_char = 0, unsigned last_char = 256, typename counter_t_t = unsigned> class counter_char_t
    {
      public:
        using counter_t = counter_t_t;

        counter_char_t() { counter_.fill(counter_t{0}); }

        template <typename Iter, typename F> counter_char_t(Iter first, Iter last, F func) : counter_char_t()
        {
            for (; first != last; ++first)
                ++counter_[func(*first) - first_char];
        }
        template <typename Iter> counter_char_t(Iter first, Iter last) : counter_char_t()
        {
            for (; first != last; ++first)
                ++counter_[static_cast<size_t>(*first) - first_char];
        }
        template <typename Container, typename F> counter_char_t(const Container& container, F func) : counter_char_t(std::begin(container), std::end(container), func) {}
        template <typename Container> counter_char_t(const Container& container) : counter_char_t(std::begin(container), std::end(container)) {}

        void reset() { counter_.fill(counter_t{0}); }

        void count(char aObj) { ++counter_[static_cast<unsigned char>(aObj) - first_char]; }
        void count(char aObj, counter_t num) { counter_[static_cast<unsigned char>(aObj) - first_char] += num; }
        template <typename Iter> void count(Iter first, Iter last)
        {
            for (; first != last; ++first)
                ++counter_[static_cast<size_t>(*first) - first_char];
        }

        void update(const counter_char_t<first_char, last_char, counter_t_t>& other)
        {
            std::transform(counter_.begin(), counter_.end(), other.counter_.begin(), counter_.begin(), [](counter_t e1, counter_t e2) { return e1 + e2; });
        }

        bool empty() const
        {
            return std::all_of(std::begin(counter_), std::end(counter_), [](counter_t val) { return val == 0; });
        }
        size_t size() const
        {
            return static_cast<size_t>(std::count_if(std::begin(counter_), std::end(counter_), [](counter_t val) { return val > counter_t{0}; }));
        }
        counter_t total() const
        {
            return std::accumulate(std::begin(counter_), std::end(counter_), counter_t{0}, [](counter_t sum, counter_t val) { return sum + val; });
        }

        std::pair<char, counter_t> max() const
        {
            const auto me = std::max_element(counter_.begin(), counter_.end(), [](counter_t e1, counter_t e2) { return e1 < e2; });
            return {static_cast<char>(me - counter_.begin() + static_cast<ssize_t>(first_char)), *me};
        }

        std::vector<char> sorted() const
        {
            std::vector<char> res;
            for (auto it = std::begin(counter_); it != std::end(counter_); ++it) {
                if (*it > 0)
                    res.push_back(static_cast<char>(it - std::begin(counter_) + static_cast<ssize_t>(first_char)));
            }
            std::sort(std::begin(res), std::end(res), [this](auto e1, auto e2) { return counter_[static_cast<size_t>(e1) - first_char] > counter_[static_cast<size_t>(e2) - first_char]; });
            return res;
        }

        enum class sorted { no, yes };
        std::vector<std::pair<char, counter_t>> pairs(enum sorted srt) const
        {
            std::vector<std::pair<char, counter_t>> result;
            for (auto it = std::begin(counter_); it != std::end(counter_); ++it) {
                if (*it > 0)
                    result.emplace_back(static_cast<char>(it - std::begin(counter_) + static_cast<ssize_t>(first_char)), *it);
            }
            if (srt == sorted::yes)
                std::sort(std::begin(result), std::end(result), [](const auto& e1, const auto& e2) { return e1.second > e2.second; });
            return result;
        }

        std::string report(std::string_view format) const
        {
            fmt::memory_buffer out;
            for (const auto& entry : pairs(sorted::no))
                format_entry(out, format, entry);
            return fmt::to_string(out);
        }

        std::string report() const { return report_sorted_max_first("{value} {counter:6d}\n"); }

        std::string report_sorted_max_first(std::string_view format) const
        {
            fmt::memory_buffer out;
            for (const auto& entry : pairs(sorted::yes))
                format_entry(out, format, entry);
            return fmt::to_string(out);
        }

        std::string report_sorted_max_first() const { return report_sorted_max_first("{counter:6d} {value}\n"); }

        constexpr counter_t operator[](char val) const { return counter_[static_cast<size_t>(val) - first_char]; }

        constexpr const auto& counter() const { return counter_; }
        constexpr auto& counter() { return counter_; }

      private:
        std::array<counter_t, last_char - first_char> counter_;

        void format_entry(fmt::memory_buffer& out, std::string_view format, const std::pair<char, counter_t>& entry) const
        {
            fmt::format_to(std::back_inserter(out), fmt::runtime(format), fmt::arg("value", entry.first), fmt::arg("counter", entry.second),
                              fmt::arg("counter_percent", static_cast<double>(entry.second) / static_cast<double>(total()) * 100.0), fmt::arg("first", entry.first),
                              fmt::arg("quoted_first", fmt::format("\"{}\"", entry.first)), fmt::arg("second", entry.second));
        }
    };
}

// ======================================================================
