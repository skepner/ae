#pragma once

// helper functions for accessing rjson::v3 values

#include "ad/color-modifier.hh"
#include "ad/rjson-v3.hh"
#include "draw/v1/size-scale.hh"
#include "draw/v1/point-coordinates.hh"

// ----------------------------------------------------------------------

namespace rjson::v3
{
    template <typename Target> std::optional<Target> read_number(const rjson::v3::value& source)
    {
        return source.visit([]<typename Val>(const Val& value) -> std::optional<Target> {
            if constexpr (std::is_same_v<Val, rjson::v3::detail::number>)
                return value.template to<Target>();
            else if constexpr (std::is_same_v<Val, rjson::v3::detail::null>)
                return std::nullopt;
            else
                throw error{fmt::format("unrecognized: {}", value)};
        });

    } // read_number

    extern template std::optional<size_t  > read_number<size_t  >(const rjson::v3::value&);
    extern template std::optional<double  > read_number<double  >(const rjson::v3::value&);
    extern template std::optional<ae::draw::v1::Pixels  > read_number<ae::draw::v1::Pixels  >(const rjson::v3::value&);
    extern template std::optional<ae::draw::v1::Scaled  > read_number<ae::draw::v1::Scaled  >(const rjson::v3::value&);
    extern template std::optional<ae::draw::v1::Rotation> read_number<ae::draw::v1::Rotation>(const rjson::v3::value&);
    extern template std::optional<ae::draw::v1::Aspect  > read_number<ae::draw::v1::Aspect  >(const rjson::v3::value&);

    // ----------------------------------------------------------------------

    template <typename Target> Target read_number(const rjson::v3::value& source, Target dflt)
    {
        return source.visit([&dflt]<typename Val>(const Val& value) -> Target {
            if constexpr (std::is_same_v<Val, rjson::v3::detail::number>)
                return value.template to<Target>();
            else if constexpr (std::is_same_v<Val, rjson::v3::detail::null>)
                return std::forward<Target>(dflt);
            else
                throw error{fmt::format("unrecognized: {}", value)};
        });

    } // read_number

    extern template size_t   read_number<size_t>(const rjson::v3::value&, size_t);
    extern template double   read_number<double>(const rjson::v3::value&, double);
    extern template ae::draw::v1::Pixels   read_number<ae::draw::v1::Pixels>(const rjson::v3::value&, Pixels);
    extern template ae::draw::v1::Scaled   read_number<ae::draw::v1::Scaled>(const rjson::v3::value&, Scaled);
    extern template ae::draw::v1::Aspect   read_number<ae::draw::v1::Aspect>(const rjson::v3::value&, Aspect);

    template <> inline ae::draw::v1::Rotation read_number<ae::draw::v1::Rotation>(const rjson::v3::value& source, ae::draw::v1::Rotation dflt)
    {
        if (const auto angle = read_number<double>(source); angle.has_value())
            return RotationRadiansOrDegrees(*angle);
        else
            return dflt;
    }

    // ----------------------------------------------------------------------

    std::optional<acmacs::color::Modifier> read_color(const rjson::v3::value& source);
    acmacs::color::Modifier read_color(const rjson::v3::value& source, const acmacs::color::Modifier& dflt);
    acmacs::color::Modifier read_color(const rjson::v3::value& source, Color dflt);
    acmacs::color::Modifier read_color_or_empty(const rjson::v3::value& source);
    std::optional<std::string_view> read_string(const rjson::v3::value& source);
    std::string_view read_string(const rjson::v3::value& source, std::string_view dflt);

    bool read_bool(const rjson::v3::value& source, bool dflt);

    std::optional<acmacs::PointCoordinates> read_point_coordinates(const rjson::v3::value& source);
    acmacs::PointCoordinates read_point_coordinates(const rjson::v3::value& source, const acmacs::PointCoordinates& dflt);

    // ----------------------------------------------------------------------

    template <typename Target, typename Callback> void call_if_not_null(const rjson::v3::value& source, Callback callback)
    {
        source.visit([&]<typename Val>(const Val& value) -> void {
            if constexpr (std::is_same_v<Val, rjson::v3::detail::boolean> && std::is_same_v<Target, bool>)
                callback(value.template to<Target>());
            else if constexpr (std::is_same_v<Val, rjson::v3::detail::number> && (std::is_same_v<Target, double> || std::is_same_v<Target, Pixels>))
                callback(value.template to<Target>());
            else if constexpr (std::is_same_v<Val, rjson::v3::detail::string> && (std::is_same_v<Target, std::string_view> || std::is_same_v<Target, std::string>))
                callback(value.template to<Target>());
            else if constexpr (!std::is_same_v<Val, rjson::v3::detail::null>) {
                if constexpr (std::is_same_v<Target, acmacs::color::Modifier>) {
                    if (const auto color = read_color(source); color.has_value())
                        callback(*color);
                }
                else if constexpr (std::is_same_v<Target, acmacs::PointCoordinates>) {
                    if (const auto coord = read_point_coordinates(source); coord.has_value())
                        callback(*coord);
                }
                else
                    throw error{fmt::format("rjson::v3::call_if_not_null: cannot call with <> as argument, value: {}: {}", typeid(Target).name(), value)};
            }
        });
    }

} // namespace rjson::v3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
