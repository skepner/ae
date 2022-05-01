#include "py/module.hh"
#include "chart/v3/chart.hh"
#include "chart/v3/styles.hh"

// ----------------------------------------------------------------------

namespace ae::py
{
    static inline ae::chart::v3::StyleModifier& add_modifier(ae::chart::v3::Style& style, const pybind11::kwargs& kwargs)
    {
        auto& modifier = style.modifiers.emplace_back();
        for (const auto [keyword_handle, value] : kwargs) {
            const auto keyword = keyword_handle.cast<std::string_view>();
            if (keyword == "parent")
                modifier.parent.assign(value.cast<std::string_view>());
            else if (keyword == "selector") {
                const auto vals = value.cast<pybind11::list>();
                modifier.semantic_selector = ae::chart::v3::SematicSelector{.attribute = vals[0].cast<std::string>(), .value = vals[1].cast<std::string>()};
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
            else
                throw std::runtime_error("Style.add_modifier: unrecognized arg"); // fmt::format("Style.add_modifier: unrecognized arg \"{}\"", keyword));
        }
        return modifier;
    }
} // namespace ae::py

// ----------------------------------------------------------------------

void ae::py::chart_v3_plot_spec(pybind11::module_& chart_v3_submodule)
{
    using namespace pybind11::literals;
    using namespace ae::chart::v3;

    pybind11::class_<Styles>(chart_v3_submodule, "Styles")                     //
        .def("__len__", &Styles::size)                                         //
        .def("__bool__", [](const Styles& styles) { return !styles.empty(); }) //
        .def(
            "__iter__", [](Styles& styles) { return pybind11::make_iterator(styles.begin(), styles.end()); }, pybind11::keep_alive<0, 1>())         //
        .def("__getitem__", &Styles::find, "name"_a, pybind11::return_value_policy::reference_internal, pybind11::doc("find or add style by name")) //
        ;

    pybind11::class_<Style>(chart_v3_submodule, "Style") //
        .def_readwrite("title", &Style::title)
        .def_readwrite("priority", &Style::priority)
        .def("add_modifier", &ae::py::add_modifier,
             // pybind11::kw_only(), */ "parent"_a, "selector"_a, "hide"_a, "fill"_a, "outline"_a, "outline_width"_a, "size"_a, "rotation"_a, "aspect"_a, "shape"_a
             pybind11::doc("kwargs: parent, selector, hide, fill, outline, outline_width, size, rotation, aspect, shape")
             ) //
        ;

    pybind11::class_<StyleModifier>(chart_v3_submodule, "StyleModifier") //
        ;
}

// ----------------------------------------------------------------------
