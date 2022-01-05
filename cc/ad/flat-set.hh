#pragma once

#include <vector>
#include <algorithm>
#include "ext/fmt.hh"

// ----------------------------------------------------------------------

namespace acmacs
{
    enum class flat_set_sort_afterwards { no, yes };

    template <typename T> class flat_set_t
    {
      public:
        flat_set_t() = default;
        flat_set_t(std::initializer_list<T> source) : data_(source) {}

        auto begin() const { return data_.begin(); }
        auto end() const { return data_.end(); }
        auto empty() const { return data_.empty(); }
        auto size() const { return data_.size(); }
        auto clear() { data_.clear(); }

        const auto& front() const { return data_.front(); }
        void sort() { std::sort(std::begin(data_), std::end(data_)); }

        template <typename Arg> auto find(Arg&& key) const { return std::find(std::begin(data_), std::end(data_), std::forward<Arg>(key)); }
        template <typename Arg> auto find(Arg&& key) { return std::find(std::begin(data_), std::end(data_), std::forward<Arg>(key)); }

        template <typename Arg> bool exists(Arg&& key) const { return find(std::forward<Arg>(key)) != std::end(data_); }

        template <typename Predicate> void erase_if(Predicate&& pred)
        {
            data_.erase(std::remove_if(std::begin(data_), std::end(data_), std::forward<Predicate>(pred)), std::end(data_));
        }

        template <typename Arg> void add(Arg&& elt, flat_set_sort_afterwards a_sort = flat_set_sort_afterwards::no)
        {
            if (!exists(elt)) {
                if constexpr (std::is_same_v<std::decay_t<Arg>, T>)
                    data_.push_back(std::forward<Arg>(elt));
                else if constexpr (std::is_constructible_v<T, Arg>)
                    data_.push_back(T{std::forward<Arg>(elt)});
                else
                    static_assert(std::is_same_v<T, void>, "flat_set_t::add cannot be called with this type of argument");
                if (a_sort == flat_set_sort_afterwards::yes)
                    sort();
            }
        }

        void merge_from(const flat_set_t& source, flat_set_sort_afterwards a_sort = flat_set_sort_afterwards::no)
        {
            for (const auto& src : source)
                add(src, flat_set_sort_afterwards::no);
            if (a_sort == flat_set_sort_afterwards::yes)
                sort();
        }

      private:
        std::vector<T> data_{};
    };

} // namespace acmacs

// ----------------------------------------------------------------------

template <typename Elt> struct fmt::formatter<acmacs::flat_set_t<Elt>> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const acmacs::flat_set_t<Elt>& value, FormatCtx& ctx)
    {
        fmt::format_to(ctx.out(), "{{");
        bool first{true};
        for (const auto& val : value) {
            if (!first)
                fmt::format_to(ctx.out(), ", ");
            else
                first = false;
            fmt::format_to(ctx.out(), "{}", val);
        }
        return fmt::format_to(ctx.out(), "}}");
    }
};

// ----------------------------------------------------------------------
