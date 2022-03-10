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
    if (auto prjs = projections(); !prjs.empty() && aProjectionNo.has_value() && *aProjectionNo < prjs.size()) {
    //     auto prj = (*prjs)[aProjectionNo ? *aProjectionNo : 0];
    //     fmt::format_to(std::back_inserter(name), " {}", prj->minimum_column_basis().format(">={}", MinimumColumnBasis::use_none::no));
    //     if (const auto stress = prj->stress(); !std::isnan(stress))
    //         fmt::format_to(std::back_inserter(name), " {:.4f}", stress);
    }
    return fmt::to_string(name);

} // ae::chart::v3::Chart::name

// ----------------------------------------------------------------------
