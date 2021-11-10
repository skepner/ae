#pragma once

#include <bitset>

#include "ext/fmt.hh"

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
        enum class issue {
            unrecognized_location,
            invalid_year,
            invalid_subtype,
            size_
        };

        struct issues_t : public std::bitset<static_cast<size_t>(issue::size_)>
        {
            void add(issue iss) { set(static_cast<size_t>(iss)); }
        };

        issues_t issues {};
        std::string subtype{};
        std::string host{};
        std::string location{};
        std::string isolation{};
        std::string year{};
        std::string reassortant{};
        std::string extra{};

        enum class mark_extra { no, yes };

        std::string name(mark_extra me = mark_extra::no) const;
        std::string host_location_isolation_year() const;
        bool operator==(const Parts&) const = default;
    };

    class parse_settings
    {
      public:
        enum class tracing { no, yes };

        parse_settings(tracing a_trace = tracing::no) : tracing_{a_trace} {}

        constexpr bool trace() const { return tracing_ == tracing::yes; }

      private:
        tracing tracing_{tracing::no};
    };

    // context is e.g. file:line referring to source fasta file
    Parts parse(std::string_view source, parse_settings& settings, Messages& messages, const MessageLocation& location);
}

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::virus::name::Parts> : fmt::formatter<eu::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const ae::virus::name::Parts& parts, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "{}", parts.name(ae::virus::name::Parts::mark_extra::yes));
    }
};


// ======================================================================
