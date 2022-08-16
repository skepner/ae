#include "chart/v3/chart-seqdb.hh"
#include "chart/v3/selected-antigens-sera.hh"
#include "sequences/clades.hh"
#include "sequences/seqdb.hh"
#include "sequences/seqdb-selected.hh"

// ======================================================================

std::pair<size_t, size_t> ae::chart::v3::populate_from_seqdb(Chart& chart, bool report_matched)
{
    const auto& seqdb = ae::sequences::seqdb_for_subtype(chart.info().virus_subtype());
    const ae::sequences::Clades clades{ae::sequences::Clades::load_from_default_file};

    const auto ag_or_sr = [](const auto& ag_sr) {
        if (std::is_same_v<std::decay_t<decltype(ag_sr)>, Antigen>)
            return "AG";
        else
            return "SR";
    };

    const auto update = [&clades, report_matched, ag_or_sr](auto& ag_sr, size_t no, auto& selected) {
        selected.find_masters();
        selected.find_clades(clades);
        const auto& ref = selected.at(0);
        ag_sr.aa(ref.aa());
        ag_sr.nuc(ref.nuc());
        ag_sr.lineage(ref.entry->lineage);
        ag_sr.semantic().clades(ref.clades);
        if (const auto& insertions = ref.seq->aa_insertions; !insertions.empty())
            ag_sr.aa_insertions(insertions);
        AD_INFO(report_matched, "{} {:4d} {:70s}  {}", ag_or_sr(ag_sr), no, ag_sr.designation(), ref.clades);
    };

    const auto populate = [update, &seqdb](auto& ag_srs) -> size_t {
        size_t populated{0};
        size_t no{0};
        for (auto& ag_sr : ag_srs) {
            auto selected_by_name = seqdb.select_by_name(ag_sr.name());
            selected_by_name->filter_name(ag_sr.name(), ag_sr.reassortant(), ag_sr.passage().to_string());
            if (!selected_by_name->empty()) {
                update(ag_sr, no, *selected_by_name);
                ++populated;
            }
            else if constexpr (std::is_same_v<std::decay_t<decltype(ag_sr)>, chart::v3::Antigen>) {
                if (!ag_sr.lab_ids().empty()) { // match by lab_id if match by name failed (e.g. CDC name in old tables)
                    for (const auto& lab_id : ag_sr.lab_ids()) {
                        if (const auto selected_by_lab_id = seqdb.select_by_lab_id(lab_id); !selected_by_lab_id->empty()) {
                            update(ag_sr, no, *selected_by_lab_id);
                            ++populated;
                        }
                    }
                }
            }
            ++no;
        }
        // AD_DEBUG("populated {}", populated);
        return populated;
    };

    return std::make_pair(populate(chart.antigens()), populate(chart.sera()));

} // ae::chart::v3::populate_from_seqdb

// ----------------------------------------------------------------------
