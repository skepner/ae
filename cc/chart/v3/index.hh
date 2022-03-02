#pragma once

#include <concepts>

#include "ext/fmt.hh"
#include "utils/named-vector.hh"

// ----------------------------------------------------------------------

namespace ae
{
    template <typename Tag> class index_iterator_tt;

    template <typename Tag> class index_tt
    {
      public:
        using value_type = size_t;

        explicit constexpr index_tt() = default;
        constexpr index_tt(const index_tt&) = default;
        constexpr index_tt(index_tt&&) = default;
        template <typename T2> requires std::constructible_from<value_type, T2> explicit constexpr index_tt(T2&& value) : value_(std::forward<T2>(value)) {}

        index_tt& operator=(const index_tt&) = default;
        index_tt& operator=(index_tt&&) = default;
        template <typename T2> requires std::assignable_from<value_type&, T2> constexpr index_tt& operator=(const T2& value) { value_ = value; return *this; }
        template <typename T2> requires std::assignable_from<value_type&, T2> constexpr index_tt& operator=(T2&& value) { value_ = std::move(value); return *this; }

        auto operator<=>(const index_tt&) const = default;

        constexpr value_type& get() noexcept { return value_; }
        constexpr const value_type& get() const noexcept { return value_; }
        constexpr value_type& operator*() noexcept { return value_; }
        constexpr const value_type& operator*() const noexcept { return value_; }
        constexpr const value_type* operator->() const noexcept { return &value_; }
        explicit constexpr operator value_type&() noexcept { return value_; }
        explicit constexpr operator const value_type&() const noexcept { return value_; }

        constexpr index_tt<Tag>& operator++() { ++this->get(); return *this; }
        constexpr index_tt<Tag> operator++(int) { const auto saved{*this}; ++this->get(); return saved; }
        constexpr index_tt<Tag>& operator--() { --this->get(); return *this; }
        constexpr index_tt<Tag> operator--(int) { const auto saved{*this}; --this->get(); return saved; }

        // to iterate from 0 to this index
        index_iterator_tt<Tag> begin() const;
        index_iterator_tt<Tag> end() const;

      protected:
        value_type value_{};
    };

    // ----------------------------------------------------------------------

    template <typename Tag> class index_iterator_tt
    {
      public:
        explicit index_iterator_tt(index_tt<Tag> ind) : value_{ind} {}
        constexpr index_iterator_tt(const index_iterator_tt&) = default;
        constexpr index_iterator_tt(index_iterator_tt&&) = default;
        index_iterator_tt& operator=(const index_iterator_tt&) = default;
        index_iterator_tt& operator=(index_iterator_tt&&) = default;

        constexpr index_tt<Tag>& operator*() noexcept { return value_; }
        constexpr const index_tt<Tag>& operator*() const noexcept { return value_; }

        constexpr index_iterator_tt<Tag>& operator++() { ++value_; return *this; }
        constexpr index_iterator_tt<Tag> operator++(int) { const auto saved{*this}; ++value_; return saved; }
        constexpr index_iterator_tt<Tag>& operator--() { --value_; return *this; }
        constexpr index_iterator_tt<Tag> operator--(int) { const auto saved{*this}; --value_; return saved; }

        auto operator<=>(const index_iterator_tt&) const = default;

      private:
        index_tt<Tag> value_;
    };

    template <typename Tag> inline index_iterator_tt<Tag> index_tt<Tag>::begin() const { return index_iterator_tt<Tag>{index_tt<Tag>{0}}; }
    template <typename Tag> inline index_iterator_tt<Tag> index_tt<Tag>::end() const { return index_iterator_tt<Tag>{*this}; }

    // ----------------------------------------------------------------------

    class antigen_index : public index_tt<struct antigen_index_tag>
    {
      public:
        using index_tt<struct antigen_index_tag>::index_tt;
    };

    class serum_index : public index_tt<struct serum_index_tag>
    {
      public:
        using index_tt<struct serum_index_tag>::index_tt;
    };

    class point_index : public index_tt<struct point_index_tag>
    {
      public:
        using index_tt<struct point_index_tag>::index_tt;
    };

    class projection_index : public index_tt<struct projection_index_tag>
    {
      public:
        using index_tt<struct projection_index_tag>::index_tt;
    };

    // ----------------------------------------------------------------------

    class antigen_indexes : public ae::named_vector_t<antigen_index, struct antigen_indexes_tag>
    {
      public:
        using ae::named_vector_t<antigen_index, struct antigen_indexes_tag>::named_vector_t;
    };

    class serum_indexes : public ae::named_vector_t<serum_index, struct serum_indexes_tag>
    {
      public:
        using ae::named_vector_t<serum_index, struct serum_indexes_tag>::named_vector_t;
    };

} // namespace ae

// ----------------------------------------------------------------------

template <typename Tag> struct fmt::formatter<ae::index_tt<Tag>> : fmt::formatter<typename ae::index_tt<Tag>::value_type>
{
    template <typename FormatCtx> auto format(const ae::index_tt<Tag>& nt, FormatCtx& ctx) const { return fmt::formatter<typename ae::index_tt<Tag>::value_type>::format(*nt, ctx); }
};

// ----------------------------------------------------------------------
