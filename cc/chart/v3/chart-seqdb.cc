#include "chart/v3/chart-seqdb.hh"
#include "chart/v3/selected-antigens-sera.hh"
#include "sequences/clades.hh"
#include "sequences/seqdb.hh"
#include "sequences/seqdb-selected.hh"

// ======================================================================

std::pair<size_t, size_t> ae::chart::v3::populate_from_seqdb(Chart& chart)
{
    const auto& seqdb = ae::sequences::seqdb_for_subtype(chart.info().virus_subtype());
    const ae::sequences::Clades clades{ae::sequences::Clades::load_from_default_file};

    const auto update = [&clades](auto& ag_sr, auto& selected) {
        selected.find_masters();
        selected.find_clades(clades);
        ag_sr.aa(selected.at(0).aa());
        ag_sr.nuc(selected.at(0).nuc());
        ag_sr.semantic().clades = selected.at(0).clades;
        if (const auto& insertions = selected.at(0).seq->aa_insertions; !insertions.empty())
            ag_sr.aa_insertions(insertions);
    };

    const auto populate = [update, &seqdb](auto& ag_srs) -> size_t {
        size_t populated{0};
        for (auto& ag_sr : ag_srs) {
            auto selected_by_name = seqdb.select_by_name(ag_sr.name());
            selected_by_name->filter_name(ag_sr.name(), ag_sr.reassortant(), ag_sr.passage().to_string());
            if (!selected_by_name->empty()) {
                update(ag_sr, *selected_by_name);
                ++populated;
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(ag_sr)>, chart::v3::Antigen>) {
                if (!ag_sr.lab_ids().empty()) { // match by lab_id if match by name failed (e.g. CDC name in old tables)
                    for (const auto& lab_id : ag_sr.lab_ids()) {
                        auto selected_by_seq_id = seqdb.select_all();
                        selected_by_seq_id->filter_lab_id(lab_id);
                        if (!selected_by_seq_id->empty()) {
                            update(ag_sr, *selected_by_seq_id);
                            ++populated;
                        }
                    }
                }
            }
        }
        // AD_DEBUG("populated {}", populated);
        return populated;
    };

    return std::make_pair(populate(chart.antigens()), populate(chart.sera()));

} // ae::chart::v3::populate_from_seqdb

// ----------------------------------------------------------------------
