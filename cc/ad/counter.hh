#pragma once

#include <map>
#include <array>
#include <vector>
#include <numeric>

#include "ext/fmt.hh"

// ----------------------------------------------------------------------

namespace acmacs
{
    template <typename Obj> class Counter
    {
     public:
        using container_type = std::map<Obj, size_t>;
        using value_type = typename container_type::value_type;

        Counter() = default;
        template <typename Iter, typename F> Counter(Iter first, Iter last, F func)
            {
                for (; first != last; ++first)
                    ++counter_[func(*first)];
            }
        template <typename Container, typename F> Counter(const Container& container, F func) : Counter(std::begin(container), std::end(container), func) {}

        template <typename S> void count(const S& aObj) { ++counter_[Obj{aObj}]; }
        template <typename S> void count_if(bool cond, const S& aObj) { if (cond) ++counter_[Obj{aObj}]; }

        const auto& min() const { return *std::min_element(counter_.begin(), counter_.end(), [](const auto& e1, const auto& e2) { return e1.second < e2.second; }); }
        const auto& max() const { return *std::max_element(counter_.begin(), counter_.end(), [](const auto& e1, const auto& e2) { return e1.second < e2.second; }); }
        const auto& min_value() const { return *std::min_element(counter_.begin(), counter_.end(), [](const auto& e1, const auto& e2) { return e1.first < e2.first; }); }
        const auto& max_value() const { return *std::max_element(counter_.begin(), counter_.end(), [](const auto& e1, const auto& e2) { return e1.first < e2.first; }); }

        auto sorted_max_first(size_t limit = 0) const
            {
                std::vector<const value_type*> result(counter_.size());
                std::transform(std::begin(counter_), std::end(counter_), std::begin(result), [](const auto& ee) { return &ee; });
                std::sort(result.begin(), result.end(), [](const auto& e1, const auto& e2) { return e1->second > e2->second; });
                if (limit > 0 && limit < result.size())
                    result.erase(std::next(result.begin(), static_cast<ssize_t>(limit)), result.end());
                return result;
            }

        constexpr const auto& counter() const { return counter_; }
        constexpr size_t size() const { return counter_.size(); }
        constexpr bool empty() const { return counter_.empty(); }

        template <typename S> size_t operator[](const S& look_for) const
        {
            if (const auto found = counter_.find(look_for); found != counter_.end())
                return found->second;
            else
                return 0;
        }

        std::string report(std::string_view format) const
        {
            fmt::memory_buffer out;
            for (const auto& entry : counter_)
                format_entry(out, format, entry);
            return fmt::to_string(out);
        }

        std::string report() const { return report("{value}  {counter:6d}\n"); }

        std::string report_sorted_max_first(std::string_view format, size_t limit = 0) const
        {
            fmt::memory_buffer out;
            for (const auto& entry : sorted_max_first(limit))
                format_entry(out, format, *entry);
            return fmt::to_string(out);
        }

        std::string report_sorted_max_first() const { return report_sorted_max_first("{counter:6d} {value}\n"); }

        size_t total() const
        {
            return std::accumulate(std::begin(counter_), std::end(counter_), 0UL, [](size_t sum, const auto& en) { return sum + en.second; });
        }

     private:
        container_type counter_;

        void format_entry(fmt::memory_buffer& out, std::string_view format, const value_type& entry) const
        {
            fmt::format_to(std::back_inserter(out), fmt::runtime(format), fmt::arg("quoted", fmt::format("\"{}\"", entry.first)), fmt::arg("value", entry.first), fmt::arg("counter", entry.second), //
                              fmt::arg("counter_percent", static_cast<double>(entry.second) / static_cast<double>(total()) * 100.0),                                                //
                              fmt::arg("first", entry.first), fmt::arg("quoted_first", fmt::format("\"{}\"", entry.first)), fmt::arg("second", entry.second));
        }

    }; // class Counter<Obj>

    template <typename Iter, typename F> Counter(Iter first, Iter last, F func) -> Counter<decltype(func(*first))>;
    template <typename Container, typename F> Counter(const Container& cont, F func) -> Counter<decltype(func(*std::begin(cont)))>;

    template <size_t first_char = 0, size_t last_char = 256, typename counter_t_t = uint32_t> class CounterCharSome
    {
      public:
        using counter_t = counter_t_t;

        CounterCharSome() { counter_.fill(counter_t{0}); }

        template <typename Iter, typename F> CounterCharSome(Iter first, Iter last, F func) : CounterCharSome()
        {
            for (; first != last; ++first)
                ++counter_[func(*first) - first_char];
        }
        template <typename Iter> CounterCharSome(Iter first, Iter last) : CounterCharSome()
        {
            for (; first != last; ++first)
                ++counter_[static_cast<size_t>(*first) - first_char];
        }
        template <typename Container, typename F> CounterCharSome(const Container& container, F func) : CounterCharSome(std::begin(container), std::end(container), func) {}
        template <typename Container> CounterCharSome(const Container& container) : CounterCharSome(std::begin(container), std::end(container)) {}

        void count(char aObj) { ++counter_[static_cast<unsigned char>(aObj) - first_char]; }
        void count(char aObj, counter_t num) { counter_[static_cast<unsigned char>(aObj) - first_char] += num; }
        template <typename Iter> void count(Iter first, Iter last)
        {
            for (; first != last; ++first)
                ++counter_[static_cast<size_t>(*first) - first_char];
        }

        void update(const CounterCharSome<first_char, last_char, counter_t_t>& other)
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

    using CounterChar = CounterCharSome<0, 256, uint32_t>;

    template <size_t first_char, size_t last_char, typename counter_t_t>
    CounterCharSome<first_char, last_char, counter_t_t> merge_CounterCharSome(const CounterCharSome<first_char, last_char, counter_t_t>& lhs,
                                                                              const CounterCharSome<first_char, last_char, counter_t_t>& rhs)
    {
        CounterCharSome<first_char, last_char, counter_t_t> result;
        std::transform(std::begin(lhs.counter()), std::end(lhs.counter()), std::begin(rhs.counter()), std::begin(result.counter()), [](auto c1, auto c2) { return c1 + c2; });
        return result;
    }

} // namespace acmacs

// ----------------------------------------------------------------------

template <typename Key> struct fmt::formatter<acmacs::Counter<Key>> : public fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatContext> auto format(const acmacs::Counter<Key>& counter, FormatContext& ctx)
    {
        return format_to(ctx.out(), "counter{{{}}}", counter.counter());
    }
};

template <> struct fmt::formatter<acmacs::CounterChar> : public fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatContext> auto format(const acmacs::CounterChar& counter, FormatContext& ctx)
    {
        auto out = format_to(ctx.out(), "counter{{");
        const auto keys = counter.sorted();
        for (auto it = keys.begin(); it != keys.end(); ++it) {
            if (it != keys.begin())
                out = format_to(out, ", ");
            out = format_to(out, "{}: {}", *it, counter[*it]);
        }
        return format_to(out, "}}");
    }
};


// ----------------------------------------------------------------------
