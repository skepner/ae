#pragma once

#include <iterator>

// ----------------------------------------------------------------------

namespace acmacs::chart::v2
{
    template <typename Index> class index_iterator
    {
      public:
        using difference_type = long;
        using value_type = Index;
        using pointer = Index*;
        using reference = Index&;
        using iterator_category = std::random_access_iterator_tag;

        index_iterator(value_type aValue) : value(aValue) {}
        template <typename A1> index_iterator(A1 aValue) : value(value_type{aValue}) {}
        index_iterator& operator++()
        {
            ++value;
            return *this;
        }
        // index_iterator operator++(int) { iterator retval = *this; ++(*this); return retval;}
        bool operator==(const index_iterator<Index>& other) const { return value == other.value; }
        bool operator!=(const index_iterator<Index>& other) const { return !(*this == other); }
        value_type operator*() const { return value; }
        difference_type operator-(const index_iterator<Index>& other) const { return static_cast<difference_type>(value) - static_cast<difference_type>(other.value); }

      private:
        value_type value;

    }; // class index_iterator<>

    index_iterator(size_t)->index_iterator<size_t>;
    index_iterator(int)->index_iterator<int>;
    index_iterator(long)->index_iterator<long>;

} // namespace acmacs::chart::v2

// ----------------------------------------------------------------------
