#pragma once

#include <iterator>

// ======================================================================

namespace acmacs
{
    template <typename Parent, typename Reference, typename Index = size_t> class iterator
    {
      public:
        using reference = Reference;
        using pointer = typename std::add_pointer<Reference>::type;
        using value_type = typename std::remove_reference<Reference>::type;
        using difference_type = ssize_t;
        using iterator_category = std::random_access_iterator_tag;

        constexpr iterator& operator++()
        {
            ++mIndex;
            return *this;
        }
        constexpr iterator& operator+=(difference_type n)
        {
            mIndex += n;
            return *this;
        }
        constexpr iterator& operator-=(difference_type n)
        {
            mIndex -= n;
            return *this;
        }
        constexpr iterator operator-(difference_type n)
        {
            iterator temp = *this;
            return temp -= n;
        }
        constexpr difference_type operator-(const iterator& rhs) { return mIndex - rhs.mIndex; }
        constexpr bool operator==(const iterator& other) const { return &mParent == &other.mParent && mIndex == other.mIndex; }
        constexpr bool operator!=(const iterator& other) const { return &mParent != &other.mParent || mIndex != other.mIndex; }
        constexpr reference operator*() { return mParent[mIndex]; }
        constexpr Index index() const { return mIndex; }
        constexpr bool operator<(const iterator& rhs) const { return mIndex < rhs.mIndex; }
        constexpr bool operator<=(const iterator& rhs) const { return mIndex <= rhs.mIndex; }
        constexpr bool operator>(const iterator& rhs) const { return mIndex > rhs.mIndex; }
        constexpr bool operator>=(const iterator& rhs) const { return mIndex >= rhs.mIndex; }

      private:
        iterator(const Parent& aParent, Index aIndex) : mParent{aParent}, mIndex{aIndex} {}

        const Parent& mParent;
        Index mIndex;

        friend Parent;
    };

} // namespace acmacs

// ======================================================================
