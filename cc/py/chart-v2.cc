#include <cstdlib>

#include "py/module.hh"
#include "sequences/seqdb-selected.hh"
#include "chart/v2/factory-import.hh"
#include "chart/v2/factory-export.hh"
#include "chart/v2/chart-modify.hh"
#include "chart/v2/selected-antigens-sera.hh"
#include "chart/v2/text-export.hh"
#include "chart/v2/grid-test.hh"

// ======================================================================

namespace ae::py
{
    static inline unsigned make_info_data(bool column_bases, bool tables, bool tables_for_sera, bool antigen_dates)
    {
        using namespace ae::chart::v2;
        return (column_bases ? info_data::column_bases : 0)         //
               | (tables ? info_data::tables : 0)                   //
               | (tables_for_sera ? info_data::tables_for_sera : 0) //
               | (antigen_dates ? info_data::dates : 0);
    }

    // ----------------------------------------------------------------------

    static inline ae::chart::v2::ChartClone::clone_data clone_type(const std::string& type)
    {
        using namespace ae::chart::v2;
        if (type == "titers")
            return ChartClone::clone_data::titers;
        else if (type == "projections")
            return ChartClone::clone_data::projections;
        else if (type == "plot_spec")
            return ChartClone::clone_data::plot_spec;
        else if (type == "projections_plot_spec")
            return ChartClone::clone_data::projections_plot_spec;
        else
            throw std::invalid_argument{AD_FORMAT("Unrecognized clone \"type\": \"{}\"", type)};
    }

    static inline ae::chart::v2::GridTest::Results grid_test(ae::chart::v2::ChartModify& chart, std::shared_ptr<ae::chart::v2::SelectedAntigensModify> antigens,
                                                             std::shared_ptr<ae::chart::v2::SelectedSeraModify> sera, size_t projection_no, double grid_step, int threads)
    {
        ae::chart::v2::GridTest test{chart, projection_no, grid_step};
        ae::chart::v2::GridTest::Results results;
        if (!antigens && !sera) {
            results = test.test_all(threads);
        }
        else {
            ae::chart::v2::PointIndexList points_to_test;
            if (antigens)
                points_to_test = antigens->indexes;
            if (sera)
                ranges::for_each(sera->indexes, [number_of_antigens = chart.number_of_antigens(), &points_to_test](auto index) { points_to_test.insert(index + number_of_antigens); });
            results = test.test(*points_to_test, threads);
        }
        return results;
    }

    struct PlotSpecRef
    {
        std::shared_ptr<ae::chart::v2::PlotSpecModify> plot_spec;
        size_t number_of_antigens;

        acmacs::PointStyle antigen(size_t antigen_no) const { return plot_spec->style(antigen_no); }
        acmacs::PointStyle serum(size_t serum_no) const { return plot_spec->style(number_of_antigens + serum_no); }
    };

    // ----------------------------------------------------------------------

    inline void populate_from_seqdb(const std::shared_ptr<ae::chart::v2::ChartModify>& chart, bool remove_old_sequences_clades = false)
    {
        const auto subtype = chart->info()->virus_type();
        const auto& seqdb = ae::sequences::seqdb_for_subtype(subtype);
        const auto populate = [&seqdb, remove_old_sequences_clades](size_t /*no*/, auto ag_sr_ptr) {
            if (remove_old_sequences_clades) {
                ag_sr_ptr->sequence_aa({});
                ag_sr_ptr->sequence_nuc({});
                ag_sr_ptr->remove_all_clades();
            }
            auto selected = seqdb.select_all();
            selected->filter_name(ag_sr_ptr->name(), ag_sr_ptr->reassortant(), ag_sr_ptr->passage().to_string());
            if (!selected->empty()) {
                selected->find_masters();
                if (const char* clades_file = getenv("AC_CLADES_JSON_V2"); clades_file)
                    selected->find_clades(clades_file);
                ag_sr_ptr->sequence_aa(selected->at(0).aa());
                ag_sr_ptr->sequence_nuc(selected->at(0).nuc());
                for (const auto& clade : selected->at(0).clades)
                    ag_sr_ptr->add_clade(clade);
            }
            // else
            //     AD_DEBUG("populate_from_seqdb: no found \"{}\" R:\"{}\" P:\"{}\"", ag_sr_ptr->name(), ag_sr_ptr->reassortant(), ag_sr_ptr->passage());
        };
        ae::chart::v2::SelectedAntigensModify(chart).for_each(populate);
        ae::chart::v2::SelectedSeraModify(chart).for_each(populate);
    }

    template <typename Selected> static inline std::map<char, std::shared_ptr<Selected>> antigens_sera_by_aa_at_pos(std::shared_ptr<ae::chart::v2::ChartModify> chart, size_t pos1)
    {
        using namespace ae::chart::v2;
        populate_from_seqdb(chart);
        const ae::sequences::pos1_t pos{pos1};
        auto antigens = [&chart]() {
            if constexpr (std::is_same_v<Selected, SelectedAntigensModify>)
                return chart->antigens();
            else
                return chart->sera();
        }();

        std::map<char, std::shared_ptr<Selected>> result;
        for (const auto ag_no : range_from_0_to(antigens->size())) {
            const auto aa = ae::sequences::sequence_aa_t{antigens->at(ag_no)->sequence_aa()}[pos];
            const auto [it, added] = result.try_emplace(aa, std::make_shared<Selected>(chart, Selected::None));
            it->second->indexes.insert(ag_no);
        }
        return result;
    }

    // template <typename AgSr> static inline bool inside(const ae::chart::v2::SelectionData<AgSr> data, const acmacs::mapi::Figure& figure)
    // {
    //     return figure.inside(data.coord);
    // }

    template <typename AgSr> static inline bool clade_any_of(const ae::chart::v2::SelectionData<AgSr> data, const std::vector<std::string>& clades)
    {
        return data.ag_sr->clades().exists_any_of(clades);
    }

    template <typename AgSr> void deselect_not_sequenced(ae::chart::v2::PointIndexList& indexes, const AgSr& antigens)
    {
        indexes.get().erase(std::remove_if(indexes.begin(), indexes.end(), [&antigens](auto index) { return antigens.at(index)->sequence_aa().empty(); }), indexes.end());
    }

    template <typename AgSr> bool all_criteria_matched(const AgSr& antigen, const ae::sequences::amino_acid_at_pos1_eq_list_t& criteria)
    {
        const ae::sequences::sequence_aa_t seq{antigen.sequence_aa()};
        for (const auto& pos1_aa_eq : criteria) {
            if (std::get<bool>(pos1_aa_eq) != (seq[std::get<ae::sequences::pos1_t>(pos1_aa_eq)] == std::get<char>(pos1_aa_eq)))
                return false;
        }
        return true;
    }

    template <typename AgSr> void deselect_by_aa(ae::chart::v2::PointIndexList& indexes, const AgSr& antigens, const std::vector<std::string>& criteria)
    {
        const auto crits = ae::sequences::extract_aa_nuc_at_pos1_eq_list(criteria);
        const auto all_criteria_matched_pred = [&crits, &antigens](auto index) { return all_criteria_matched(*antigens.at(index), crits); };
        indexes.get().erase(std::remove_if(indexes.begin(), indexes.end(), all_criteria_matched_pred), indexes.end());
    }

} // namespace ae::py

// ----------------------------------------------------------------------

void ae::py::chart_v2(pybind11::module_& mdl)
{
    using namespace std::string_view_literals;
    using namespace pybind11::literals;
    using namespace ae::chart::v2;

    // ----------------------------------------------------------------------

    auto chart_v2_submodule = mdl.def_submodule("chart_v2", "chart_v2 api");

    pybind11::class_<ChartModify, std::shared_ptr<ChartModify>>(chart_v2_submodule, "Chart") //
        .def(pybind11::init([](pybind11::object path) { return std::make_shared<ChartModify>(import_from_file(pybind11::str(path))); }), "filename"_a, pybind11::doc("imports chart from a file"))

        .def(
            "clone",                                                                                                                                           //
            [](ChartModify& chart, const std::string& type) -> std::shared_ptr<ChartModify> { return std::make_shared<ChartClone>(chart, clone_type(type)); }, //
            "type"_a = "titers",                                                                                                                               //
            pybind11::doc(R"(type: "titers", "projections", "plot_spec", "projections_plot_spec")"))                                                           //

        .def(
            "make_name",                                                            //
            [](const ChartModify& chart) { return chart.make_name(std::nullopt); }, //
            pybind11::doc("returns name of the chart"))                             //
        .def(
            "make_name",                                                                                   //
            [](const ChartModify& chart, size_t projection_no) { return chart.make_name(projection_no); }, //
            "projection_no"_a,                                                                             //
            pybind11::doc("returns name of the chart with the stress of the passed projection"))           //
        .def(
            "table_as_text", //
            [](const ChartModify& chart, int layer_no, bool sort, bool clades, bool org_mode_separators, bool show_aa) {
                const auto layer{layer_no >= 0 ? std::optional<size_t>{static_cast<size_t>(layer_no)} : std::nullopt};
                const show_clades_t show_clades{clades ? show_clades_t::yes : show_clades_t::no};
                const org_mode_separators_t org_mode_sep{org_mode_separators ? org_mode_separators_t::yes : org_mode_separators_t::no};
                return ae::chart::v2::export_table_to_text(chart, layer, sort, show_clades, org_mode_sep, show_aa ? show_aa_t::yes : show_aa_t::no);
            },                                                                                                                                                            //
            "layer"_a = -1, "sort"_a = false, "show_clades"_a = false, "org_mode_separators"_a = false, "show_aa"_a = true,                                               //
            pybind11::doc("returns table as text\nif layer >= 0 shows corresponding layer\nif sort is True sort antigens/sera to be able to compare with another table")) //
        .def(
            "names_as_text",                                                                                                                  //
            [](std::shared_ptr<ChartModify> chart, const std::string& format) { return ae::chart::v2::export_names_to_text(chart, format); }, //
            "format"_a = "{ag_sr} {no0} {name_full}{ }{species}{ }{date_in_brackets}{ }{lab_ids}{ }{ref}\n",                                  //
            pybind11::doc("returns antigen and /serum names as text"))                                                                        //
        .def(
            "names_as_text", //
            [](const ChartModify& chart, const SelectedAntigensModify& antigens, const SelectedSeraModify& sera, const std::string& format) {
                return ae::chart::v2::export_names_to_text(chart, format, antigens, sera);
            },                                                                                                                       //
            "antigens"_a, "sera"_a, "format"_a = "{ag_sr} {no0} {name_full}{ }{species}{ }{date_in_brackets}{ }{lab_ids}{ }{ref}\n", //
            pybind11::doc("returns antigen and /serum names as text for pre-selected antigens/sera"))                                //

        .def("subtype", [](const ChartModify& chart) { return *chart.info()->virus_type(); })                            //
        .def("subtype_short", [](const ChartModify& chart) { return std::string{chart.info()->virus_type().h_or_b()}; }) //
        .def("subset", [](const ChartModify& chart) { return chart.info()->subset(); })                                  //
        .def("assay", [](const ChartModify& chart) { return *chart.info()->assay(); })                                   //
        .def("assay_hi_or_neut", [](const ChartModify& chart) { return chart.info()->assay().hi_or_neut(); })            //
        .def("lab", [](const ChartModify& chart) { return *chart.info()->lab(); })                                       //
        .def("rbc", [](const ChartModify& chart) { return *chart.info()->rbc_species(); })                               //
        .def("assay_rbc",
             [](const ChartModify& chart) {
                 const auto assay = chart.info()->assay().short_name();
                 if (assay == "HI")
                     return fmt::format("HI-{}", *chart.info()->rbc_species());
                 else
                     return assay;
             })                                                                                        //
        .def("date", [](const ChartModify& chart) { return *chart.info()->date(Info::Compute::Yes); }) //
        .def(
            "lineage", [](const ChartModify& chart) -> std::string { return chart.lineage(); }, pybind11::doc("returns chart lineage: VICTORIA, YAMAGATA")) //
        .def("subtype_lineage",
             [](const ChartModify& chart) {
                 const auto subtype = chart.info()->virus_type().h_or_b();
                 if (subtype == "B")
                     return fmt::format("B{}", chart.lineage());
                 else
                     return std::string{subtype};
             }) //

        .def("description",                                       //
             &Chart::description,                                 //
             pybind11::doc("returns chart one line description")) //

        .def(
            "make_info", //
            [](const ChartModify& chart, size_t max_number_of_projections_to_show, bool column_bases, bool tables, bool tables_for_sera, bool antigen_dates) {
                return chart.make_info(max_number_of_projections_to_show, make_info_data(column_bases, tables, tables_for_sera, antigen_dates));
            },                                                                                                                                               //
            "max_number_of_projections_to_show"_a = 20, "column_bases"_a = true, "tables"_a = false, "tables_for_sera"_a = false, "antigen_dates"_a = false, //
            pybind11::doc("returns detailed chart description"))                                                                                             //
        .def("__str__",                                                                                                                                      //
             [](const ChartModify& chart) { return chart.make_info(10, make_info_data(false, false, false, false)); })                                       //

        .def("number_of_antigens", &Chart::number_of_antigens)
        .def("number_of_sera", &Chart::number_of_sera)
        .def("number_of_projections", &Chart::number_of_projections)

        .def("populate_from_seqdb", &ae::py::populate_from_seqdb, "remove_old_sequences_clades"_a = false, pybind11::doc("populate with sequences from seqdb")) //

        .def(
            "relax", //
            [](ChartModify& chart, size_t number_of_dimensions, size_t number_of_optimizations, const std::string& minimum_column_basis, bool dimension_annealing, bool rough,
               size_t /*number_of_best_distinct_projections_to_keep*/, std::shared_ptr<SelectedAntigensModify> antigens_to_disconnect, std::shared_ptr<SelectedSeraModify> sera_to_disconnect) {
                if (number_of_optimizations == 0)
                    number_of_optimizations = 100;
                ae::chart::v2::DisconnectedPoints disconnect;
                if (antigens_to_disconnect && !antigens_to_disconnect->empty())
                    disconnect.extend(antigens_to_disconnect->points());
                if (sera_to_disconnect && !sera_to_disconnect->empty())
                    disconnect.extend(sera_to_disconnect->points());
                chart.relax(number_of_optimizations_t{number_of_optimizations}, MinimumColumnBasis{minimum_column_basis}, ae::chart::v2::number_of_dimensions_t{number_of_dimensions},
                            use_dimension_annealing_from_bool(dimension_annealing), optimization_options{optimization_precision{rough ? optimization_precision::rough : optimization_precision::fine}},
                            disconnect);
                chart.projections_modify().sort();
            },                                                                                                                                                    //
            "number_of_dimensions"_a = 2, "number_of_optimizations"_a = 0, "minimum_column_basis"_a = "none", "dimension_annealing"_a = false, "rough"_a = false, //
            "unused_number_of_best_distinct_projections_to_keep"_a = 5, "disconnect_antigens"_a = nullptr, "disconnect_sera"_a = nullptr,                         //
            pybind11::doc{"makes one or more antigenic maps from random starting layouts, adds new projections, projections are sorted by stress"})               //

        .def(
            "relax_incremental", //
            [](ChartModify& chart, size_t projection_no, size_t number_of_optimizations, bool rough, size_t /*number_of_best_distinct_projections_to_keep*/, bool remove_source_projection,
               bool unmovable_non_nan_points) {
                if (number_of_optimizations == 0)
                    number_of_optimizations = 100;
                chart.relax_incremental(projection_no, number_of_optimizations_t{number_of_optimizations},
                                        optimization_options{optimization_precision{rough ? optimization_precision::rough : optimization_precision::fine}},
                                        ae::chart::v2::remove_source_projection{remove_source_projection ? ae::chart::v2::remove_source_projection::yes : ae::chart::v2::remove_source_projection::no},
                                        ae::chart::v2::unmovable_non_nan_points{unmovable_non_nan_points ? ae::chart::v2::unmovable_non_nan_points::yes : ae::chart::v2::unmovable_non_nan_points::no});
                chart.projections_modify().sort();
            }, //
            "projection_no"_a = 0, "number_of_optimizations"_a = 0, "rough"_a = false, "number_of_best_distinct_projections_to_keep"_a = 5, "remove_source_projection"_a = true,
            "unmovable_non_nan_points"_a = false) //

        .def("grid_test", &grid_test, "antigens"_a = nullptr, "sera"_a = nullptr, "projection_no"_a = 0, "grid_step"_a = 0.1, "threads"_a = 0) //

        .def(
            "projection",                                                                                    //
            [](ChartModify& chart, size_t projection_no) { return chart.projection_modify(projection_no); }, //
            "projection_no"_a = 0)                                                                           //

        .def("remove_all_projections",                                                   //
             [](ChartModify& chart) { return chart.projections_modify().remove_all(); }) //
        .def(
            "remove_all_projections_except",                                                                                    //
            [](ChartModify& chart, size_t to_keep) { return chart.projections_modify().remove_all_except(to_keep); }, "keep"_a) //
        .def(
            "remove_projection",                                                                                                  //
            [](ChartModify& chart, size_t to_remove) { return chart.projections_modify().remove(to_remove); }, "projection_no"_a) //
        .def(
            "keep_projections",                                                                               //
            [](ChartModify& chart, size_t to_keep) { return chart.projections_modify().keep_just(to_keep); }, //
            "keep"_a)                                                                                         //

        .def(
            "orient_to",
            [](ChartModify& chart, const ChartModify& master, size_t projection_no) {
                ae::chart::v2::CommonAntigensSera common(master, chart, CommonAntigensSera::match_level_t::strict);
                const auto procrustes_data = ae::chart::v2::procrustes(*master.projection(0), *chart.projection(projection_no), common.points(), ae::chart::v2::procrustes_scaling_t::no);
                chart.projection_modify(projection_no)->transformation(procrustes_data.transformation);
            },                                 //
            "master"_a, "projection_no"_a = 0) //

        .def(
            "export", //
            [](ChartModify& chart, pybind11::object path, pybind11::object program_name) {
                const std::string path_s = pybind11::str(path), pn_s = pybind11::str(program_name);
                ae::chart::v2::export_factory(chart, path_s, pn_s);
            },                                            //
            "filename"_a, "program_name"_a = "acmacs-py") //

        .def("antigen", &ChartModify::antigen, "antigen_no"_a) //
        .def("serum", &ChartModify::serum, "serum_no"_a)       //

        .def(
            "select_antigens", //
            [](std::shared_ptr<ChartModify> chart, const std::function<bool(const SelectionData<Antigen>&)>& func, size_t projection_no, bool report) {
                auto selected = std::make_shared<SelectedAntigensModify>(chart, func, projection_no);
                AD_PRINT_L(report, [&selected]() { return selected->report("{ag_sr} {no0:{num_digits}d} {name_full_passage}\n"); });
                return selected;
            },                                                        //
            "predicate"_a, "projection_no"_a = 0, "report"_a = false, //
            pybind11::doc("Passed predicate (function with one arg: SelectionDataAntigen object)\n"
                          "is called for each antigen, selects just antigens for which predicate\n"
                          "returns True, returns SelectedAntigens object.")) //
        // .def(
        //     "select_antigens_by_aa", //
        //     [](std::shared_ptr<ChartModify> chart, const std::vector<std::string>& criteria, bool report) {
        //         auto selected = std::make_shared<SelectedAntigensModify>(chart);
        //         ae::py::populate_from_seqdb(chart);
        //         ae::py::select_by_aa(selected->indexes, *chart->antigens(), criteria);
        //         AD_PRINT_L(report, [&selected]() { return selected->report("{ag_sr} {no0:{num_digits}d} {name_full_passage}\n"); });
        //         return selected;
        //     },                                                                                                        //
        //     "criteria"_a, "report"_a = false,                                                                         //
        //     pybind11::doc(R"(Criteria is a list of strings, e.g. ["156K", "!145K"], all criteria is the list must match)")) //
        // .def(
        //     "select_antigens_by_clade", //
        //     [](std::shared_ptr<ChartModify> chart, const std::vector<std::string>& clades, bool report) {
        //         auto selected = std::make_shared<SelectedAntigensModify>(chart);
        //         ae::py::populate_from_seqdb(chart);
        //         const auto pred = [&clades, antigens = chart->antigens()](auto index) { return antigens->at(index)->clades().exists_any_of(clades); };
        //         selected->indexes.get().erase(std::remove_if(selected->indexes.begin(), selected->indexes.end(), pred), selected->indexes.end());
        //         AD_PRINT_L(report, [&selected]() { return selected->report("{ag_sr} {no0:{num_digits}d} {name_full_passage}\n"); });
        //         return selected;
        //     },                                                                                                //
        //     "clades"_a, "report"_a = false,                                                                   //
        //     pybind11::doc(R"(Select antigens with a clade from clades, one or more entries in clades must match)")) //
        .def(
            "select_all_antigens",                                                                              //
            [](std::shared_ptr<ChartModify> chart) { return std::make_shared<SelectedAntigensModify>(chart); }, //
            pybind11::doc(R"(Selects all antigens and returns SelectedAntigens object.)"))                      //
        .def(
            "select_no_antigens",                                                                                                             //
            [](std::shared_ptr<ChartModify> chart) { return std::make_shared<SelectedAntigensModify>(chart, SelectedAntigensModify::None); }, //
            pybind11::doc(R"(Selects no antigens and returns SelectedAntigens object.)"))                                                     //

        .def("antigens_by_aa_at_pos", &ae::py::antigens_sera_by_aa_at_pos<SelectedAntigensModify>, "pos"_a,
             pybind11::doc(R"(Returns dict with AA at passed pos as keys and SelectedAntigens as values.)")) //
        .def("sera_by_aa_at_pos", &ae::py::antigens_sera_by_aa_at_pos<SelectedSeraModify>, "pos"_a,
             pybind11::doc(R"(Returns dict with AA at passed pos as keys and SelectedSera as values.)")) //

        .def(
            "select_sera", //
            [](std::shared_ptr<ChartModify> chart, const std::function<bool(const SelectionData<Serum>&)>& func, size_t projection_no, bool report) {
                auto selected = std::make_shared<SelectedSeraModify>(chart, func, projection_no);
                AD_PRINT_L(report, [&selected]() { return selected->report("{ag_sr} {no0:{num_digits}d} {name_full_passage}\n"); });
                return selected;
            },                                                        //
            "predicate"_a, "projection_no"_a = 0, "report"_a = false, //
            pybind11::doc("Passed predicate (function with one arg: SelectionDataSerum object)\n"
                          "is called for each serum, selects just sera for which predicate\n"
                          "returns True, returns SelectedSera object.")) //
        // .def(
        //     "select_sera_by_aa", //
        //     [](std::shared_ptr<ChartModify> chart, const std::vector<std::string>& criteria, bool report) {
        //         auto selected = std::make_shared<SelectedSeraModify>(chart);
        //         ae::py::populate_from_seqdb(chart);
        //         ae::py::select_by_aa(selected->indexes, *chart->sera(), criteria);
        //         AD_PRINT_L(report, [&selected]() { return selected->report("{ag_sr} {no0:{num_digits}d} {name_full_passage}\n"); });
        //         return selected;
        //     },                                                                                                         //
        //     "criteria"_a, "report"_a = false,                                                                          //
        //     pybind11::doc("Criteria is a list of strings, e.g. [\"156K\", \"!145K\"], all criteria is the list must match")) //
        // .def(
        //     "select_sera_by_clade", //
        //     [](std::shared_ptr<ChartModify> chart, const std::vector<std::string>& clades, bool report) {
        //         auto selected = std::make_shared<SelectedSeraModify>(chart);
        //         ae::py::populate_from_seqdb(chart);
        //         const auto pred = [&clades, sera = chart->sera()](auto index) { return sera->at(index)->clades().exists_any_of(clades); };
        //         selected->indexes.get().erase(std::remove_if(selected->indexes.begin(), selected->indexes.end(), pred), selected->indexes.end());
        //         AD_PRINT_L(report, [&selected]() { return selected->report("{ag_sr} {no0:{num_digits}d} {name_full_passage}\n"); });
        //         return selected;
        //     },                                                                                            //
        //     "clades"_a, "report"_a = false,                                                               //
        //     pybind11::doc(R"(Select sera with a clade from clades, one or more entries in clades must match)")) //
        .def(
            "select_all_sera",                                                                              //
            [](std::shared_ptr<ChartModify> chart) { return std::make_shared<SelectedSeraModify>(chart); }, //
            pybind11::doc(R"(Selects all sera and returns SelectedSera object.)"))                          //
        .def(
            "select_no_sera",                                                                                                         //
            [](std::shared_ptr<ChartModify> chart) { return std::make_shared<SelectedSeraModify>(chart, SelectedSeraModify::None); }, //
            pybind11::doc(R"(Selects no sera and returns SelectedSera object.)"))                                                     //

        .def("titers", &ChartModify::titers_modify_ptr, pybind11::doc("returns Titers oject"))

        .def("column_basis", &ChartModify::column_basis, "serum_no"_a, "projection_no"_a = 0, pybind11::doc("return column_basis for the passed serum"))
        .def(
            "column_bases", [](const ChartModify& chart, std::string_view minimum_column_basis) { return chart.column_bases(MinimumColumnBasis{minimum_column_basis})->data(); },
            "minimum_column_basis"_a, pybind11::doc("get column bases")) //
        .def(
            "column_bases", [](ChartModify& chart, const std::vector<double>& column_bases) { chart.forced_column_bases_modify(ColumnBasesData{column_bases}); }, "column_bases"_a,
            pybind11::doc("set forced column bases")) //

        .def("plot_spec", [](ChartModify& chart) { return PlotSpecRef{.plot_spec = chart.plot_spec_modify_ptr(), .number_of_antigens = chart.number_of_antigens()}; }) //

        .def("combine_projections", &ChartModify::combine_projections, "merge_in"_a) //

        .def(
            "remove_antigens_sera",
            [](ChartModify& chart, std::shared_ptr<SelectedAntigensModify> antigens, std::shared_ptr<SelectedSeraModify> sera, bool remove_projections) {
                if (remove_projections)
                    chart.projections_modify().remove_all();
                if (antigens && !antigens->empty())
                    chart.remove_antigens(ae::chart::v2::ReverseSortedIndexes{*antigens->indexes});
                if (sera && !sera->empty())
                    chart.remove_sera(ae::chart::v2::ReverseSortedIndexes{*sera->indexes});
            },                                                                          //
            "antigens"_a = nullptr, "sera"_a = nullptr, "remove_projections"_a = false, //
            pybind11::doc(R"(
Usage:
    chart.remove_antigens_sera(antigens=chart.select_antigens(lambda ag: ag.lineage == "VICTORIA"), sera=chart.select_sera(lambda sr: sr.lineage == "VICTORIA"))
)"))                                                                                    //
        .def(
            "keep_antigens_sera",
            [](ChartModify& chart, std::shared_ptr<SelectedAntigensModify> antigens, std::shared_ptr<SelectedSeraModify> sera, bool remove_projections) {
                if (remove_projections)
                    chart.projections_modify().remove_all();
                if (antigens && !antigens->empty()) {
                    ae::chart::v2::ReverseSortedIndexes antigens_to_remove(chart.number_of_antigens());
                    antigens_to_remove.remove(*antigens->indexes);
                    // AD_INFO("antigens_to_remove:  {} {}", antigens_to_remove.size(), antigens_to_remove);
                    chart.remove_antigens(antigens_to_remove);
                }
                if (sera && !sera->empty()) {
                    ae::chart::v2::ReverseSortedIndexes sera_to_remove(chart.number_of_sera());
                    sera_to_remove.remove(*sera->indexes);
                    // AD_INFO("sera_to_remove:  {} {}", sera_to_remove.size(), sera_to_remove);
                    chart.remove_sera(sera_to_remove);
                }
            },                                                                          //
            "antigens"_a = nullptr, "sera"_a = nullptr, "remove_projections"_a = false, //
            pybind11::doc(R"(
Usage:
    chart.remove_antigens_sera(antigens=chart.select_antigens(lambda ag: ag.lineage == "VICTORIA"), sera=chart.select_sera(lambda sr: sr.lineage == "VICTORIA"))
)"))                                                                                    //
        ;

    // ----------------------------------------------------------------------

    pybind11::class_<ae::draw::v1::Transformation>(chart_v2_submodule, "Transformation")                                      //
        .def("__str__", [](const ae::draw::v1::Transformation& transformation) { return fmt::format("{}", transformation); }) //
        ;

    pybind11::class_<ae::chart::v2::Area>(chart_v2_submodule, "Area")                                                //
        .def("__str__", [](const ae::chart::v2::Area& transformation) { return fmt::format("{}", transformation); }) //
        .def_property_readonly("min", [](const ae::chart::v2::Area& area) { return area.min.as_vector(); })          //
        .def_property_readonly("max", [](const ae::chart::v2::Area& area) { return area.max.as_vector(); })          //
        .def("area", &ae::chart::v2::Area::area)                                                                     //
        ;

    chart_v2_submodule.def("intersection", &ae::chart::v2::intersection, "area1"_a, "area2"_a);

    pybind11::class_<ProjectionModify, std::shared_ptr<ProjectionModify>>(chart_v2_submodule, "Projection") //
        .def(
            "stress",                                                                                                                                                      //
            [](const ProjectionModify& projection, bool recalculate) { return projection.stress(recalculate ? RecalculateStress::if_necessary : RecalculateStress::no); }, //
            "recalculate"_a = false)                                                                                                                                       //
        .def(
            "relax",
            [](ProjectionModify& projection, bool rough) {
                projection.relax(ae::chart::v2::optimization_options{optimization_precision{rough ? optimization_precision::rough : optimization_precision::fine}});
            },
            "rough"_a = false)                                                                                                                                   //
        .def_property_readonly("no", &ProjectionModify::projection_no)                                                                                           //
        .def("transformation", pybind11::overload_cast<>(&ProjectionModify::transformation, pybind11::const_))                                                   //
        .def("layout", [](const ProjectionModify& proj) { return proj.layout()->as_vector_of_vectors_double(); })                                                //
        .def("transformed_layout", [](const ProjectionModify& proj) { return proj.transformed_layout()->as_vector_of_vectors_double(); })                        //
        .def("connect_all_disconnected", &ProjectionModify::connect_all_disconnected, pybind11::doc("reconnected points still have NaN coordinates after call")) //
        .def("comment", pybind11::overload_cast<>(&ProjectionModify::comment, pybind11::const_))                                                                 //
        .def("comment", pybind11::overload_cast<std::string>(&ProjectionModify::comment))                                                                        //
        ;

    // ----------------------------------------------------------------------

    pybind11::class_<ae::chart::v2::GridTest::Results>(chart_v2_submodule, "GridTestResults")             //
        .def("__str__", [](const ae::chart::v2::GridTest::Results& results) { return results.report(); }) //
        .def(
            "report", [](const ae::chart::v2::GridTest::Results& results, const ae::chart::v2::ChartModify& chart) { return results.report(chart); }, "chart"_a) //
        .def(
            "json", [](const ae::chart::v2::GridTest::Results& results, const ae::chart::v2::ChartModify& chart) { return results.export_to_json(chart, 0); }, "chart"_a) //
        ;

    // ----------------------------------------------------------------------

    pybind11::class_<PlotSpecRef>(chart_v2_submodule, "PlotSpec") //
        .def("antigen", &PlotSpecRef::antigen, "antigen_no"_a)    //
        .def("serum", &PlotSpecRef::serum, "serum_no"_a)          //
        ;

    using namespace acmacs;

    pybind11::class_<PointStyle>(chart_v2_submodule, "PointStyle")                         //
        .def("shown", pybind11::overload_cast<>(&PointStyle::shown, pybind11::const_))     //
        .def("fill", pybind11::overload_cast<>(&PointStyle::fill, pybind11::const_))       //
        .def("outline", pybind11::overload_cast<>(&PointStyle::outline, pybind11::const_)) //
        .def("outline_width", [](const PointStyle& ps) { return *ps.outline_width(); })    //
        .def("size", [](const PointStyle& ps) { return *ps.size(); })                      //
        .def("diameter", [](const PointStyle& ps) { return *ps.diameter(); })              //
        .def("rotation", [](const PointStyle& ps) { return *ps.rotation(); })              //
        .def("aspect", [](const PointStyle& ps) { return *ps.aspect(); })                  //
        .def("shape", [](const PointStyle& ps) { return fmt::format("{}", ps.shape()); })  //
        // .def("label", pybind11::overload_cast<>(&PointStyle::label, pybind11::const_)) //
        .def("label_text", pybind11::overload_cast<>(&PointStyle::label_text, pybind11::const_)) //
        ;

    // ======================================================================

    pybind11::class_<detail::AntigenSerum, std::shared_ptr<detail::AntigenSerum>>(chart_v2_submodule, "AntigenSerum")                               //
        .def("__str__", [](const detail::AntigenSerum& ag_sr) { return ag_sr.format("{fields}"); })                                                 //
        .def("name", [](const detail::AntigenSerum& ag_sr) -> std::string { return std::string{*ag_sr.name()}; })                                   //
        .def("name_full", &detail::AntigenSerum::name_full)                                                                                         //
        .def("passage", &detail::AntigenSerum::passage)                                                                                             //
        .def("lineage", [](const detail::AntigenSerum& ag_sr) { return ag_sr.lineage().to_string(); })                                              //
        .def("reassortant", [](const detail::AntigenSerum& ag_sr) { return *ag_sr.reassortant(); })                                                 //
        .def("annotations", &detail::AntigenSerum::annotations)                                                                                     //
        .def("format", [](const detail::AntigenSerum& ag_sr, const std::string& pattern) { return ag_sr.format(pattern, collapse_spaces_t::yes); }) //
        .def(
            "is_egg", [](const detail::AntigenSerum& ag_sr, bool reass_as_egg) { return ag_sr.is_egg(reass_as_egg ? reassortant_as_egg::yes : reassortant_as_egg::no); },
            "reassortant_as_egg"_a = true)                        //
        .def("is_cell", &detail::AntigenSerum::is_cell)           //
        .def("passage_type", &detail::AntigenSerum::passage_type) //
        .def("distinct", &detail::AntigenSerum::distinct)         //
        .def("location", &detail::AntigenSerum::location_data)    //
        ;

    pybind11::class_<Antigen, std::shared_ptr<Antigen>, detail::AntigenSerum>(chart_v2_submodule, "AntigenRO") //
        .def("date", [](const Antigen& ag) { return *ag.date(); })                                             //
        .def("reference", &Antigen::reference)                                                                 //
        .def("lab_ids", [](const Antigen& ag) { return *ag.lab_ids(); })                                       //
        ;

    pybind11::class_<AntigenModify, std::shared_ptr<AntigenModify>, Antigen>(chart_v2_submodule, "Antigen")                                         //
        .def("name", [](const AntigenModify& ag) { return *ag.name(); })                                                                            //
        .def("name", [](AntigenModify& ag, const std::string& new_name) { ag.name(new_name); })                                                     //
        .def("passage", [](const AntigenModify& ag) { return ag.passage(); })                                                                       //
        .def("passage", [](AntigenModify& ag, const std::string& new_passage) { ag.passage(ae::virus::Passage{new_passage}); })                     //
        .def("passage", [](AntigenModify& ag, const ae::virus::Passage& new_passage) { ag.passage(new_passage); })                                  //
        .def("reassortant", [](const AntigenModify& ag) { return *ag.reassortant(); })                                                              //
        .def("reassortant", [](AntigenModify& ag, const std::string& new_reassortant) { ag.reassortant(ae::virus::Reassortant{new_reassortant}); }) //
        .def("add_annotation", &AntigenModify::add_annotation)                                                                                      //
        .def("remove_annotation", &AntigenModify::remove_annotation)                                                                                //
        .def("date", [](const AntigenModify& ag) { return *ag.date(); })                                                                            //
        .def("date", [](AntigenModify& ag, const std::string& new_date) { ag.date(new_date); })                                                     //
        .def("reference", [](const AntigenModify& ag) { return ag.reference(); })                                                                   //
        .def("reference", [](AntigenModify& ag, bool new_reference) { ag.reference(new_reference); })                                               //
        .def("sequenced", [](AntigenModify& ag) { return ag.sequenced(); })                                                                         //
        .def("sequence_aa", [](AntigenModify& ag) { return ae::sequences::sequence_aa_t{ag.sequence_aa()}; })                                       //
        .def("sequence_aa", [](AntigenModify& ag, std::string_view sequence) { ag.sequence_aa(sequence); })                                         //
        .def("sequence_nuc", [](AntigenModify& ag) { return ae::sequences::sequence_nuc_t{ag.sequence_nuc()}; })                                    //
        .def("sequence_nuc", [](AntigenModify& ag, std::string_view sequence) { ag.sequence_nuc(sequence); })                                       //
        .def("add_clade", &AntigenModify::add_clade)                                                                                                //
        .def("clades", [](const AntigenModify& ag) { return *ag.clades(); })                                                                        //
        ;

    pybind11::class_<Serum, std::shared_ptr<Serum>, detail::AntigenSerum>(chart_v2_submodule, "SerumRO") //
        .def("serum_id", [](const Serum& sr) { return *sr.serum_id(); })                                 //
        .def("serum_species", [](const Serum& sr) { return *sr.serum_species(); })                       //
        .def("homologous_antigens", &Serum::homologous_antigens)                                         //
        ;

    pybind11::class_<SerumModify, std::shared_ptr<SerumModify>, Serum>(chart_v2_submodule, "Serum")                                               //
        .def("name", [](const SerumModify& sr) -> std::string { return std::string{*sr.name()}; })                                                //
        .def("name", [](SerumModify& sr, const std::string& new_name) { sr.name(new_name); })                                                     //
        .def("passage", [](const SerumModify& sr) { return sr.passage(); })                                                                       //
        .def("passage", [](SerumModify& sr, const std::string& new_passage) { sr.passage(ae::virus::Passage{new_passage}); })                     //
        .def("passage", [](SerumModify& sr, const ae::virus::Passage& new_passage) { sr.passage(new_passage); })                                  //
        .def("reassortant", [](const SerumModify& sr) { return *sr.reassortant(); })                                                              //
        .def("reassortant", [](SerumModify& sr, const std::string& new_reassortant) { sr.reassortant(ae::virus::Reassortant{new_reassortant}); }) //
        .def("add_annotation", &SerumModify::add_annotation)                                                                                      //
        .def("remove_annotation", &SerumModify::remove_annotation)                                                                                //
        .def("serum_id", [](const SerumModify& sr) { return *sr.serum_id(); })                                                                    //
        .def("serum_id", [](SerumModify& sr, const std::string& new_serum_id) { return sr.serum_id(SerumId{new_serum_id}); })                     //
        .def("serum_species", [](const SerumModify& sr) { return *sr.serum_species(); })                                                          //
        .def("serum_species", [](SerumModify& sr, const std::string& new_species) { return sr.serum_species(SerumSpecies{new_species}); })        //
        .def("sequenced", [](SerumModify& sr) { return sr.sequenced(); })                                                                         //
        .def("sequence_aa", [](SerumModify& sr) { return ae::sequences::sequence_aa_t{sr.sequence_aa()}; })                                       //
        .def("sequence_aa", [](SerumModify& sr, std::string_view sequence) { sr.sequence_aa(sequence); })                                         //
        .def("sequence_nuc", [](SerumModify& sr) { return ae::sequences::sequence_nuc_t{sr.sequence_nuc()}; })                                    //
        .def("sequence_nuc", [](SerumModify& sr, std::string_view sequence) { sr.sequence_nuc(sequence); })                                       //
        .def("add_clade", &SerumModify::add_clade)                                                                                                //
        .def("clades", [](const SerumModify& sr) { return *sr.clades(); })                                                                        //
        ;

    // ----------------------------------------------------------------------

    pybind11::class_<SelectionData<Antigen>>(chart_v2_submodule, "SelectionDataAntigen")                                                         //
        .def_readonly("no", &SelectionData<Antigen>::index)                                                                                      //
        .def_readonly("point_no", &SelectionData<Antigen>::point_no)                                                                             //
        .def_readonly("antigen", &SelectionData<Antigen>::ag_sr)                                                                                 //
        .def_readonly("coordinates", &SelectionData<Antigen>::coord)                                                                             //
        .def_property_readonly("name", [](const SelectionData<Antigen>& data) { return *data.ag_sr->name(); })                                   //
        .def_property_readonly("lineage", [](const SelectionData<Antigen>& data) { return data.ag_sr->lineage().to_string(); })                  //
        .def_property_readonly("passage", [](const SelectionData<Antigen>& data) { return data.ag_sr->passage(); })                              //
        .def_property_readonly("reassortant", [](const SelectionData<Antigen>& data) { return *data.ag_sr->reassortant(); })                     //
        .def_property_readonly("aa", [](const SelectionData<Antigen>& data) { return ae::sequences::sequence_aa_t{data.ag_sr->sequence_aa()}; }) //
        // .def("inside", &inside<Antigen>, "figure"_a)                                                                                                  //
        .def("clade_any_of", &ae::py::clade_any_of<Antigen>, "clades"_a) //
        ;

    pybind11::class_<SelectionData<Serum>>(chart_v2_submodule, "SelectionDataSerum")                                                           //
        .def_readonly("no", &SelectionData<Serum>::index)                                                                                      //
        .def_readonly("point_no", &SelectionData<Serum>::point_no)                                                                             //
        .def_readonly("serum", &SelectionData<Serum>::ag_sr)                                                                                   //
        .def_readonly("coordinates", &SelectionData<Serum>::coord)                                                                             //
        .def_property_readonly("name", [](const SelectionData<Serum>& data) { return *data.ag_sr->name(); })                                   //
        .def_property_readonly("lineage", [](const SelectionData<Serum>& data) { return data.ag_sr->lineage().to_string(); })                  //
        .def_property_readonly("passage", [](const SelectionData<Serum>& data) { return data.ag_sr->passage(); })                              //
        .def_property_readonly("reassortant", [](const SelectionData<Serum>& data) { return *data.ag_sr->reassortant(); })                     //
        .def_property_readonly("serum_id", [](const SelectionData<Serum>& data) { return *data.ag_sr->serum_id(); })                           //
        .def_property_readonly("serum_species", [](const SelectionData<Serum>& data) { return *data.ag_sr->serum_species(); })                 //
        .def_property_readonly("aa", [](const SelectionData<Serum>& data) { return ae::sequences::sequence_aa_t{data.ag_sr->sequence_aa()}; }) //
        // .def("inside", &inside<Serum>, "figure"_a)                                                                                                  //
        .def("clade_any_of", &ae::py::clade_any_of<Serum>, "clades"_a) //
        ;

    // ----------------------------------------------------------------------

    pybind11::class_<SelectedAntigensModify, std::shared_ptr<SelectedAntigensModify>>(chart_v2_submodule, "SelectedAntigens")
        .def(
            "deselect_by_aa",
            [](SelectedAntigensModify& selected, const std::vector<std::string>& criteria) {
                ae::py::populate_from_seqdb(selected.chart);
                ae::py::deselect_by_aa(selected.indexes, *selected.chart->antigens(), criteria);
                return selected;
            },
            "criteria"_a, pybind11::doc("Criteria is a list of strings, e.g. [\"156K\", \"!145K\"], all criteria is the list must match")) //
        .def(
            "exclude",
            [](SelectedAntigensModify& selected, const SelectedAntigensModify& exclude) {
                selected.exclude(exclude);
                return selected;
            },
            "exclude"_a, pybind11::doc("Deselect antigens selected by exclusion list")) //
        .def(
            "filter_sequenced",
            [](SelectedAntigensModify& selected) {
                ae::py::populate_from_seqdb(selected.chart);
                ae::py::deselect_not_sequenced(selected.indexes, *selected.chart->antigens());
                return selected;
            },
            pybind11::doc("deselect not sequenced"))                                                                                   //
        .def("report", &SelectedAntigensModify::report, "format"_a = "{no0},")                                                         //
        .def("report_list", &SelectedAntigensModify::report_list, "format"_a = "{name}")                                               //
        .def("__repr__", [](const SelectedAntigensModify& selected) { return fmt::format("SelectedAntigens ({})", selected.size()); }) //
        .def("empty", &SelectedAntigensModify::empty)                                                                                  //
        .def("size", &SelectedAntigensModify::size)                                                                                    //
        .def("__len__", &SelectedAntigensModify::size)                                                                                 //
        .def("__getitem__", &SelectedAntigensModify::operator[])                                                                       //
        .def("__bool_", [](const SelectedAntigensModify& antigens) { return !antigens.empty(); })                                      //
        .def(
            "__iter__", [](SelectedAntigensModify& antigens) { return pybind11::make_iterator(antigens.begin(), antigens.end()); }, pybind11::keep_alive<0, 1>()) //
        .def("indexes", [](const SelectedAntigensModify& selected) { return *selected.indexes; })                                                                 //
        .def(
            "points", [](const SelectedAntigensModify& selected) { return *selected.points(); }, pybind11::doc("return point numbers")) //
        .def(
            "area", [](const SelectedAntigensModify& selected, size_t projection_no) { return selected.area(projection_no); }, "projection_no"_a = 0,
            pybind11::doc("return boundaries of the selected points (not transformed)")) //
        .def(
            "area_transformed", [](const SelectedAntigensModify& selected, size_t projection_no) { return selected.area_transformed(projection_no); }, "projection_no"_a = 0,
            pybind11::doc("return boundaries of the selected points (transformed)")) //
        .def("for_each", &SelectedAntigensModify::for_each, "modifier"_a,
             pybind11::doc("modifier(ag_no, antigen) is called for each selected antigen, antigen fields, e.g. name, can be modified in the function.")) //
        ;

    pybind11::class_<SelectedSeraModify, std::shared_ptr<SelectedSeraModify>>(chart_v2_submodule, "SelectedSera")
        .def(
            "deselect_by_aa",
            [](SelectedSeraModify& selected, const std::vector<std::string>& criteria) {
                ae::py::populate_from_seqdb(selected.chart);
                ae::py::deselect_by_aa(selected.indexes, *selected.chart->sera(), criteria);
                return selected;
            },
            "criteria"_a, pybind11::doc("Criteria is a list of strings, e.g. [\"156K\", \"!145K\"], all criteria is the list must match")) //
        .def(
            "exclude",
            [](SelectedSeraModify& selected, const SelectedSeraModify& exclude) {
                selected.exclude(exclude);
                return selected;
            },
            "exclude"_a, pybind11::doc("Deselect sera selected by exclusion list")) //
        .def(
            "filter_sequenced",
            [](SelectedSeraModify& selected) {
                ae::py::populate_from_seqdb(selected.chart);
                ae::py::deselect_not_sequenced(selected.indexes, *selected.chart->sera());
                return selected;
            },
            pybind11::doc("deselect not sequenced"))                                                                           //
        .def("report", &SelectedSeraModify::report, "format"_a = "{no0},")                                                     //
        .def("report_list", &SelectedSeraModify::report_list, "format"_a = "{name}")                                           //
        .def("__repr__", [](const SelectedSeraModify& selected) { return fmt::format("SelectedSera ({})", selected.size()); }) //
        .def("empty", &SelectedSeraModify::empty)                                                                              //
        .def("size", &SelectedSeraModify::size)                                                                                //
        .def("__len__", &SelectedSeraModify::size)                                                                             //
        .def("__bool_", [](const SelectedSeraModify& sera) { return !sera.empty(); })                                          //
        .def(
            "__iter__", [](SelectedSeraModify& sera) { return pybind11::make_iterator(sera.begin(), sera.end()); }, pybind11::keep_alive<0, 1>()) //
        .def("__getitem__", &SelectedSeraModify::operator[])                                                                                      //
        .def("indexes", [](const SelectedSeraModify& selected) { return *selected.indexes; })                                                     //
        .def("for_each", &SelectedSeraModify::for_each, "modifier"_a,
             pybind11::doc("modifier(sr_no, serum) is called for each selected serum, serum fields, e.g. name, can be modified in the function.")) //
        ;

    // ----------------------------------------------------------------------

    chart_v2_submodule.def(
        "titer_merge",
        [](const std::vector<std::string>& source_titers) {
            if (source_titers.empty())
                throw std::invalid_argument{"titer_merge: no source titers"};
            if (source_titers.size() == 1)
                return std::make_pair(source_titers[0], std::string{});
            constexpr double standard_deviation_threshold = 1.0; // lispmds: average-multiples-unless-sd-gt-1-ignore-thresholded-unless-only-entries-then-min-threshold
            std::vector<ae::chart::v2::Titer> titers(source_titers.size());
            std::transform(source_titers.begin(), source_titers.end(), std::begin(titers), [](const auto& src) { return ae::chart::v2::Titer{src}; });
            const auto [merged, report] = ae::chart::v2::TitersModify::merge_titers(titers, ae::chart::v2::TitersModify::more_than_thresholded::adjust_to_next, standard_deviation_threshold);
            std::string message;
            if (report != ae::chart::v2::TitersModify::titer_merge::regular_only)
                message = ae::chart::v2::TitersModify::titer_merge_report_long(report);
            return std::make_pair(merged.get(), message);
        },
        "source_titers"_a, pybind11::doc("calculates merged titer, return pair: [merged_titer: str, message: str]"));
}

// ======================================================================
