#pragma once

#include <vector>
#include <string>

#include "chart/v3/point-style.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    struct StyleModifier
    {
        std::string parent{};
        std::pair<std::string, std::string> semantic_selector{};
        PointStyle point_style{};
    };

    struct Style
    {
        std::string name{};
        std::string title{};
        int priority{0};
        // viewport
        std::vector<StyleModifier> modifiers{};

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

        // find or add style by name
        Style& find(std::string_view name);

      private:
        std::vector<Style> styles_{};
    };
}

// ----------------------------------------------------------------------
