#pragma once

#include "utils/named-vector.hh"
#include "chart/v3/index.hh"
#include "chart/v3/point-style.hh"

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

        // virtual size_t number_of_points() const = 0;
        // virtual bool empty() const = 0;
        // virtual PointStyle style(size_t aPointNo) const = 0;
        // virtual PointStyle& style_ref(size_t /*aPointNo*/) { throw std::runtime_error{"PointStyles::style_ref not supported for this style collection"}; }

        const DrawingOrder& drawing_order() const { return drawing_order_; }
        DrawingOrder& drawing_order() { return drawing_order_; }
        const std::vector<size_t>& style_for_point() const { return style_for_point_; }
        std::vector<size_t>& style_for_point() { return style_for_point_; }
        const std::vector<PointStyle>& styles() const { return styles_; }
        std::vector<PointStyle>& styles() { return styles_; }

        Color error_line_positive_color() const { return error_line_positive_color_; }
        void error_line_positive_color(Color color) { error_line_positive_color_ = color; }
        Color error_line_negative_color() const { return error_line_negative_color_; }
        void error_line_negative_color(Color color) { error_line_negative_color_ = color; }

        // virtual std::vector<acmacs::PointStyle> all_styles() const = 0;

        //   acmacs::PointStyle style(size_t aPointNo) const { return modified() ? style_modified(aPointNo) : main_->style(aPointNo); }
        //   std::vector<acmacs::PointStyle> all_styles() const { return modified() ? styles_ : main_->all_styles(); }
        //   size_t number_of_points() const { return modified() ? styles_.size() : main_->number_of_points(); }

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
