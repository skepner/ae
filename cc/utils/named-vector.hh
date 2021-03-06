#pragma once

#include <vector>
#include <algorithm>
#include <numeric>

#include "ext/compare.hh"
#include "utils/named-type.hh"
#include "utils/log.hh"

// ----------------------------------------------------------------------

namespace ae
{
    template <typename T, typename Tag> class named_vector_t : public named_t<std::vector<T>, Tag>
    {
      public:
        using value_type = T;
        using named_t<std::vector<T>, Tag>::named_t;
        using iterator = typename std::vector<T>::iterator;
        using const_iterator = typename std::vector<T>::const_iterator;

        named_vector_t() = default;
        explicit named_vector_t(size_t size, T val) : named_t<std::vector<T>, Tag>{std::vector<T>(size, val)} {}
        template <typename Iter> named_vector_t(Iter first, Iter last) : named_t<std::vector<T>, Tag>{std::vector<T>(first, last)} {}
        auto operator<=>(const named_vector_t&) const = default;

        constexpr const_iterator begin() const { return this->get().begin(); }
        constexpr const_iterator end() const { return this->get().end(); }
        constexpr auto begin() { return this->get().begin(); }
        constexpr auto end() { return this->get().end(); }
        constexpr auto rbegin() { return this->get().rbegin(); }
        constexpr auto rend() { return this->get().rend(); }
        constexpr auto empty() const { return this->get().empty(); }
        constexpr auto size() const { return this->get().size(); }
        constexpr auto operator[](size_t index) const { return this->get()[index]; }
        constexpr auto& operator[](size_t index) { return this->get()[index]; }
        constexpr auto back() const { return this->get().back(); }
        // constexpr void resize(size_t size) { this->get().resize(size); }

        // for std::back_inserter
        constexpr void push_back(const T& val) { this->get().push_back(val); }
        constexpr void push_back(T&& val) { this->get().push_back(std::forward<T>(val)); }

        // size of strings joined by separator
        size_t join_size(size_t separator_size) const
        {
            if (empty())
                return 0;
            if constexpr (std::is_same_v<T, std::string>) {
                return separator_size * (size() - 1) + std::accumulate(begin(), end(), 0ul, [](size_t acc, const std::string& add) { return acc + add.size(); });
            }
            else
                static_assert(std::is_same_v<T, void>);
        }

        template <constructible_from<T> T2> void remove(T2&& val)
        {
            if (const auto found = std::find(begin(), end(), std::forward<T2>(val)); found != end())
                this->get().erase(found);
        }

        // set like
        template <constructible_from<T> T2> bool contains(T2&& val) const { return std::find(begin(), end(), T{std::forward<T2>(val)}) != end(); }

        bool contains_any_of(const std::vector<T>& vals) const
        {
            return std::any_of(std::begin(vals), std::end(vals), [this](const T& val) { return contains(val); });
        }

        template <constructible_from<T> T2> void insert(T2&& val)
        {
            this->get().emplace_back(std::forward<T2>(val));
        }

        template <constructible_from<T> T2> void insert_if_not_present(T2&& val)
        {
            if (!contains(std::forward<T2>(val)))
                insert(std::forward<T2>(val));
        }

        template <constructible_from<T> T2, typename Tag2> void insert_if_not_present(const named_vector_t<T2, Tag2>& another)
        {
            for (const auto& val : another)
                insert_if_not_present(val);
        }

        void unique()
        {
            std::sort(begin(), end());
            this->get().erase(std::unique(begin(), end()), end());
        }
    };

    template <typename Tag> inline std::vector<std::string> to_vector_base_t(const ae::named_vector_t<std::string, Tag>& source)
    {
        std::vector<std::string> result(source.size());
        std::transform(source.begin(), source.end(), result.begin(), [](auto src) { return src; });
        return result;
    }

    template <typename Tag> bool operator<(const named_vector_t<std::string, Tag>& lhs, const named_vector_t<std::string, Tag>& rhs) noexcept
    {
        const auto sz = std::min(lhs.size(), rhs.size());
        for (size_t i = 0; i < sz; ++i) {
            if (!(lhs[i] == rhs[i]))
                return lhs[i] < rhs[i];
        }
        if (lhs.size() < rhs.size())
            return true;
        return false;
    }

} // namespace ae

// ----------------------------------------------------------------------
