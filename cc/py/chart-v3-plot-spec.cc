#include "py/module.hh"
#include "chart/v3/chart.hh"
#include "chart/v3/styles.hh"

// ----------------------------------------------------------------------

namespace ae::py
{
    // static inline std::vector<double> get(const ae::draw::v2::offset_t& offset)
    // {
    //     return std::vector<double>{offset.x, offset.y};
    // }

    // static inline void put(ae::draw::v2::offset_t& offset, const std::vector<double>& src)
    // {
    //     if (src.size() != 2)
    //         throw std::runtime_error{"invalid offset"};
    //     offset.x = src[0];
    //     offset.y = src[1];
    // }

    // static inline std::string get(const ae::draw::v2::font_slant_t& slant)
    // {
    //     return fmt::format("{}", slant);
    // }

    // static inline void put(ae::draw::v2::font_slant_t& slant, std::string_view src)
    // {
    //     slant = ae::draw::v2::font_slant_t{src};
    // }

    // static inline std::string get(const ae::draw::v2::font_weight_t& weight)
    // {
    //     return fmt::format("{}", weight);
    // }

    // static inline void put(ae::draw::v2::font_weight_t& weight, std::string_view src)
    // {
    //     weight = ae::draw::v2::font_weight_t{src};
    // }

    // ----------------------------------------------------------------------

    static inline ae::chart::v3::semantic::StyleModifier& add_modifier(ae::chart::v3::semantic::Style& style, const pybind11::kwargs& kwargs)
    {
        auto& modifier = style.modifiers.emplace_back();
        for (const auto [keyword_handle, value] : kwargs) {
            if (!value.is_none()) {
                const auto keyword = keyword_handle.cast<std::string_view>();
                if (keyword == "parent")
                    modifier.parent.assign(value.cast<std::string_view>());
                else if (keyword == "selector") {
                    const auto vals = value.cast<pybind11::list>();
                    modifier.selector = ae::chart::v3::semantic::Selector{.attribute = vals[0].cast<std::string>(), .value = vals[1].cast<std::string>()};
                }
                else if (keyword == "hide")
                    modifier.point_style.shown(!value.cast<bool>());
                else if (keyword == "fill")
                    modifier.point_style.fill(value.cast<std::string_view>());
                else if (keyword == "outline")
                    modifier.point_style.outline(value.cast<std::string_view>());
                else if (keyword == "outline_width")
                    modifier.point_style.outline_width(value.cast<double>());
                else if (keyword == "size")
                    modifier.point_style.size(value.cast<double>());
                else if (keyword == "rotation")
                    modifier.point_style.rotation(ae::chart::v3::Rotation{value.cast<double>()});
                else if (keyword == "aspect")
                    modifier.point_style.aspect(ae::chart::v3::Aspect{value.cast<double>()});
                else if (keyword == "shape")
                    modifier.point_style.shape(ae::chart::v3::point_shape{value.cast<std::string_view>()});
                else if ((keyword == "raise" || keyword == "rais" || keyword == "raise_") && value.cast<bool>())
                    modifier.order = ae::chart::v3::semantic::DrawingOrderModifier::raise;
                else if (keyword == "lower" && value.cast<bool>())
                    modifier.order = ae::chart::v3::semantic::DrawingOrderModifier::lower;
                else if (keyword == "only") {
                    switch (std::tolower(value.cast<std::string_view>()[0])) {
                        case 'a':
                            modifier.select_antigens_sera = ae::chart::v3::semantic::SelectAntigensSera::antigens_only;
                            break;
                        case 's':
                            modifier.select_antigens_sera = ae::chart::v3::semantic::SelectAntigensSera::sera_only;
                            break;
                    }
                }
                else if (keyword == "legend") {
                    modifier.legend.text.assign(value.cast<std::string_view>());
                }
                else if (keyword == "legend_priority") {
                    modifier.legend.priority = value.cast<int>();
                }
                else
                    throw std::runtime_error("Style.add_modifier: unrecognized arg"); // fmt::format("Style.add_modifier: unrecognized arg \"{}\"", keyword));
            }
        }
        return modifier;
    }

    static inline void remove_modifiers(ae::chart::v3::semantic::Style& style)
    {
        style.modifiers.clear();
    }

} // namespace ae::py

// ----------------------------------------------------------------------

void ae::py::chart_v3_plot_spec(pybind11::module_& chart_v3_submodule)
{
    using namespace pybind11::literals;
    using namespace ae::chart::v3;

    pybind11::class_<semantic::Styles>(chart_v3_submodule, "SemanticStyles")             //
        .def("__len__", &semantic::Styles::size)                                         //
        .def("__bool__", [](const semantic::Styles& styles) { return !styles.empty(); }) //
        .def(
            "__iter__", [](semantic::Styles& styles) { return pybind11::make_iterator(styles.begin(), styles.end()); }, pybind11::keep_alive<0, 1>())         //
        .def("__getitem__", &semantic::Styles::find, "name"_a, pybind11::return_value_policy::reference_internal, pybind11::doc("find or add style by name")) //
        ;

    pybind11::class_<semantic::Style>(chart_v3_submodule, "SemanticStyle") //
        .def_readwrite("title", &semantic::Style::title)
        .def_readwrite("priority", &semantic::Style::priority)
        .def("remove_modifiers", &ae::py::remove_modifiers) //
        .def(
            "add_modifier", &ae::py::add_modifier,
            pybind11::doc(
                R"(kwargs: parent="another-style", selector={"C": "clade"}, hide=False, fill="transparent", outline="black", outline_width=1.0, size=5.0, rotation=0.0, aspect=1.0, shape="C|B|E|U|T", rais=True, raise_=True, lower=True, only="a|s", legend="text", legend_priority=0)")) //
        .def(
            "viewport",
            [](semantic::Style& style, double x, double y, double width, double height) {
                if (height < 0.0)
                    height = width;
                style.viewport = ae::draw::v2::Viewport{x, y, width, height};
            },
            "x"_a, "y"_a, "width"_a, "height"_a = -1.0)                                                              //
        .def_readonly("legend", &semantic::Style::legend, pybind11::return_value_policy::reference_internal)         //
        .def_readonly("plot_title", &semantic::Style::plot_title, pybind11::return_value_policy::reference_internal) //
        ;

    pybind11::class_<semantic::StyleModifier>(chart_v3_submodule, "SemanticStyleModifier") //
        ;

    pybind11::class_<semantic::Title>(chart_v3_submodule, "SemanticTitle") //
        .def_property(
            "shown", [](const semantic::Title& title) { return title.shown.has_value() ? *title.shown : false; }, [](semantic::Title& title, bool shown) { title.shown = shown; }) //
        .def_property_readonly(
            "box",
            [](semantic::Title& title) -> semantic::box_t& {
                if (!title.box.has_value())
                    title.box = semantic::box_t{};
                return *title.box;
            },
            pybind11::return_value_policy::reference_internal) //
        .def_readwrite("text", &semantic::Title::text)         //
        ;

    pybind11::class_<semantic::Legend>(chart_v3_submodule, "SemanticLegend") //
        .def_property(
            "shown", [](const semantic::Legend& legend) { return legend.shown.has_value() ? *legend.shown : false; }, [](semantic::Legend& legend, bool shown) { legend.shown = shown; }) //
        .def_property(
            "show_rows_with_zero_count", [](const semantic::Legend& legend) { return legend.show_rows_with_zero_count.has_value() ? *legend.show_rows_with_zero_count : false; }, [](semantic::Legend& legend, bool show_rows_with_zero_count) { legend.show_rows_with_zero_count = show_rows_with_zero_count; }) //
        .def_property(
            "point_size", [](const semantic::Legend& legend) { return legend.point_size.has_value() ? *legend.point_size : 10.0; }, [](semantic::Legend& legend, double point_size) { legend.point_size = point_size; }) //
        .def_property_readonly(
            "box",
            [](semantic::Legend& legend) -> semantic::box_t& {
                if (!legend.box.has_value())
                    legend.box = semantic::box_t{};
                return *legend.box;
            },
            pybind11::return_value_policy::reference_internal) //
        .def_property_readonly(
            "title",
            [](semantic::Legend& legend) -> semantic::text_t& {
                if (!legend.title.has_value())
                    legend.title = semantic::text_t{};
                return *legend.title;
            },
            pybind11::return_value_policy::reference_internal) //
        ;

    pybind11::class_<semantic::box_t>(chart_v3_submodule, "SemanticBox") //
        .def_property(
            "origin", [](const semantic::box_t& box) { return box.origin.has_value() ? *box.origin : std::string{"tl"}; },
            [](semantic::box_t& box, std::string_view origin) { box.set_origin(origin); }) //
        .def_property(
            "padding", [](const semantic::box_t& /*box*/) { return pybind11::none{}; }, [](semantic::box_t& box, double value) { box.set_padding(value); }) //
        .def(
            "offset",
            [](semantic::box_t& box, double x, double y) {
                box.offset = semantic::offset_t{x, y};
            },
            "x"_a, "y"_a) //
        .def_property(
            "border_color", [](const semantic::box_t& box) { return box.border_color.has_value() ? **box.border_color : std::string{"transparent"}; },
            [](semantic::box_t& box, std::string_view border_color) { box.border_color = semantic::color_t{border_color}; }) //
        .def_property(
            "border_width", [](const semantic::box_t& box) { return box.border_width.has_value() ? *box.border_width : 0.0; },
            [](semantic::box_t& box, double border_width) { box.border_width = border_width; }) //
        .def_property(
            "background_color", [](const semantic::box_t& box) { return box.background_color.has_value() ? **box.background_color : std::string{"transparent"}; },
            [](semantic::box_t& box, std::string_view background_color) { box.background_color = semantic::color_t{background_color}; }) //
        ;

    pybind11::class_<semantic::text_t>(chart_v3_submodule, "SemanticText") //
        .def_property(
            "text", [](const semantic::text_t& text) { return text.text.has_value() ? *text.text : std::string{}; },
            [](semantic::text_t& text, std::string_view value) { text.text = std::string{value}; }) //
        .def_property(
            "font_face", [](const semantic::text_t& text) { return text.font_face.has_value() ? *text.font_face : std::string{}; },
            [](semantic::text_t& text, std::string_view font_face) { text.set_font_face(font_face); }) //
        .def_property(
            "font_weight", [](const semantic::text_t& text) { return text.font_weight.has_value() ? *text.font_weight : std::string{}; },
            [](semantic::text_t& text, std::string_view font_weight) { text.set_font_weight(font_weight); }) //
        .def_property(
            "font_slant", [](const semantic::text_t& text) { return text.font_slant.has_value() ? *text.font_slant : std::string{}; },
            [](semantic::text_t& text, std::string_view font_slant) { text.set_font_slant(font_slant); }) //
        .def_property(
            "font_size", [](const semantic::text_t& text) { return text.font_size.has_value() ? *text.font_size : 16.0; },
            [](semantic::text_t& text, double font_size) { text.font_size = font_size; }) //
        .def_property(
            "color", [](const semantic::text_t& text) { return text.color.has_value() ? **text.color : std::string{"black"}; },
            [](semantic::text_t& text, std::string_view color) { text.color = semantic::color_t{color}; }) //
        .def_property(
            "interline", [](const semantic::text_t& text) { return text.interline.has_value() ? *text.interline : 0.2; },
            [](semantic::text_t& text, double interline) { text.interline = interline; }) //
        ;
}

// ----------------------------------------------------------------------
