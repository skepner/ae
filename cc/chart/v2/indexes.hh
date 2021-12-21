#pragma once

#include <vector>
#include <algorithm>

#include "ext/range-v3.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v2
{
    using Indexes = std::vector<size_t>;

    class SortedIndexes : public Indexes
    {
     public:
        explicit SortedIndexes(const Indexes& source)
            : Indexes(source) { std::sort(begin(), end()); }
    };

    class ReverseSortedIndexes : public Indexes
    {
     public:
        explicit ReverseSortedIndexes() = default;
        explicit ReverseSortedIndexes(const Indexes& source) : Indexes(source) { sort(); }
        explicit ReverseSortedIndexes(size_t range) : ReverseSortedIndexes(range_from_0_to(range) | ranges::to_vector) {}

        void add(const Indexes& to_add) { insert(end(), to_add.begin(), to_add.end()); sort(); }
        void remove(const Indexes& to_remove)
        {
            erase(std::remove_if(begin(), end(), [&to_remove](size_t ind) -> bool { return std::find(to_remove.begin(), to_remove.end(), ind) != to_remove.end(); }), end());
        }

        bool contains(size_t look_for) const
        {
            const auto found = std::lower_bound(begin(), end(), look_for, cmp);
            return found != end() && *found == look_for;
        }

      private:
        static inline bool cmp(size_t i1, size_t i2) { return i1 > i2; }
        void sort() { std::sort(begin(), end(), cmp); erase(std::unique(begin(), end()), end()); }
    };

    template <typename T> void remove(const ReverseSortedIndexes& indexes, std::vector<T>& data, ReverseSortedIndexes::difference_type base_index = 0)
    {
        for (auto index : indexes)
            data.erase(data.begin() + static_cast<ReverseSortedIndexes::difference_type>(index) + base_index);
    }
}

// ----------------------------------------------------------------------
