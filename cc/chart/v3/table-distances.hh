#pragma once

#include <iostream>
#include <vector>
#include <algorithm>

#include "chart/v3/layout.hh"
#include "chart/v3/titers.hh"
#include "chart/v3/optimize-options.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    namespace detail
    {
        class DistancesBase
        {
          public:
            struct Entry
            {
                Entry(point_index p1, point_index p2, double dist) : point_1(p1), point_2(p2), distance{dist} {}
                point_index point_1;
                point_index point_2;
                double distance;
            };

            using entries_t = std::vector<Entry>;

            const entries_t& regular() const { return regular_; }
            entries_t& regular() { return regular_; }
            const entries_t& less_than() const { return less_than_; }
            entries_t& less_than() { return less_than_; }
            // const entries_t& more_than() const { return more_than_; }
            // entries_t& more_than() { return more_than_; }

            class IteratorForPoint
            {
              private:
                friend class DistancesBase;

                using iterator_t = typename entries_t::const_iterator;
                IteratorForPoint(point_index point_no, iterator_t it, iterator_t last) : point_no_(point_no), current_(it), last_(last)
                {
                    if (current_ != last_ && current_->point_1 != point_no_ && current_->point_2 != point_no_)
                        operator++();
                }

                point_index point_no_;
                iterator_t current_, last_;

              public:
                bool operator==(const IteratorForPoint& rhs) const { return current_ == rhs.current_; }
                bool operator!=(const IteratorForPoint& rhs) const { return !operator==(rhs); }
                const Entry& operator*() const { return *current_; }
                const Entry* operator->() const { return &*current_; }

                const IteratorForPoint& operator++()
                {
                    for (++current_; current_ != last_ && current_->point_1 != point_no_ && current_->point_2 != point_no_; ++current_)
                        ;
                    return *this;
                }

            }; // class IteratorForPoint

            IteratorForPoint begin_regular_for(point_index point_no) const { return IteratorForPoint(point_no, regular().begin(), regular().end()); }
            IteratorForPoint end_regular_for(point_index point_no) const { return IteratorForPoint(point_no, regular().end(), regular().end()); }
            IteratorForPoint begin_less_than_for(point_index point_no) const { return IteratorForPoint(point_no, less_than().begin(), less_than().end()); }
            IteratorForPoint end_less_than_for(point_index point_no) const { return IteratorForPoint(point_no, less_than().end(), less_than().end()); }
            // IteratorForPoint begin_more_than_for(point_index point_no) const { return IteratorForPoint(point_no, more_than().begin(), more_than().end()); }
            // IteratorForPoint end_more_than_for(point_index point_no) const { return IteratorForPoint(point_no, more_than().end(), more_than().end()); }

          private:
            entries_t regular_{};
            entries_t less_than_{};
            // entries_t more_than_{};

        }; // class DistancesBase

    } // namespace detail

// ----------------------------------------------------------------------

    class TableDistances : public detail::DistancesBase
    {
     public:
        using entries_t = typename detail::DistancesBase::entries_t;
        using detail::DistancesBase::regular;
        using detail::DistancesBase::less_than;
        // using detail::DistancesBase::more_than;

        void dodgy_is_regular(dodgy_titer_is_regular dodgy_is_regular) { dodgy_is_regular_ = dodgy_is_regular; }

        void update(const Titer& titer, point_index p1, point_index p2, double column_basis, double adjust, multiply_antigen_titer_until_column_adjust mult)
        {
            try {
                auto distance = column_basis - titer.logged() - adjust;
                if (distance < 0 && mult == multiply_antigen_titer_until_column_adjust::yes)
                    distance = 0;
                add_value(titer.type(), p1, p2, distance);
            }
            catch (invalid_titer&) {
                // ignore dont-care
            }
        }

        // void report() const { std::cerr << "TableDistances regular: " << regular().size() << "  less-than: " << less_than().size() << '\n'; }

        struct EntryForPoint
        {
            EntryForPoint(point_index ap, double td) : another_point(ap), distance(td) {}
            point_index another_point;
            double distance;
        };
        using entries_for_point_t = std::vector<EntryForPoint>;

        static entries_for_point_t entries_for_point(const entries_t& source, point_index point_no)
        {
            entries_for_point_t result;
            for (const auto& src : source) {
                if (src.point_1 == point_no)
                    result.emplace_back(src.point_2, src.distance);
                else if (src.point_2 == point_no)
                    result.emplace_back(src.point_1, src.distance);
            }
            return result;
        }

        struct EntriesForPoint
        {
            EntriesForPoint(point_index point_no, const TableDistances& table_distances)
                : regular(entries_for_point(table_distances.regular(), point_no)), less_than(entries_for_point(table_distances.less_than(), point_no))
            {
            }

            bool empty() const { return regular.empty() && less_than.empty(); }

            entries_for_point_t regular, less_than;
        };

        void add_value(Titer::Type type, point_index p1, point_index p2, double value)
        {
            switch (type) {
                case Titer::Dodgy:
                    if (dodgy_is_regular_ == dodgy_titer_is_regular::no)
                        break;
                    [[fallthrough]];
                case Titer::Regular:
                    regular().emplace_back(p1, p2, value);
                    break;
                case Titer::LessThan:
                    less_than().emplace_back(p1, p2, value);
                    break;
                case Titer::MoreThan:
                    // more_than().emplace_back(p1, p2, value);
                    // break;
                case Titer::Invalid:
                case Titer::DontCare:
                    break;
            }
        }

      private:
        dodgy_titer_is_regular dodgy_is_regular_{dodgy_titer_is_regular::no};

    }; // class TableDistances

    // ----------------------------------------------------------------------

    class MapDistances : public detail::DistancesBase
    {
     public:
       MapDistances(const Layout& layout, const TableDistances& table_distances)
       {
           auto make_map_distance = [&layout](const auto& table_distance_entry) -> Entry {
               return {table_distance_entry.point_1, table_distance_entry.point_2, layout.distance(table_distance_entry.point_1, table_distance_entry.point_2)};
           };
           std::transform(table_distances.regular().begin(), table_distances.regular().end(), std::back_inserter(regular()), make_map_distance);
           std::transform(table_distances.less_than().begin(), table_distances.less_than().end(), std::back_inserter(less_than()), make_map_distance);
           // std::transform(table_distances.more_than().begin(), table_distances.more_than().end(), std::back_inserter(more_than()), make_map_distance);
       }

    }; // class MapDistances

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

namespace std
{
    template <> struct iterator_traits<ae::chart::v3::detail::DistancesBase::IteratorForPoint>
    {
        using iterator_category = output_iterator_tag;
    };
}

// ----------------------------------------------------------------------
