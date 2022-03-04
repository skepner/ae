#pragma once

#include <concepts>

#include "ext/fmt.hh"
#include "utils/named-vector.hh"

// ----------------------------------------------------------------------

namespace ae
{
    template <typename Derived, typename Tag> class index_iterator_tt;

    template <typename Derived, typename Tag> class index_tt
    {
      public:
        using value_type = size_t;
        using base_type = index_tt<Derived, Tag>;

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
        operator Derived&() noexcept { return *static_cast<Derived*>(this); }
        operator const Derived&() const noexcept { return *static_cast<const Derived*>(this); }

        index_tt<Derived, Tag>& operator++() { ++this->get(); return *this; }
        index_tt<Derived, Tag> operator++(int) { const auto saved{*this}; ++this->get(); return saved; }
        index_tt<Derived, Tag>& operator--() { --this->get(); return *this; }
        index_tt<Derived, Tag> operator--(int) { const auto saved{*this}; --this->get(); return saved; }

        Derived operator+(Derived rhs) { return Derived{get() + rhs.get()}; }

        // to iterate from 0 to this index
        index_iterator_tt<Derived, Tag> begin() const;
        index_iterator_tt<Derived, Tag> end() const;

      protected:
        value_type value_{};
    };

    struct index_hash_for_unordered_map
    {
        using is_transparent = void;
        template <typename Derived, typename Tag> size_t operator()(const index_tt<Derived, Tag>& ind) const { return std::hash<typename index_tt<Derived, Tag>::value_type>{}(ind.get()); }
    };

    // template <typename Tag> inline index_tt<Tag> operator+(index_tt<Tag> lhs, index_tt<Tag> rhs) { return index_tt<Tag>{lhs.get() + rhs.get()}; }

    // ----------------------------------------------------------------------

    template <typename Derived, typename Tag> class index_iterator_tt
    {
      public:
        explicit index_iterator_tt(Derived ind) : value_{ind} {}
        index_iterator_tt(const index_iterator_tt&) = default;
        index_iterator_tt(index_iterator_tt&&) = default;
        index_iterator_tt& operator=(const index_iterator_tt&) = default;
        index_iterator_tt& operator=(index_iterator_tt&&) = default;

        Derived& operator*() noexcept { return value_; }
        const Derived& operator*() const noexcept { return value_; }

        index_iterator_tt<Derived, Tag>& operator++() { ++value_; return *this; }
        index_iterator_tt<Derived, Tag> operator++(int) { const auto saved{*this}; ++value_; return saved; }
        index_iterator_tt<Derived, Tag>& operator--() { --value_; return *this; }
        index_iterator_tt<Derived, Tag> operator--(int) { const auto saved{*this}; --value_; return saved; }

        auto operator<=>(const index_iterator_tt&) const = default;

      private:
        Derived value_;
    };

    template <typename Derived, typename Tag> inline index_iterator_tt<Derived, Tag> index_tt<Derived, Tag>::begin() const { return index_iterator_tt<Derived, Tag>{Derived{0}}; }
    template <typename Derived, typename Tag> inline index_iterator_tt<Derived, Tag> index_tt<Derived, Tag>::end() const { return index_iterator_tt<Derived, Tag>{*this}; }

    // ----------------------------------------------------------------------

    class antigen_index : public index_tt<antigen_index, struct antigen_index_tag>
    {
      public:
        using index_tt<antigen_index, struct antigen_index_tag>::index_tt;
    };

    class serum_index : public index_tt<serum_index, struct serum_index_tag>
    {
      public:
        using index_tt<serum_index, struct serum_index_tag>::index_tt;
    };

    class point_index : public index_tt<point_index, struct point_index_tag>
    {
      public:
        using index_tt<point_index, struct point_index_tag>::index_tt;
    };

    class projection_index : public index_tt<projection_index, struct projection_index_tag>
    {
      public:
        using index_tt<projection_index, struct projection_index_tag>::index_tt;
    };

    class layer_index : public index_tt<layer_index, struct layer_index_tag>
    {
      public:
        using index_tt<layer_index, struct layer_index_tag>::index_tt;
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

    class point_indexes : public ae::named_vector_t<point_index, struct point_indexes_tag>
    {
      public:
        using ae::named_vector_t<point_index, struct point_indexes_tag>::named_vector_t;
    };

} // namespace ae

// ----------------------------------------------------------------------

template <typename Derived, typename Tag> struct fmt::formatter<ae::index_tt<Derived, Tag>> : fmt::formatter<typename ae::index_tt<Derived, Tag>::value_type>
{
    template <typename FormatCtx> auto format(const ae::index_tt<Derived, Tag>& nt, FormatCtx& ctx) const { return fmt::formatter<typename ae::index_tt<Derived, Tag>::value_type>::format(*nt, ctx); }
};

template <> struct fmt::formatter<ae::antigen_index> : fmt::formatter<typename ae::antigen_index::value_type>
{
    template <typename FormatCtx> auto format(const ae::antigen_index& nt, FormatCtx& ctx) const { return fmt::formatter<typename ae::antigen_index::value_type>::format(*nt, ctx); }
};

template <> struct fmt::formatter<ae::serum_index> : fmt::formatter<typename ae::serum_index::value_type>
{
    template <typename FormatCtx> auto format(const ae::serum_index& nt, FormatCtx& ctx) const { return fmt::formatter<typename ae::serum_index::value_type>::format(*nt, ctx); }
};

// template <> struct fmt::formatter<ae::antigen_index> : fmt::formatter<ae::index_tt<ae::antigen_index, struct ae::antigen_index_tag>> {};

// ----------------------------------------------------------------------
