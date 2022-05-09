#include "chart/v3/styles.hh"

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
    if (value != "normal"sv || value != "italic"sv)
        throw std::runtime_error{fmt::format("invalid sematic text font slant \"{}\"", value)};
    font_slant = value;

} // ae::chart::v3::semantic::text_t::set_font_salnt

// ----------------------------------------------------------------------
