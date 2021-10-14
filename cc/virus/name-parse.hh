#include "ext/fmt.hh"

// ======================================================================

namespace ae::virus::name::inline v1
{
    struct Parts
    {
        std::string subtype{};
        std::string host{};
        std::string location{};
        std::string isolation{};
        std::string year{};
        std::string reassortant{};
        std::string extra{};

        enum class mark_extra { no, yes };

        std::string name(mark_extra me = mark_extra::no) const;
    };

    enum class parse_tracing { no, yes };

    Parts parse(std::string_view source, parse_tracing tracing = parse_tracing::no);
}

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::virus::name::Parts> : fmt::formatter<eu::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const ae::virus::name::Parts& parts, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "{}", parts.name(ae::virus::name::Parts::mark_extra::yes));
    }
};


// ======================================================================
