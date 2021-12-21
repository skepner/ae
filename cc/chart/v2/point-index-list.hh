#pragma once

#include "utils/named-vector.hh"
#include "ad/rjson-v2.hh"
#include "chart/v2/indexes.hh"
#include "ad/algorithm.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v2
{
    // sorted list of indexes
    class PointIndexList : public ae::named_vector_t<size_t, struct chart_PointIndexList_tag_t>
    {
      public:
        using difference_type = std::vector<size_t>::difference_type;
        using base_t = ae::named_vector_t<size_t, struct chart_PointIndexList_tag_t>;
        using const_iterator = std::vector<size_t>::const_iterator;

        using base_t::named_vector_t;

        PointIndexList(const PointIndexList& src) : base_t::named_vector_t(src.size()) { std::copy(std::begin(src), std::end(src), begin()); }
        PointIndexList(PointIndexList&& src) : base_t::named_vector_t(src.size()) { std::move(std::begin(src), std::end(src), begin()); }
        PointIndexList& operator=(const PointIndexList& src) = default;
        PointIndexList& operator=(PointIndexList&& src) = default;

        PointIndexList(const rjson::value& src) : base_t::named_vector_t(src.size()) { rjson::copy(src, begin()); }

        PointIndexList(std::initializer_list<size_t> src) : base_t::named_vector_t{src} {}

        template <typename Iter> PointIndexList(Iter first, Iter last) : base_t::named_vector_t(static_cast<size_t>(last - first))
        {
            std::copy(first, last, begin());
            std::sort(begin(), end());
        }

        template <typename Iter, typename Convert> PointIndexList(Iter first, Iter last, Convert convert) : base_t::named_vector_t(static_cast<size_t>(last - first))
        {
            std::transform(first, last, begin(), convert);
            std::sort(begin(), end());
        }

        template <typename Iter, typename Predicate, typename Convert>
        PointIndexList(Iter first, Iter last, Predicate predicate, Convert convert) : base_t::named_vector_t(static_cast<size_t>(last - first))
        {
            acmacs::transform_if(first, last, begin(), predicate, convert);
            std::sort(begin(), end());
        }

        template <typename Iter, typename Predicate, typename Convert>
        PointIndexList(Iter first, Iter last, size_t number_of_points, Predicate predicate, Convert convert) : base_t::named_vector_t(number_of_points)
        {
            acmacs::transform_if(first, last, begin(), predicate, convert);
            std::sort(begin(), end());
        }

        bool contains(size_t val) const
        {
            const auto found = std::lower_bound(begin(), end(), val);
            return found != end() && *found == val;
        }

        void insert(size_t val)
        {
            if (const auto found = std::lower_bound(begin(), end(), val); found == end() || *found != val)
                get().insert(found, val);
        }

        void erase(const_iterator ptr) { get().erase(ptr); }

        void erase_except(size_t val)
        {
            const auto present = contains(val);
            get().clear();
            if (present)
                insert(val);
        }

        void extend(const PointIndexList& source)
        {
            for (const auto no : source)
                insert(no);
        }

        void remove(const ReverseSortedIndexes& indexes, size_t base_index = 0)
        {
            get().erase(std::remove_if(begin(), end(), [&indexes, base_index](size_t index) { return index >= base_index && indexes.contains(index - base_index); }), end());
        }

        void keep(const ReverseSortedIndexes& indexes, size_t base_index = 0)
        {
            get().erase(std::remove_if(begin(), end(), [&indexes, base_index](size_t index) { return index >= base_index && !indexes.contains(index - base_index); }), end());
        }

        template <typename Pred> void remove_if(Pred pred) { get().erase(std::remove_if(begin(), end(), pred), end()); }

        void clear() { get().clear(); }

        PointIndexList& serum_index_to_point(size_t to_add)
        {
            for (auto& no : get())
                no += to_add;
            return *this;
        }

    }; // class PointIndexList

    // ----------------------------------------------------------------------

    class UnmovablePoints : public PointIndexList
    {
      public:
        using PointIndexList::PointIndexList;
        UnmovablePoints(const PointIndexList& src) : PointIndexList(src) {}
        UnmovablePoints(PointIndexList&& src) : PointIndexList(std::move(src)) {}
    };

    class UnmovableInTheLastDimensionPoints : public PointIndexList
    {
      public:
        using PointIndexList::PointIndexList;
        UnmovableInTheLastDimensionPoints(const PointIndexList& src) : PointIndexList(src) {}
        UnmovableInTheLastDimensionPoints(PointIndexList&& src) : PointIndexList(std::move(src)) {}
    };

    class DisconnectedPoints : public PointIndexList
    {
      public:
        using PointIndexList::PointIndexList;
        DisconnectedPoints(const PointIndexList& src) : PointIndexList(src) {}
        DisconnectedPoints(PointIndexList&& src) : PointIndexList(std::move(src)) {}
    };

} // namespace ae::chart::v2

// namespace acmacs
// {
//     inline std::string to_string(const ae::chart::v2::PointIndexList& indexes) { return to_string(*indexes); }

// } // namespace acmacs

// namespace ae::chart::v2
// {
//     inline std::ostream& operator<<(std::ostream& out, const PointIndexList& indexes) { return out << acmacs::to_string(indexes); }

// } // namespace ae::chart::v2

// ----------------------------------------------------------------------
