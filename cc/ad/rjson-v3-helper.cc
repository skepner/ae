#include "ad/rjson-v3-helper.hh"

// ----------------------------------------------------------------------

template std::optional<size_t  > rjson::v3::read_number<size_t  >(const rjson::v3::value&);
template std::optional<double  > rjson::v3::read_number<double  >(const rjson::v3::value&);
template std::optional<ae::draw::v1::Pixels  > rjson::v3::read_number<ae::draw::v1::Pixels  >(const rjson::v3::value&);
template std::optional<ae::draw::v1::Scaled  > rjson::v3::read_number<ae::draw::v1::Scaled  >(const rjson::v3::value&);
template std::optional<ae::draw::v1::Rotation> rjson::v3::read_number<ae::draw::v1::Rotation>(const rjson::v3::value&);
template std::optional<ae::draw::v1::Aspect  > rjson::v3::read_number<ae::draw::v1::Aspect  >(const rjson::v3::value&);

template size_t rjson::v3::read_number<size_t>(const rjson::v3::value&, size_t);
template double rjson::v3::read_number<double>(const rjson::v3::value&, double);
template ae::draw::v1::Pixels rjson::v3::read_number<ae::draw::v1::Pixels>(const rjson::v3::value&, ae::draw::v1::Pixels);
template ae::draw::v1::Scaled rjson::v3::read_number<ae::draw::v1::Scaled>(const rjson::v3::value&, ae::draw::v1::Scaled);
template ae::draw::v1::Aspect rjson::v3::read_number<ae::draw::v1::Aspect>(const rjson::v3::value&, ae::draw::v1::Aspect);

// ----------------------------------------------------------------------

std::optional<acmacs::color::Modifier> rjson::v3::read_color(const rjson::v3::value& source)
{
    return source.visit([]<typename Val>(const Val& value) -> std::optional<acmacs::color::Modifier> {
        if constexpr (std::is_same_v<Val, rjson::v3::detail::string>)
            return acmacs::color::Modifier{value.template to<std::string_view>()};
        else if constexpr (std::is_same_v<Val, rjson::v3::detail::null>)
            return std::nullopt;
        else
            throw error{fmt::format("unrecognized: {} (expected color)", value)};
    });

} // rjson::v3::read_color

// ----------------------------------------------------------------------

acmacs::color::Modifier rjson::v3::read_color_or_empty(const rjson::v3::value& source)
{
    if (auto color = read_color(source); color.has_value())
        return *color;
    else
        return {};

} // rjson::v3::read_color_or_empty

// ----------------------------------------------------------------------

acmacs::color::Modifier rjson::v3::read_color(const rjson::v3::value& source, const acmacs::color::Modifier& dflt)
{
    if (auto color = read_color(source); color.has_value())
        return *color;
    else
        return dflt;

} // rjson::v3::read_color

// ----------------------------------------------------------------------

acmacs::color::Modifier rjson::v3::read_color(const rjson::v3::value& source, Color dflt)
{
    if (auto color = read_color(source); color.has_value())
        return *color;
    else
        return acmacs::color::Modifier{dflt};

} // rjson::v3::read_color

// ----------------------------------------------------------------------

std::optional<std::string_view> rjson::v3::read_string(const rjson::v3::value& source)
{
    return source.visit([]<typename Val>(const Val& value) -> std::optional<std::string_view> {
        if constexpr (std::is_same_v<Val, rjson::v3::detail::string>)
            return value.template to<std::string_view>();
        else if constexpr (std::is_same_v<Val, rjson::v3::detail::null>)
            return std::nullopt;
        else
            throw error{fmt::format("unrecognized: {} (expected string)", value)};
    });

} // rjson::v3::read_string

// ----------------------------------------------------------------------

std::string_view rjson::v3::read_string(const rjson::v3::value& source, std::string_view dflt)
{
    if (auto str = read_string(source); str.has_value())
        return *str;
    else
        return dflt;

} // rjson::v3::read_string

// ----------------------------------------------------------------------

bool rjson::v3::read_bool(const rjson::v3::value& source, bool dflt)
{
    return source.visit([dflt]<typename Val>(const Val& value) -> bool {
        if constexpr (std::is_same_v<Val, rjson::v3::detail::null>)
            return dflt;
        else if constexpr (std::is_same_v<Val, rjson::v3::detail::boolean>)
            return value.template to<bool>();
        else
            throw error{fmt::format("unrecognized {} (expected boolean)", value)};
    });

} // rjson::v3::read_bool

// ----------------------------------------------------------------------

std::optional<ae::draw::v1::PointCoordinates> rjson::v3::read_point_coordinates(const rjson::v3::value& source)
{
    return source.visit([]<typename Val>(const Val& point_coordinates) -> std::optional<ae::draw::v1::PointCoordinates> {
        if constexpr (std::is_same_v<Val, rjson::v3::detail::array>) {
            if (point_coordinates.size() == 2)
                return ae::draw::v1::PointCoordinates{point_coordinates[0].template to<double>(), point_coordinates[1].template to<double>()};
            else
                throw error{fmt::format("unrecognized: {} (expected array of two numbers)", point_coordinates)};
        }
        else if constexpr (std::is_same_v<Val, rjson::v3::detail::null>)
            return std::nullopt;
        else
            throw error{fmt::format("unrecognized: {} (expected array of two numbers)", point_coordinates)};
    });

} // rjson::v3::read_point_coordinates

// ----------------------------------------------------------------------

ae::draw::v1::PointCoordinates rjson::v3::read_point_coordinates(const rjson::v3::value& source, const ae::draw::v1::PointCoordinates& dflt)
{
    if (auto pc = read_point_coordinates(source); pc.has_value())
        return *pc;
    else
        return dflt;

} // rjson::v3::read_point_coordinates

// ----------------------------------------------------------------------
