#pragma once

#include <bitset>
#include <optional>

#include "ext/fmt.hh"
#include "ext/from_chars.hh"
#include "virus/type-subtype.hh"
#include "virus/name.hh"

// ======================================================================

namespace ae
{
    class Messages;
    struct MessageLocation;
}

namespace ae::virus::name::inline v1
{
    struct Parts
    {
        enum class issue { unrecognized_location, invalid_year, invalid_subtype, size_ };

        struct issues_t : public std::bitset<static_cast<size_t>(issue::size_)>
        {
            void add(issue iss) { set(static_cast<size_t>(iss)); }
        };

        issues_t issues{};
        std::string subtype{};
        std::string host{};
        std::string location{};
        std::string isolation{};
        std::string year{};
        std::string reassortant{};
        std::string extra{};
        std::string continent{};
        std::string country{};

        enum class mark_extra { no, yes };

        std::string name(mark_extra me = mark_extra::no) const;
        std::string host_location_isolation_year() const;
        bool good() const noexcept { return !location.empty() && !isolation.empty() && year.size() == 4; }
        bool good_but_no_country() const noexcept { return good() && country.empty(); }

        bool operator==(const Parts& rhs) const
        {
            return subtype == rhs.subtype && host == rhs.host && location == rhs.location && isolation == rhs.isolation && year == rhs.year && reassortant == rhs.reassortant && extra == rhs.extra;
        }
    };

    class parse_settings
    {
      public:
        enum class tracing { no, yes };
        enum class report { no, yes };

        parse_settings(tracing a_trace = tracing::no, report a_report = report::no, std::string_view type_subtype_hint = {})
            : tracing_{a_trace}, report_{a_report}, type_subtype_hint_{type_subtype_hint}
        {
        }
        parse_settings(report a_report, std::string_view type_subtype_hint = {}) : report_{a_report}, type_subtype_hint_{type_subtype_hint} {}
        parse_settings(std::string_view type_subtype_hint) : type_subtype_hint_{type_subtype_hint} {}

        constexpr bool trace() const { return tracing_ == tracing::yes; }
        constexpr report report_errors() const { return report_; }
        const virus::type_subtype_t& type_subtype_hint() const { return type_subtype_hint_; }

      private:
        tracing tracing_{tracing::no};
        report report_{report::no};
        virus::type_subtype_t type_subtype_hint_;
    };

    Parts parse(std::string_view source, std::string_view year_hint, parse_settings& settings, Messages& messages, const MessageLocation& location);
    Parts parse(std::string_view source);

    inline bool is_good(std::string_view source) { return parse(source).good(); }

    inline std::string location(std::string_view name) { return parse(name).location; }
    inline std::string isolation(std::string_view name) { return parse(name).isolation; }
    inline std::string without_subtype(const Name& name) { return parse(name).host_location_isolation_year(); }
    inline std::string year(const Name& name) { return parse(name).year; }

    inline std::optional<size_t> year_as_number(const Name& name)
    {
        if (const auto parts = parse(name); !parts.year.empty())
            return ae::from_chars<size_t>(parts.year);
        else
            return std::nullopt;
    }

} // namespace ae::virus::name::inline v1

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::virus::name::Parts> : fmt::formatter<ae::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const ae::virus::name::Parts& parts, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "{}", parts.name(ae::virus::name::Parts::mark_extra::yes));
    }
};


// ======================================================================
