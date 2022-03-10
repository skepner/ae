#include "chart/v3/chart.hh"

// ----------------------------------------------------------------------

std::string ae::chart::v3::Chart::name(std::optional<projection_index> aProjectionNo) const
{
    fmt::memory_buffer name;
    if (!info().name().empty()) {
        fmt::format_to(std::back_inserter(name), "{}", info().name());
    }
    else {
        fmt::format_to(std::back_inserter(name), "{}",
                       ae::string::join(" ", info().make_lab(), info().make_virus_not_influenza(), info().make_virus_type(), info().make_assay(Assay::assay_name_t::no_hi), info().make_rbc_species(),
                                        info().make_date()));
    }
    if (const auto& prjs = projections(); !prjs.empty() && aProjectionNo.has_value() && *aProjectionNo < prjs.size()) {
        const auto& prj = prjs[*aProjectionNo];
        fmt::format_to(std::back_inserter(name), "{}", prj.minimum_column_basis().format(" >={}", minimum_column_basis::use_none::no));
        if (const auto stress = prj.stress(); !std::isnan(stress))
            fmt::format_to(std::back_inserter(name), " {:.4f}", stress);
    }
    return fmt::to_string(name);

} // ae::chart::v3::Chart::name

// ----------------------------------------------------------------------

std::string ae::chart::v3::Chart::name_for_file() const
{
    std::string name;
    if (!info().name().empty())
        name = info().name();
    else
        name = ae::string::join("-", info().make_virus_not_influenza(), info().make_virus_subtype(), info().make_assay(Assay::assay_name_t::brief), info().make_rbc_species(), info().make_lab(), info().make_date(Info::include_number_of_tables::no));
    ae::string::replace_in_place(name, '/', '-');
    ae::string::replace_in_place(name, ' ', '-');
    ae::string::lowercase_in_place(name);
    return name;

} // ae::chart::v3::Chart::name_for_file

// ----------------------------------------------------------------------
