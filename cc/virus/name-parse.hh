#include <string>

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
    };

    enum class parse_tracing { no, yes };

    Parts parse(std::string_view source, parse_tracing tracing = parse_tracing::no);
}

// ======================================================================
