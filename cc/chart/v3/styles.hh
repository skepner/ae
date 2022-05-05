#pragma once

#include <vector>
#include <array>
#include <string>
#include <optional>

#include "utils/float.hh"
#include "utils/log.hh"
#include "draw/v2/viewport.hh"
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

    using padding_t = std::array<double, 4>;

    struct AreaStyle
    {
        Color border_color{"black"};
        Float border_width{1.0};
        Color background{"white"};
        padding_t padding{0.0, 0.0, 0.0, 0.0};

        bool operator==(const AreaStyle&) const = default;
    };

    struct Title : public AreaStyle
    {
        ae::draw::v2::text_and_offset text{ae::draw::v2::offset_t{10.0, 10.0}};

        bool operator==(const Title&) const = default;
    };

    struct Legend : public AreaStyle
    {
        enum class v_relative { top, center, bottom };
        enum class h_relative { left, center, right };

        bool shown{true};
        ae::draw::v2::offset_t offset{10, 10};
        v_relative vrelative{v_relative::top};
        h_relative hrelative{h_relative::left};
        bool add_counter{true};
        Float point_size{16.0};
        bool show_rows_with_zero_count{true};
        Title title{};

        bool operator==(const Legend&) const = default;

        std::string format_relative() const
        {
            std::string res{"tl"};
            switch (vrelative) {
                case v_relative::top:
                    res[0] = 't';
                    break;
                case v_relative::bottom:
                    res[0] = 'b';
                    break;
                case v_relative::center:
                    res[0] = 'c';
                    break;
            }
            switch (hrelative) {
                case h_relative::left:
                    res[1] = 'l';
                    break;
                case h_relative::right:
                    res[1] = 'r';
                    break;
                case h_relative::center:
                    res[1] = 'c';
                    break;
            }
            return res;
        }

        void relative_from(std::string_view source)
            {
                if (source.size() == 2) {
                    switch (source[0]) {
                        case 't':
                            vrelative = v_relative::top;
                            break;
                        case 'b':
                            vrelative = v_relative::bottom;
                            break;
                        case 'c':
                            vrelative = v_relative::center;
                            break;
                        default:
                            AD_WARNING("[semantic Legend relative]: invalid value \"{}\"", source);
                            break;
                    }
                    switch (source[1]) {
                        case 'l':
                            hrelative = h_relative::left;
                            break;
                        case 'r':
                            hrelative = h_relative::right;
                            break;
                        case 'c':
                            hrelative = h_relative::center;
                            break;
                        default:
                            AD_WARNING("[semantic Legend relative]: invalid value \"{}\"", source);
                            break;
                    }
                }
                else
                    AD_WARNING("[semantic Legend relative]: invalid value \"{}\"", source);
            }
    };

    inline bool is_default(const Legend& legend) { return legend == Legend{}; }

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
        std::optional<ae::draw::v2::Viewport> viewport{};
        std::vector<StyleModifier> modifiers{};
        Legend legend{};
        Title plot_title{};

        Style() = default;
        Style(std::string_view a_name) : name{a_name} {}
    };

    class Styles
    {
      public:
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
