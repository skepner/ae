#pragma once

#include "utils/string.hh"
#include "ext/range-v3.hh"
#include "chart/v2/chart-modify.hh"
#include "chart/v2/name-format.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v2
{
    namespace detail
    {
        template <typename AgSr, typename Chrt> struct SelectedIterator;

        template <typename AgSr, typename Chrt> struct Selected
        {
            using AntigensSeraType = AgSr;
            enum None { None };

            // Selected() = default;
            Selected(std::shared_ptr<Chrt> a_chart) : chart{a_chart}, indexes{ag_sr()->all_indexes()} {}
            Selected(std::shared_ptr<Chrt> a_chart, enum None) : chart{a_chart}, indexes{} {}
            // call func for each antigen/serum and select ag/sr if func returns true
            template <typename F> Selected(std::shared_ptr<Chrt> a_chart, F&& func, size_t projection_no) : chart{a_chart}, indexes{a_chart->template indexes<AgSr, F>(*ag_sr(), std::forward<F>(func), projection_no)} {}

            std::shared_ptr<AgSr> ag_sr() const;

            bool empty() const { return indexes.empty(); }
            size_t size() const { return indexes.size(); }

            auto operator[](size_t no) const
            {
                // no is not a antigen_no/serum_no, it's no in index, i.e. 0 to size()
                const auto ag_sr_no = indexes[no];
                return std::pair{ag_sr_no, ag_sr()->ptr_at(ag_sr_no)};
            }

            // substitutions in format: {no0} {no1} {ag_sr} {name} {full_name}
            std::string report(std::string_view format = "{no0},") const
            {
                fmt::memory_buffer out;
                for (const auto index : indexes)
                    format_antigen_serum<AgSr>(out, format, *chart, index, collapse_spaces_t::yes);
                return fmt::to_string(out);
            }

            // substitutions in format: {no0} {no1} {ag_sr} {name} {full_name}
            std::vector<std::string> report_list(std::string_view format = "{name}") const
            {
                std::vector<std::string> result(indexes.size());
                std::transform(std::begin(indexes), std::end(indexes), std::begin(result), [this, format](auto index) {
                    fmt::memory_buffer out;
                    format_antigen_serum<AgSr>(out, format, *chart, index, collapse_spaces_t::yes);
                    return fmt::to_string(out);
                });
                return result;
            }

            void for_each(const std::function<void(size_t, std::shared_ptr<typename AgSr::AntigenSerumType>)>& modifier)
            {
                for (const auto index : indexes)
                    modifier(index, ag_sr()->ptr_at(index));
            }

            PointIndexList points() const
            {
                if constexpr (std::is_same_v<Antigens, AgSr> || std::is_same_v<AntigensModify, AgSr>)
                    return indexes;
                else
                    return PointIndexList{ranges::views::transform(indexes, [number_of_antigens = chart->number_of_antigens()](size_t index) { return index + number_of_antigens; }) |
                                          ranges::to_vector};
            }

            Area area(size_t projection_no) const { return chart->projection(projection_no)->layout()->area(*points()); }

            Area area_transformed(size_t projection_no) const { return chart->projection(projection_no)->transformed_layout()->area(*points()); }

            void exclude(const Selected<AgSr, Chrt>& to_exclude) { indexes.remove(ReverseSortedIndexes{*to_exclude.indexes}); }

            SelectedIterator<AgSr, Chrt> begin();
            SelectedIterator<AgSr, Chrt> end();

            SelectedIterator<AgSr, Chrt> begin() const;
            SelectedIterator<AgSr, Chrt> end() const;

            std::shared_ptr<Chrt> chart;
            PointIndexList indexes;
        };

        template <typename AgSr, typename Chrt> struct SelectedIterator
        {
          public:
            SelectedIterator(const Selected<AgSr, Chrt>& parent, typename PointIndexList::const_iterator current) : parent_{parent}, current_{current} {}

            SelectedIterator& operator++()
            {
                ++current_;
                return *this;
            }

            auto operator*() { return parent_[static_cast<size_t>(current_ - parent_.indexes.begin())]; }

            bool operator==(const SelectedIterator& rhs) const { return current_ == rhs.current_; }

          private:
            Selected<AgSr, Chrt> parent_;
            PointIndexList::const_iterator current_;
        };

        template <typename AgSr, typename Chrt> SelectedIterator<AgSr, Chrt> Selected<AgSr, Chrt>::begin() { return SelectedIterator<AgSr, Chrt>{*this, indexes.begin()}; }
        template <typename AgSr, typename Chrt> SelectedIterator<AgSr, Chrt> Selected<AgSr, Chrt>::end() { return SelectedIterator<AgSr, Chrt>{*this, indexes.end()}; }

        template <typename AgSr, typename Chrt> SelectedIterator<AgSr, Chrt> Selected<AgSr, Chrt>::begin() const { return SelectedIterator<AgSr, Chrt>{*this, indexes.begin()}; }
        template <typename AgSr, typename Chrt> SelectedIterator<AgSr, Chrt> Selected<AgSr, Chrt>::end() const { return SelectedIterator<AgSr, Chrt>{*this, indexes.end()}; }

        template <> inline std::shared_ptr<Antigens> Selected<Antigens, Chart>::ag_sr() const { return chart->antigens(); }
        template <> inline std::shared_ptr<AntigensModify> Selected<AntigensModify, ChartModify>::ag_sr() const { return chart->antigens_modify_ptr(); }
        template <> inline std::shared_ptr<Sera> Selected<Sera, Chart>::ag_sr() const { return chart->sera(); }
        template <> inline std::shared_ptr<SeraModify> Selected<SeraModify, ChartModify>::ag_sr() const { return chart->sera_modify_ptr(); }

    } // namespace detail

    struct SelectedAntigens : public detail::Selected<Antigens, Chart>
    {
        using detail::Selected<Antigens, Chart>::Selected;
    };

    struct SelectedAntigensModify : public detail::Selected<AntigensModify, ChartModify>
    {
        using detail::Selected<AntigensModify, ChartModify>::Selected;
    };

    struct SelectedSera : public detail::Selected<Sera, Chart>
    {
        using detail::Selected<Sera, Chart>::Selected;
    };

    struct SelectedSeraModify : public detail::Selected<SeraModify, ChartModify>
    {
        using detail::Selected<SeraModify, ChartModify>::Selected;
    };

} // namespace ae::chart::v2

// ----------------------------------------------------------------------
