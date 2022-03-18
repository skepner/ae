#include <regex>

#include "chart/v3/antigens.hh"

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

// static const std::regex sAntigenAnnotationToIgnore{"^10-[1-9]$"}; // concentration (Crick H3 PRN)
static const std::regex sSerumAnnotationToIgnore{"(CONC|RDE@|BOOST|BLEED|LAIV|^CDC$)"};

#pragma GCC diagnostic pop

bool ae::chart::v3::Annotations::match(const Annotations& antigen, const Annotations& serum)
{
    std::vector<std::string_view> antigen_fixed(antigen->size());
    auto antigen_fixed_end = antigen_fixed.begin();
    for (const auto& anno : antigen) {
        // const std::string_view annos = static_cast<std::string_view>(anno);
        // if (!std::regex_search(std::begin(annos), std::end(annos), sAntigenAnnotationToIgnore))
        *antigen_fixed_end++ = anno;
    }
    antigen_fixed.erase(antigen_fixed_end, antigen_fixed.end());
    std::sort(antigen_fixed.begin(), antigen_fixed.end());

    std::vector<std::string_view> serum_fixed(serum->size());
    auto serum_fixed_end = serum_fixed.begin();
    for (const auto& anno : serum) {
        const std::string_view annos = static_cast<std::string_view>(anno);
        if (!std::regex_search(std::begin(annos), std::end(annos), sSerumAnnotationToIgnore))
            *serum_fixed_end++ = anno;
    }
    serum_fixed.erase(serum_fixed_end, serum_fixed.end());
    std::sort(serum_fixed.begin(), serum_fixed.end());

    return antigen_fixed == serum_fixed;

} // ae::chart::v3::Annotations::match_antigen_serum

// ----------------------------------------------------------------------
