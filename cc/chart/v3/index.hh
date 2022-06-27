#pragma once

#include <concepts>

#include "ext/fmt.hh"
#include "ext/range-v3.hh"
#include "utils/named-vector.hh"

// ----------------------------------------------------------------------

namespace ae
{
    template <typename Tag> class index_iterator_tt;

    enum index_tt_invalid_ { invalid_index };

    template <typename Tag> class index_tt
    {
      public:
        using value_type = size_t;

        explicit constexpr index_tt() : value_{0} {}
        constexpr index_tt(index_tt_invalid_) : value_{static_cast<value_type>(-1)} {}
        constexpr index_tt(const index_tt&) = default;
        constexpr index_tt(index_tt&&) = default;
        template <typename T2> requires std::constructible_from<value_type, T2> constexpr explicit index_tt(T2&& value) : value_(std::forward<T2>(value)) {}

        constexpr index_tt& operator=(const index_tt&) = default;
        constexpr index_tt& operator=(index_tt&&) = default;

        template <typename T2> requires std::assignable_from<value_type&, T2> index_tt& operator=(const T2& value)
        {
            value_ = value;
            return *this;
        }

        template <typename T2> requires std::assignable_from<value_type&, T2> index_tt& operator=(T2&& value)
        {
            value_ = std::move(value);
            return *this;
        }

        constexpr auto operator<=>(const index_tt&) const = default;

        constexpr value_type& get() noexcept { return value_; }
        constexpr const value_type& get() const noexcept { return value_; }
        constexpr value_type& operator*() noexcept { return value_; }
        constexpr const value_type& operator*() const noexcept { return value_; }
        constexpr const value_type* operator->() const noexcept { return &value_; }
        explicit constexpr operator value_type&() noexcept { return value_; }
        explicit constexpr operator const value_type&() const noexcept { return value_; }

        index_tt<Tag>& operator++()
        {
            ++this->get();
            return *this;
        }
        index_tt<Tag> operator++(int)
        {
            const auto saved{*this};
            ++this->get();
            return saved;
        }
        index_tt<Tag>& operator--()
        {
            --this->get();
            return *this;
        }
        index_tt<Tag> operator--(int)
        {
            const auto saved{*this};
            --this->get();
            return saved;
        }

        index_tt<Tag> operator+(index_tt<Tag> rhs) const { return index_tt<Tag>{get() + rhs.get()}; }
        template <typename Number> index_tt<Tag> operator+(Number rhs) const { return index_tt<Tag>{get() + rhs}; }
        index_tt<Tag> operator-(index_tt<Tag> rhs) const { return index_tt<Tag>{get() - rhs.get()}; }
        template <typename Number> index_tt<Tag> operator-(Number rhs) const { return index_tt<Tag>{get() - rhs}; }

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

        index_iterator_tt<Tag>& operator++()
        {
            ++value_;
            return *this;
        }
        index_iterator_tt<Tag> operator++(int)
        {
            const auto saved{*this};
            ++value_;
            return saved;
        }
        index_iterator_tt<Tag>& operator--()
        {
            --value_;
            return *this;
        }
        index_iterator_tt<Tag> operator--(int)
        {
            const auto saved{*this};
            --value_;
            return saved;
        }

        auto operator<=>(const index_iterator_tt&) const = default;

      private:
        index_tt<Tag> value_;
    };

    template <typename Tag> inline index_iterator_tt<Tag> index_tt<Tag>::begin() const { return index_iterator_tt<Tag>{index_tt<Tag>{0}}; }
    template <typename Tag> inline index_iterator_tt<Tag> index_tt<Tag>::end() const { return index_iterator_tt<Tag>{*this}; }

    template <typename Tag> inline ae::named_vector_t<index_tt<Tag>, Tag> index_range(index_tt<Tag> last)
    {
        ae::named_vector_t<index_tt<Tag>, Tag> result(*last);
        std::iota(result.begin(), result.end(), index_tt<Tag>{0});
        return result;
    }

    // ----------------------------------------------------------------------

    using antigen_index = index_tt<struct antigen_index_tag>;
    using serum_index = index_tt<struct serum_index_tag>;
    using point_index = index_tt<struct point_index_tag>;
    using projection_index = index_tt<struct projection_index_tag>;
    using layer_index = index_tt<struct layer_index_tag>;

    using number_of_dimensions_t = index_tt<struct number_of_dimensions_tag>;
    inline constexpr bool valid(number_of_dimensions_t nd) { return nd.get() > 0; }
    inline constexpr const number_of_dimensions_t DIMX{0};
    inline constexpr const number_of_dimensions_t DIMY{1};
    inline constexpr const number_of_dimensions_t DIMZ{2};
    inline constexpr const number_of_dimensions_t DIM2{2};
    inline constexpr const number_of_dimensions_t DIM3{3};

    // inline antigen_index operator+(antigen_index ag1, antigen_index ag2) { return antigen_index{ag1.get() + ag2.get()}; }
    // inline serum_index operator+(serum_index sr1, serum_index sr2) { return serum_index{sr1.get() + sr2.get()}; }
    inline point_index operator+(antigen_index ag_no, serum_index sr_no) { return point_index{ag_no.get() + sr_no.get()}; }
    inline size_t operator*(point_index point_no, number_of_dimensions_t num_dim) { return point_no.get() * num_dim.get(); }
    inline point_index to_point_index(const antigen_index& agi) { return point_index{*agi}; }
    inline antigen_index to_antigen_index(const point_index& pi) { return antigen_index{*pi}; }

    // ----------------------------------------------------------------------

    using antigen_indexes = ae::named_vector_t<antigen_index, struct antigen_index_tag>;
    using serum_indexes = ae::named_vector_t<serum_index, struct serum_index_tag>;
    using point_indexes = ae::named_vector_t<point_index, struct point_index_tag>;
    using layer_indexes = ae::named_vector_t<layer_index, struct layer_index_tag>;

    inline point_indexes to_point_indexes(const antigen_indexes& agi, antigen_index = antigen_index{0})
    {
        point_indexes pi{ranges::views::transform(agi, [](antigen_index index) { return point_index{*index}; }) | ranges::to_vector};
        ranges::sort(pi);
        return pi;
    }
    inline point_indexes to_point_indexes(const serum_indexes& sri, antigen_index number_of_antigens)
    {
        point_indexes pi{ranges::views::transform(sri, [number_of_antigens](serum_index index) { return number_of_antigens + index; }) | ranges::to_vector};
        ranges::sort(pi);
        return pi;
    }

    template <typename Index, typename Tag> inline std::vector<typename Index::value_type> to_vector_base_t(const ae::named_vector_t<Index, Tag>& source)
    {
        std::vector<typename Index::value_type> result(source.size());
        std::transform(source.begin(), source.end(), result.begin(), [](auto src) { return *src; });
        return result;
    }

    template <typename Index, typename Tag> inline std::vector<typename Index::value_type> to_vector_base_t_descending(const ae::named_vector_t<Index, Tag>& source)
    {
        std::vector<typename Index::value_type> result(source.size());
        std::transform(source.begin(), source.end(), result.begin(), [](auto src) { return *src; });
        std::sort(result.begin(), result.end(), [](auto i1, auto i2) { return i1 > i2; });
        return result;
    }

    // indexes is an ascending (after sorting below) list of indexes
    // to_remove_sorted_ascending is a sorted list of indexes to remove
    // indexes has to be renumbered upon each removal
    template <typename Index, typename Tag> inline void remove_and_renumber(ae::named_vector_t<Index, Tag>& indexes, const point_indexes& to_remove_sorted_ascending)
    {
        if (!to_remove_sorted_ascending.empty()) {
            std::sort(indexes.begin(), indexes.end());
            auto no_ptr = to_remove_sorted_ascending.begin();
            auto target = indexes.begin();
            for (auto src = indexes.begin(); src != indexes.end(); ++src) {
                while (*src > *no_ptr && no_ptr != to_remove_sorted_ascending.end())
                    ++no_ptr;
                if (no_ptr != to_remove_sorted_ascending.end()) {
                    if (*src < *no_ptr) {
                        *target = *src - (no_ptr - to_remove_sorted_ascending.begin());
                        ++target;
                    }
                    else if (*src == *no_ptr) {
                        // skip src (remove)
                        ++no_ptr;
                    }
                    else {
                        ++no_ptr;
                    }
                }
                if (no_ptr == to_remove_sorted_ascending.end()) {
                    // just copy and renumber rest
                    target = std::transform(src, indexes.end(), target, [diff = no_ptr - to_remove_sorted_ascending.begin()](const auto& en) { return en - diff; });
                    break;
                }
            }
            // truncate
            indexes.get().erase(target, indexes.end());
        }
    }

    class disconnected_points : public point_indexes
    {
      public:
        using point_indexes::point_indexes;
    };
    class unmovable_points : public point_indexes
    {
      public:
        using point_indexes::point_indexes;
    };
    class unmovable_in_the_last_dimension_points : public point_indexes
    {
      public:
        using point_indexes::point_indexes;
    };

} // namespace ae

// ----------------------------------------------------------------------

template <typename Tag> struct fmt::formatter<ae::index_tt<Tag>> : fmt::formatter<typename ae::index_tt<Tag>::value_type>
{
    template <typename FormatCtx> auto format(const ae::index_tt<Tag>& nt, FormatCtx& ctx) const { return fmt::formatter<typename ae::index_tt<Tag>::value_type>::format(*nt, ctx); }
};

// ----------------------------------------------------------------------
