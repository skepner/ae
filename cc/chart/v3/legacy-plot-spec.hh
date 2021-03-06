#pragma once

#include "utils/named-vector.hh"
#include "chart/v3/index.hh"
#include "chart/v3/point-style.hh"
#include <iterator>

// ----------------------------------------------------------------------

namespace ae::chart::v3::legacy
{
    using DrawingOrder = ae::named_vector_t<point_index, struct chart_DrawingOrder_tag_t>;

    // ----------------------------------------------------------------------

    class PlotSpec
    {
      public:
        PlotSpec() = default;
        PlotSpec(const PlotSpec&) = default;
        PlotSpec(PlotSpec&&) = default;
        PlotSpec& operator=(const PlotSpec&) = default;
        PlotSpec& operator=(PlotSpec&&) = default;

        bool empty() const { return drawing_order_.empty() && styles_.empty(); }

        const auto& drawing_order() const { return drawing_order_; }
        auto& drawing_order() { return drawing_order_; }
        const auto& style_for_point() const { return style_for_point_; }
        auto& style_for_point() { return style_for_point_; }
        const auto& styles() const { return styles_; }
        auto& styles() { return styles_; }

        Color error_line_positive_color() const { return error_line_positive_color_; }
        void error_line_positive_color(Color color) { error_line_positive_color_ = color; }
        Color error_line_negative_color() const { return error_line_negative_color_; }
        void error_line_negative_color(Color color) { error_line_negative_color_ = color; }

        void initialize(antigen_index number_of_antigens, const antigen_indexes& reference, serum_index number_of_sera)
        {
            styles_.clear();
            style_for_point_.clear();

            auto& test_antigen = styles_.emplace_back();
            test_antigen.fill(Color{"green"});
            auto& reference_antigen = styles_.emplace_back();
            reference_antigen.size(1.3);
            auto& serum = styles_.emplace_back();
            serum.size(1.3);
            serum.shape(point_shape::Box);

            for ([[maybe_unused]] const auto ag_no : number_of_antigens)
                style_for_point_.push_back(0);
            for ([[maybe_unused]] const auto sr_no : number_of_sera)
                style_for_point_.push_back(2);
            for (const auto ag_no : reference)
                style_for_point_[*ag_no] = 1;

            drawing_order_.get().clear();
            // sera followed by reference antigens followed by test antigens
            ranges::copy(ranges::views::iota(to_point_index(number_of_antigens), number_of_antigens + number_of_sera), std::back_inserter(*drawing_order_));
            ranges::copy(to_point_indexes(reference), std::back_inserter(*drawing_order_));
            ranges::copy(ranges::views::iota(antigen_index{0}, number_of_antigens)                                //
                             | ranges::views::filter([&reference](auto ind) { return !reference.contains(ind); }) //
                             | ranges::views::transform(&to_point_index),
                         std::back_inserter(*drawing_order_));
        }

        void remove_points(const point_indexes& points)
        {
            if (!style_for_point_.empty()) {
                for (const auto ind : to_vector_base_t_descending(points))
                    style_for_point_.erase(std::next(style_for_point_.begin(), ind));
            }
            if (!drawing_order_.empty())
                remove_and_renumber(drawing_order_, points);
        }

      private:
        // antigen_index number_of_antigens_{};
        std::vector<PointStyle> styles_{};
        std::vector<size_t> style_for_point_{};
        DrawingOrder drawing_order_{};
        Color error_line_positive_color_{"blue"};
        Color error_line_negative_color_{"red"};
    };

} // namespace ae::chart::v3::legacy

// ----------------------------------------------------------------------
