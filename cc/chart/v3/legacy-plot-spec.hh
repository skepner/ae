#pragma once


#include "chart/v3/point-style.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    // ----------------------------------------------------------------------

    class LegacyPlotSpec
    {
      public:
        LegacyPlotSpec() = default;
        LegacyPlotSpec(const LegacyPlotSpec&) = default;
        LegacyPlotSpec(LegacyPlotSpec&&) = default;
        LegacyPlotSpec& operator=(const LegacyPlotSpec&) = default;
        LegacyPlotSpec& operator=(LegacyPlotSpec&&) = default;

        // virtual size_t number_of_points() const = 0;
        // virtual bool empty() const = 0;
        // virtual PointStyle style(size_t aPointNo) const = 0;
        // virtual PointStyle& style_ref(size_t /*aPointNo*/) { throw std::runtime_error{"PointStyles::style_ref not supported for this style collection"}; }
        // virtual PointStylesCompacted compacted() const = 0;

        // virtual DrawingOrder drawing_order() const = 0;
        // virtual Color error_line_positive_color() const = 0;
        // virtual Color error_line_negative_color() const = 0;
        // virtual std::vector<acmacs::PointStyle> all_styles() const = 0;
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
