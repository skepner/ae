#include "chart/v3/styles.hh"
#include "chart/v3/legacy-plot-spec.hh"

// ----------------------------------------------------------------------

const ae::chart::v3::semantic::Style* ae::chart::v3::semantic::Styles::find_if_exists(std::string_view name) const
{
    if (const auto found = std::find_if(begin(), end(), [name](const auto& style) { return style.name == name; }); found != end())
        return &*found;
    else
        return nullptr;

} // ae::chart::v3::semantic::Styles::find_if_exists

// ----------------------------------------------------------------------

ae::chart::v3::semantic::Style& ae::chart::v3::semantic::Styles::find(std::string_view name)
{
    if (const auto found = std::find_if(begin(), end(), [name](const auto& style) { return style.name == name; }); found != end())
        return *found;

    return styles_.emplace_back(name);

} // ae::chart::v3::Styles::find

// ----------------------------------------------------------------------

void ae::chart::v3::semantic::box_t::set_origin(std::string_view value)
{
    using namespace std::string_view_literals;
    if (value.size() != 2 || "TtBbc"sv.find(value[0]) == std::string_view::npos || "LlRrc"sv.find(value[1]) == std::string_view::npos)
        throw std::runtime_error{fmt::format("invalid sematic box origin \"{}\"", value)};
    origin = std::string{value};

} // ae::chart::v3::semantic::box_t::set_origin

// ----------------------------------------------------------------------

void ae::chart::v3::semantic::text_t::set_font_face(std::string_view value)
{
    using namespace std::string_view_literals;
    if (value != "monospace"sv && value != "sansserif"sv && value != "serif"sv && value != "helvetica"sv && value != "courier"sv && value != "times"sv)
        throw std::runtime_error{fmt::format("invalid sematic text font face \"{}\"", value)};
    font_face = value;

} // ae::chart::v3::semantic::text_t::set_font_face

// ----------------------------------------------------------------------

void ae::chart::v3::semantic::text_t::set_font_weight(std::string_view value)
{
    using namespace std::string_view_literals;
    if (value != "normal"sv && value != "bold"sv)
        throw std::runtime_error{fmt::format("invalid sematic text font weight \"{}\"", value)};
    font_weight = value;

} // ae::chart::v3::semantic::text_t::set_font_weight

// ----------------------------------------------------------------------

void ae::chart::v3::semantic::text_t::set_font_slant(std::string_view value)
{
    using namespace std::string_view_literals;
    if (value != "normal"sv && value != "italic"sv)
        throw std::runtime_error{fmt::format("invalid sematic text font slant \"{}\"", value)};
    font_slant = value;

} // ae::chart::v3::semantic::text_t::set_font_salnt

// ----------------------------------------------------------------------

void ae::chart::v3::semantic::Style::export_to(ae::chart::v3::legacy::PlotSpec& plot_spec) const
{
    plot_spec.styles().clear();
    auto& basic_style = plot_spec.styles().emplace_back(PointStyle{});
    basic_style.fill(Color{"pink"});
    basic_style.outline(Color{"black"});
    basic_style.size(2.0);
    basic_style.shape(point_shape{point_shape::Shape::Triangle});
    basic_style.shown(true);
    ranges::fill(plot_spec.style_for_point(), 0ul);
    plot_spec.drawing_order().get().clear();
    ranges::copy(ranges::views::iota(point_index{0}, point_index{plot_spec.style_for_point().size()}), std::back_inserter(plot_spec.drawing_order().get()));

    // drawing order is not touched

} // ae::chart::v3::semantic::Style::export_to

// ----------------------------------------------------------------------

void ae::chart::v3::semantic::Styles::find_and_export_to(std::string_view name, legacy::PlotSpec& plot_spec) const
{
    if (const auto* found = find_if_exists(name); found)
        found->export_to(plot_spec);

} // ae::chart::v3::semantic::Styles::find_and_export_to

// ----------------------------------------------------------------------
