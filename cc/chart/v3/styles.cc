#include "chart/v3/styles.hh"

// ----------------------------------------------------------------------

ae::chart::v3::semantic::Style& ae::chart::v3::semantic::Styles::find(std::string_view name)
{
    if (const auto found = std::find_if(begin(), end(), [name](const auto& style) { return style.name == name; }); found != end())
        return *found;

    return styles_.emplace_back(name);

} // ae::chart::v3::Styles::find

// ----------------------------------------------------------------------
