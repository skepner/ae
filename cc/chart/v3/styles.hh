#pragma once

#include <vector>
#include <string>

#include "utils/float.hh"
#include "chart/v3/point-style.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3::semantic
{
    enum class DrawingOrderModifier { no_change, raise, lower };
    enum class SelectAntigensSera { all, antigens_only, sera_only };

    struct Selector
    {
        std::string attribute{};
        std::string value{};

        bool empty() const { return attribute.empty(); }
    };

    struct LegendRow
    {
        int priority{0};
        std::string text{};
    };

    struct Legend
    {
        enum class v_relative { top, center, bottom };
        enum class h_relative { left, center, right };
        static constexpr double default_point_size = 16.0;

        bool shown{true};
        // Offset offset{10, 10};
        v_relative vrelative{v_relative::top};
        h_relative hrelative{h_relative::left};
        // padding [top, right, bottom, left]
        Color border{"black"};
        Float border_width{1.0};
        Color background{"white"};
        bool add_counter{true};
        Float point_size{default_point_size};
        bool show_rows_with_zero_count{true};

        // title
        // "t" | str                              | title text
        //  "f" | str                              | font face
        //  "S" | str                              | font slant: "normal" (default), "italic"
        //  "W" | str                              | font weight: "normal" (default), "bold"
        //  "s" | float                            | label size, default 1.0
        //  "c" | color                            | label color, default: "black"

        bool operator==(const Legend&) const = default;
        bool empty() const { return *this == Legend{}; }
    };

    struct StyleModifier
    {
        std::string parent{};
        Selector selector{};
        SelectAntigensSera select_antigens_sera{SelectAntigensSera::all};
        PointStyle point_style{};
        DrawingOrderModifier order{DrawingOrderModifier::no_change};
        LegendRow legend{};
    };

    struct Style
    {
        std::string name{};
        std::string title{};
        int priority{0};
        // viewport
        std::vector<StyleModifier> modifiers{};
        Legend legend{};

        Style() = default;
        Style(std::string_view a_name) : name{a_name} {}
    };

    class Styles
    {
      public:
        // Styles() = default;
        // Styles(const Styles&) = default;
        // Styles(Styles&&) = default;
        // Styles& operator=(const Styles&) = default;
        // Styles& operator=(Styles&&) = default;

        bool empty() const { return styles_.empty(); }
        size_t size() const { return styles_.size(); }
        auto begin() { return styles_.begin(); }
        auto end() { return styles_.end(); }
        auto begin() const { return styles_.begin(); }
        auto end() const { return styles_.end(); }

        // find or add style by name
        Style& find(std::string_view name);

      private:
        std::vector<Style> styles_{};
    };
}

// ----------------------------------------------------------------------
