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

        explicit index_tt() : value_{0} {}
        index_tt(const index_tt&) = default;
        index_tt(index_tt&&) = default;
        template <typename T2> requires std::constructible_from<value_type, T2> explicit index_tt(T2&& value) : value_(std::forward<T2>(value)) {}

        index_tt& operator=(const index_tt&) = default;
        index_tt& operator=(index_tt&&) = default;
        template <typename T2> requires std::assignable_from<value_type&, T2> index_tt& operator=(const T2& value) { value_ = value; return *this; }
        template <typename T2> requires std::assignable_from<value_type&, T2> index_tt& operator=(T2&& value) { value_ = std::move(value); return *this; }

        auto operator<=>(const index_tt&) const = default;

        value_type& get() noexcept { return value_; }
        const value_type& get() const noexcept { return value_; }
        value_type& operator*() noexcept { return value_; }
        const value_type& operator*() const noexcept { return value_; }
        const value_type* operator->() const noexcept { return &value_; }
        explicit operator value_type&() noexcept { return value_; }
        explicit operator const value_type&() const noexcept { return value_; }

        index_tt<Tag>& operator++() { ++this->get(); return *this; }
        index_tt<Tag> operator++(int) { const auto saved{*this}; ++this->get(); return saved; }
        index_tt<Tag>& operator--() { --this->get(); return *this; }
        index_tt<Tag> operator--(int) { const auto saved{*this}; --this->get(); return saved; }

        index_tt<Tag> operator+(index_tt<Tag> rhs) { return index_tt<Tag>{get() + rhs.get()}; }

        // to iterate from 0 to this index
        index_iterator_tt<Tag> begin() const;
        index_iterator_tt<Tag> end() const;

      protected:
        value_type value_{};
    };

    struct index_hash_for_unordered_map
    {
        using is_transparent = void;
        template <typename Tag> size_t operator()(const index_tt<Tag>& ind) const { return std::hash<typename index_tt<Tag>::value_type>{}(ind.get()); }
    };

    // ----------------------------------------------------------------------

    template <typename Tag> class index_iterator_tt
    {
      public:
        explicit index_iterator_tt(index_tt<Tag> ind) : value_{ind} {}
        index_iterator_tt(const index_iterator_tt&) = default;
        index_iterator_tt(index_iterator_tt&&) = default;
        index_iterator_tt& operator=(const index_iterator_tt&) = default;
        index_iterator_tt& operator=(index_iterator_tt&&) = default;

        index_tt<Tag>& operator*() noexcept { return value_; }
        const index_tt<Tag>& operator*() const noexcept { return value_; }

        index_iterator_tt<Tag>& operator++() { ++value_; return *this; }
        index_iterator_tt<Tag> operator++(int) { const auto saved{*this}; ++value_; return saved; }
        index_iterator_tt<Tag>& operator--() { --value_; return *this; }
        index_iterator_tt<Tag> operator--(int) { const auto saved{*this}; --value_; return saved; }

        auto operator<=>(const index_iterator_tt&) const = default;

      private:
        index_tt<Tag> value_;
    };

    template <typename Tag> inline index_iterator_tt<Tag> index_tt<Tag>::begin() const { return index_iterator_tt<Tag>{index_tt<Tag>{0}}; }
    template <typename Tag> inline index_iterator_tt<Tag> index_tt<Tag>::end() const { return index_iterator_tt<Tag>{*this}; }

    // ----------------------------------------------------------------------

    using antigen_index = index_tt<struct antigen_index_tag>;
    using serum_index = index_tt<struct serum_index_tag>;
    using point_index = index_tt<struct point_index_tag>;
    using projection_index = index_tt<struct projection_index_tag>;
    using layer_index = index_tt<struct layer_index_tag>;

    // ----------------------------------------------------------------------

    using antigen_indexes = ae::named_vector_t<antigen_index, struct antigen_indexes_tag>;
    using serum_indexes = ae::named_vector_t<serum_index, struct serum_indexes_tag>;
    using point_indexes = ae::named_vector_t<point_index, struct point_indexes_tag>;

} // namespace ae

// ----------------------------------------------------------------------

template <typename Tag> struct fmt::formatter<ae::index_tt<Tag>> : fmt::formatter<typename ae::index_tt<Tag>::value_type>
{
    template <typename FormatCtx> auto format(const ae::index_tt<Tag>& nt, FormatCtx& ctx) const { return fmt::formatter<typename ae::index_tt<Tag>::value_type>::format(*nt, ctx); }
};

// ----------------------------------------------------------------------
