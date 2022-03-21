#include "utils/string.hh"
#include "utils/log.hh"
#include "chart/v3/info.hh"

// ----------------------------------------------------------------------

std::string ae::chart::v3::Assay::name(assay_name_t an) const
{
    switch (an) {
        case assay_name_t::full:
            return get();
        case assay_name_t::brief:
            return short_name();
        case assay_name_t::hi_or_neut:
            return hi_or_neut(no_hi::no);
        case assay_name_t::HI_or_Neut:
            return HI_or_Neut(no_hi::no);
        case assay_name_t::no_hi:
            return hi_or_neut(no_hi::yes);
        case assay_name_t::no_HI:
            return HI_or_Neut(no_hi::yes);
    }
    return get();

} // ae::chart::v3::Assay::name

// ----------------------------------------------------------------------

std::string ae::chart::v3::Assay::hi_or_neut(no_hi nh) const
{
    if (empty() || get() == "HI") {
        if (nh == no_hi::no)
            return "hi";
        else
            return "";
    }
    else if (get() == "HINT")
        return "hint";
    else
        return "neut";
}

// ----------------------------------------------------------------------

std::string ae::chart::v3::Assay::HI_or_Neut(no_hi nh) const
{
    if (empty() || get() == "HI") {
        if (nh == no_hi::no)
            return "HI";
        else
            return "";
    }
    else if (get() == "HINT")
        return "HINT";
    else
        return "Neut";
}

// ----------------------------------------------------------------------

std::string ae::chart::v3::Assay::short_name() const
{
    if (get() == "FOCUS REDUCTION")
        return "FRA";
    else
        return get();
}

// ----------------------------------------------------------------------

template <typename Field> static inline std::string info_make_field(const ae::chart::v3::Info& info, const Field& (ae::chart::v3::TableSource::*func)() const)
{
    if (const auto& field = std::invoke(func, info); !field.empty() || info.sources().empty()) {
        return std::string{field.get()};
    }

    std::vector<std::string> composition(info.sources().size());
    std::transform(info.sources().begin(), info.sources().end(), composition.begin(), [&func](const auto& source) { return std::string{std::invoke(func, source).get()}; });
    std::sort(composition.begin(), composition.end());
    composition.erase(std::unique(composition.begin(), composition.end()), composition.end());
    return ae::string::join("+", composition);
}

std::string ae::chart::v3::Info::make_virus_not_influenza() const
{
    const auto vir = ::info_make_field(*this, &TableSource::virus);
    if (vir == "INFLUENZA")
        return {};
    else
        return vir;

} // ae::chart::v3::Info::make_virus_not_influenza

// ----------------------------------------------------------------------

std::string ae::chart::v3::Info::make_virus_type() const
{
    return ::info_make_field(*this, &TableSource::type_subtype);

} // ae::chart::v3::Info::make_virus_type

// ----------------------------------------------------------------------

std::string ae::chart::v3::Info::make_virus_subtype() const
{
    const auto vt = make_virus_type();
    if (vt == "A(H1N1)")
        return "H1";
    else if (vt == "A(H3N2)")
        return "H3";
    else
        return vt;

} // ae::chart::v3::Info::make_virus_subtype

// ----------------------------------------------------------------------

std::string ae::chart::v3::Info::make_assay(Assay::assay_name_t tassay) const
{
    if (const auto ass = assay().name(tassay); !ass.empty())
        return ass;

    std::vector<std::string> composition(sources().size());
    std::transform(sources().begin(), sources().end(), composition.begin(), [tassay](const auto& source) { return source.assay().name(tassay); });
    std::sort(composition.begin(), composition.end());
    composition.erase(std::unique(composition.begin(), composition.end()), composition.end());
    return ae::string::join("+", composition);

} // ae::chart::v3::Info::make_assay

// ----------------------------------------------------------------------

std::string ae::chart::v3::Info::make_rbc_species() const
{
    return ::info_make_field(*this, &TableSource::rbc_species);

} // ae::chart::v3::Info::make_rbc_species

// ----------------------------------------------------------------------

std::string ae::chart::v3::Info::make_lab() const
{
    return ::info_make_field(*this, &TableSource::lab);

} // ae::chart::v3::Info::make_lab

// ----------------------------------------------------------------------

std::string ae::chart::v3::Info::make_date(include_number_of_tables inc) const
{
    if (!date().empty() || sources().empty())
        return std::string{date().get()};

    std::vector<std::string> composition(sources().size());
    std::transform(sources().begin(), sources().end(), composition.begin(), [](const auto& source) { return std::string{source.date().get()}; });
    std::sort(composition.begin(), composition.end());
    if (inc == include_number_of_tables::yes)
        return fmt::format("{}-{} ({} tables)", composition.front(), composition.back(), sources().size());
    else
        return fmt::format("{}-{}", composition.front(), composition.back());

} // ae::chart::v3::Info::make_date

// ----------------------------------------------------------------------

size_t ae::chart::v3::Info::max_source_name() const
{
    if (sources().size() < 2)
        return 0;
    size_t msn = 0;
    for (size_t s_no{0}; s_no < sources().size(); ++s_no)
        msn = std::max(msn, sources()[s_no].name_or_date().size());
    return msn;

} // ae::chart::v3::Info::max_source_name

// ----------------------------------------------------------------------
