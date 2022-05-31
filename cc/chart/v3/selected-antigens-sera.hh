#pragma once

#include <unordered_set>

#include "utils/string.hh"
#include "chart/v3/chart.hh"
// #include "chart/v3/name-format.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    template <typename AgSr> struct SelectedIterator;

    // ----------------------------------------------------------------------

    template <typename AgSr> struct SelectionData
    {
        std::shared_ptr<ae::chart::v3::Chart> chart;
        typename AgSr::index_t index{0};
        projection_index projection_no;
        const AgSr& ag_sr;
    };

    // ----------------------------------------------------------------------

    template <typename AgSr> struct Selected
    {
        using AntigensSeraType = AgSr;
        using index_t = typename AgSr::index_t;
        using indexes_t = typename AgSr::indexes_t;

        enum None { None };

        // all antigens/sera
        Selected(std::shared_ptr<Chart> a_chart) : chart{a_chart}, indexes(*a_chart->antigens_sera<AgSr>().size(), typename AgSr::index_t{0})
        {
            const auto num{a_chart->antigens_sera<AgSr>().size()};
            std::copy(num.begin(), num.end(), indexes.begin());
        }
        // no antigens/sera
        Selected(std::shared_ptr<Chart> a_chart, enum None) : chart{a_chart}, indexes{} {}
        // specified indexes
        Selected(std::shared_ptr<Chart> a_chart, const indexes_t& a_indexes) : chart{a_chart}, indexes{a_indexes} {}
        // call func for each antigen/serum and select ag/sr if func returns true
        template <typename F> Selected(std::shared_ptr<Chart> a_chart, F&& func, projection_index projection_no) : chart{a_chart}, indexes{}
        {
            // if (projection_no < a_chart->projections().size()) {
            //     // use transformed layout
            // }
            // else {
            // }

            const auto call = [&](auto no, const typename AgSr::element_t& ref) -> bool {
                return func(SelectionData<typename AgSr::element_t>{.chart = a_chart, .index = no, .projection_no = projection_no, .ag_sr = ref});
                // if constexpr (std::is_invocable_v<F, const typename AgSr::element_t&>)
                //     return func(ref);
                // else if constexpr (std::is_invocable_v<F, size_t, const typename AgSr::element_t&>)
                //     return func(no, ref);
                // else if constexpr (std::is_invocable_v<F, const SelectionData<typename AgSr::element_t>&>)
                //     return func(
                //         SelectionData<typename AgSr::element_t>>{.chart = a_chart, .index = no, .projection_no = projection_no, .ag_sr = ref});
                // else
                //     static_assert(std::is_invocable_v<F, void, int>, "unsupported filter function signature");
            };

            const auto& ag_sr = a_chart->antigens_sera<AgSr>();
            for (const auto no : ag_sr.size()) {
                if (call(no, ag_sr[no]))
                    indexes.push_back(no);
            }
        }

        std::shared_ptr<AgSr> ag_sr() const;

        bool empty() const { return indexes.empty(); }
        size_t size() const { return indexes.size(); }

        auto operator[](size_t no) const
        {
            // no is not a antigen_no/serum_no, it's no in index, i.e. 0 to size()
            const auto ag_sr_no = indexes[no];
            return std::pair<size_t, const typename AgSr::element_t&>{*ag_sr_no, chart->antigens_sera<AgSr>()[ag_sr_no]};
            // return std::pair<typename AgSr::index_t, const typename AgSr::element_t&>{ag_sr_no, chart->antigens_sera<AgSr>()[ag_sr_no]};
            // return chart->antigens_sera<AgSr>()[ag_sr_no];
        }

        // substitutions in format: {no0} {no1} {ag_sr} {name} {full_name}
        // std::string report(std::string_view format = "{no0},") const
        // {
        //     fmt::memory_buffer out;
        //     for (const auto index : indexes)
        //         format_antigen_serum<AgSr>(out, format, *chart, index, collapse_spaces_t::yes);
        //     return fmt::to_string(out);
        // }

        // // substitutions in format: {no0} {no1} {ag_sr} {name} {full_name}
        // std::vector<std::string> report_list(std::string_view format = "{name}") const
        // {
        //     std::vector<std::string> result(indexes.size());
        //     std::transform(std::begin(indexes), std::end(indexes), std::begin(result), [this, format](auto index) {
        //         fmt::memory_buffer out;
        //         format_antigen_serum<AgSr>(out, format, *chart, index, collapse_spaces_t::yes);
        //         return fmt::to_string(out);
        //     });
        //     return result;
        // }

        // void for_each(const std::function<void(size_t, std::shared_ptr<typename AgSr::AntigenSerumType>)>& modifier)
        // {
        //     for (const auto index : indexes)
        //         modifier(index, ag_sr()->ptr_at(index));
        // }

        point_indexes points() const { return to_point_indexes(indexes, chart->antigens().size()); }

        // Area area(projection_index projection_no) const { return chart->projection(projection_no)->layout()->area(*points()); }
        // Area area_transformed(projection_index projection_no) const { return chart->projection(projection_no)->transformed_layout()->area(*points()); }

        // void exclude(const Selected<AgSr>& to_exclude) { indexes.remove(ReverseSortedIndexes{*to_exclude.indexes}); }

        // if indexes have the same number of layers, first comes with the bigger layer no
        // if the biggest layers nos are the same, first comes the bigger index
        void sort_by_number_of_layers_descending()
        {
            const auto& titers = chart->titers();
            std::sort(std::begin(indexes), std::end(indexes), [&titers](auto i1, auto i2) {
                if (const auto ln1 = titers.layers_with(i1), ln2 = titers.layers_with(i2); ln1.size() == ln2.size()) {
                    if (ln1.back() == ln2.back())
                        return i1 > i2;
                    else
                        return ln1.back() > ln2.back();
                }
                else
                    return ln1.size() > ln2.size();
            });
        }

        void filter_new(const Antigens& compare_to)
        {
            std::unordered_set<std::string> designations;
            for (const auto& ag : compare_to)
                designations.insert(ag.designation());
            indexes.get().erase(
                std::remove_if(std::begin(indexes), std::end(indexes), [this, &designations](const auto& index) { return designations.contains(chart->antigens()[index].designation()); }),
                indexes.end());
        }

        SelectedIterator<AgSr> begin();
        SelectedIterator<AgSr> end();

        SelectedIterator<AgSr> begin() const;
        SelectedIterator<AgSr> end() const;

        std::shared_ptr<Chart> chart;
        indexes_t indexes;
    };

    template <typename AgSr> struct SelectedIterator
    {
      public:
        SelectedIterator(const Selected<AgSr>& parent, typename AgSr::indexes_t::const_iterator current) : parent_{parent}, current_{current} {}

        SelectedIterator& operator++()
        {
            ++current_;
            return *this;
        }

        auto operator*() { return parent_[current_ - parent_.indexes.begin()]; }

        bool operator==(const SelectedIterator& rhs) const { return current_ == rhs.current_; }

      private:
        Selected<AgSr> parent_;
        typename AgSr::indexes_t::const_iterator current_;
    };

    template <typename AgSr> SelectedIterator<AgSr> Selected<AgSr>::begin() { return SelectedIterator<AgSr>{*this, indexes.begin()}; }
    template <typename AgSr> SelectedIterator<AgSr> Selected<AgSr>::end() { return SelectedIterator<AgSr>{*this, indexes.end()}; }

    template <typename AgSr> SelectedIterator<AgSr> Selected<AgSr>::begin() const { return SelectedIterator<AgSr>{*this, indexes.begin()}; }
    template <typename AgSr> SelectedIterator<AgSr> Selected<AgSr>::end() const { return SelectedIterator<AgSr>{*this, indexes.end()}; }

    struct SelectedAntigens : public Selected<Antigens>
    {
        using Selected<Antigens>::Selected;
    };

    struct SelectedSera : public Selected<Sera>
    {
        using Selected<Sera>::Selected;
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
