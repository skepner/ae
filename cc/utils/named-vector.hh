#pragma once

#include <vector>

#include "utils/named-type.hh"

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

        explicit named_vector_t(size_t size, T val) : named_t<std::vector<T>, Tag>{std::vector<T>(size, val)} {}

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

        // for std::back_inserter
        constexpr void push_back(const T& val) { this->get().push_back(val); }
        constexpr void push_back(T&& val) { this->get().push_back(std::forward<T>(val)); }

        void remove(const T& val)
        {
            if (const auto found = std::find(begin(), end(), val); found != end())
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
    };

    template <typename Tag> constexpr bool operator<(const named_vector_t<std::string, Tag>& lhs, const named_vector_t<std::string, Tag>& rhs) noexcept
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
