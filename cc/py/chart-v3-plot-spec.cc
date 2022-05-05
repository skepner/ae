#include "py/module.hh"
#include "chart/v3/chart.hh"
#include "chart/v3/styles.hh"

// ----------------------------------------------------------------------

namespace ae::py
{
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
            "x"_a, "y"_a, "width"_a, "height"_a = -1.0)                                                       //
        .def_readonly("legend", &semantic::Style::legend, pybind11::return_value_policy::reference_internal) //
        .def_readonly("plot_title", &semantic::Style::plot_title, pybind11::return_value_policy::reference_internal)   //
        ;

    pybind11::class_<semantic::StyleModifier>(chart_v3_submodule, "SemanticStyleModifier") //
        ;

    pybind11::class_<semantic::Legend>(chart_v3_submodule, "SemanticLegend") //
        ;

    pybind11::class_<semantic::Title>(chart_v3_submodule, "SemanticTitle") //
        ;
}

// ----------------------------------------------------------------------
