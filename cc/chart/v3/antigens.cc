#include <regex>

#include "utils/log.hh"
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

void ae::chart::v3::AntigenSerum::update_with(const AntigenSerum& src)
{
    if (lineage_.empty())
        lineage_ = src.lineage();
    else if (!src.lineage().empty() && lineage_ != src.lineage())
        AD_WARNING("merged antigen lineages {} vs. {}", lineage_, src.lineage());

} // ae::chart::v3::AntigenSerum::update_with

// ----------------------------------------------------------------------

void ae::chart::v3::Antigen::update_with(const Antigen& src)
{
    AntigenSerum::update_with(src);
    if (date_.empty())
        date_ = src.date();
    else if (!src.date().empty() && date_ != src.date())
        AD_WARNING("merged antigen dates {} vs. {}", date_, src.date());

    lab_ids_.insert_if_not_present(src.lab_ids());

} // ae::chart::v3::Antigen::update_with

// ----------------------------------------------------------------------

void ae::chart::v3::Serum::update_with(const Serum& src)
{
    AntigenSerum::update_with(src);

    if (serum_species_.empty())
        serum_species_ = src.serum_species();
    else if (!src.serum_species().empty() && serum_species_ != src.serum_species())
        AD_WARNING("merged serum serum_speciess {} vs. {}", serum_species_, src.serum_species());

} // ae::chart::v3::Serum::update_with

// ----------------------------------------------------------------------

ae::antigen_indexes ae::chart::v3::Antigens::homologous(const Serum& serum) const
{
    const auto match_passage = [&serum](virus::Passage antigen_passage, virus::Passage serum_passage) -> bool {
        if (serum_passage.empty()) // NIID has passage type data in serum_id
            return antigen_passage.is_egg() == (serum.serum_id().find("EGG") != std::string::npos);
        else
            return antigen_passage.is_egg() == serum_passage.is_egg();
    };

    antigen_indexes homologous_canditates;

    const auto check_antigens = [&homologous_canditates, this, &serum, match_passage](bool respect_annotations) -> bool {
        bool matched = false;
        for (const auto ag_no : size()) {
            const auto& antigen = operator[](ag_no);
            if (antigen.name() == serum.name() && antigen.reassortant() == serum.reassortant() && (!respect_annotations || antigen.annotations() == serum.annotations()) && match_passage(antigen.passage(), serum.passage())) {
                homologous_canditates.insert_if_not_present(ag_no);
                matched = true;
            }
        }
        return matched;
    };

    check_antigens(true) || check_antigens(false);

    return homologous_canditates;

} // ae::chart::v3::Antigens::homologous

// ----------------------------------------------------------------------

std::pair<std::string_view, std::string_view> ae::chart::v3::Antigens::date_range(bool test_only, const indexes_t& reference_indexes) const
{
    std::string_view first{}, last{};
    for (const auto ag_no : size()) {
        if (!test_only || !reference_indexes.contains(ag_no)) {
            if (const auto& antigen = operator[](ag_no); !antigen.date().empty()) {
                if (first.empty() || first > *antigen.date())
                    first = *antigen.date();
                if (last.empty() || last < *antigen.date())
                    last = *antigen.date();
            }
        }
    }
    return {first, last};

} // ae::chart::v3::Antigens::date_range

// ----------------------------------------------------------------------
