#include "py/module.hh"
#include "chart/v3/chart.hh"
#include "chart/v3/selected-antigens-sera.hh"

// ======================================================================

namespace ae::py
{
    struct ProjectionRef
    {
        std::shared_ptr<ae::chart::v3::Chart> chart;
        ae::chart::v3::Projection& projection;

        ProjectionRef(std::shared_ptr<ae::chart::v3::Chart> a_chart, ae::chart::v3::Projection& a_projection) : chart{a_chart}, projection{a_projection} {}

        // ae::chart::v3::Projection& p() { return chart->projections()[projection_no]; }
        // const ae::chart::v3::Projection& p() const { return chart->projections()[projection_no]; }

        double stress() const { return projection.stress(); }
        std::string_view comment() const { return projection.comment(); }
        std::string minimum_column_basis() const { return projection.minimum_column_basis().format("{}", ae::chart::v3::minimum_column_basis::use_none::yes); }
        const std::vector<double>& forced_column_bases() const { return projection.forced_column_bases().data(); }
        std::vector<size_t> disconnected() const { return to_vector_base_t(projection.disconnected()); }
        std::vector<size_t> unmovable() const { return to_vector_base_t(projection.unmovable()); }
        std::vector<size_t> unmovable_in_the_last_dimension() const { return to_vector_base_t(projection.unmovable_in_the_last_dimension()); }
    };

    struct InfoRef
    {
        std::shared_ptr<ae::chart::v3::Chart> chart;
        ae::chart::v3::TableSource& table_source;
        std::vector<ae::chart::v3::TableSource>* sources;

        InfoRef(std::shared_ptr<ae::chart::v3::Chart> a_chart) : chart{a_chart}, table_source{a_chart->info()}, sources{&a_chart->info().sources()} {}
        InfoRef(std::shared_ptr<ae::chart::v3::Chart> a_chart, ae::chart::v3::TableSource& a_table_source) : chart{a_chart}, table_source{a_table_source}, sources{nullptr} {}

        std::string_view virus() const { return *table_source.virus(); }
        std::string_view type_subtype() const { return *table_source.type_subtype(); }
        std::string_view assay() const { return *table_source.assay(); }
        std::string_view rbc_species() const { return *table_source.rbc_species(); }
        std::string_view lab() const { return *table_source.lab(); }
        std::string_view date() const { return *table_source.date(); }
        std::string_view name() const { return table_source.name(); }

        size_t number_of_sources() const { return sources ? sources->size() : 0ul; }
        InfoRef* source(size_t no) {
            if (!sources || sources->empty())
                throw std::invalid_argument{"chart table has no sources"};
            else if (no >= sources->size())
                throw std::invalid_argument{fmt::format("invalid source_no {}, number of sources in the chart table: {}", no, sources->size())};
            return new InfoRef{chart, (*sources)[no]};
        }
    };

} // namespace ae::py

// ======================================================================

void ae::py::chart_v3(pybind11::module_& mdl)
{
    using namespace std::string_view_literals;
    using namespace pybind11::literals;
    using namespace ae::chart::v3;

    // ----------------------------------------------------------------------

    auto chart_v3_submodule = mdl.def_submodule("chart_v3", "chart_v3 api");

    pybind11::class_<Chart, std::shared_ptr<Chart>>(chart_v3_submodule, "Chart")                                       //
        .def(pybind11::init<>(), pybind11::doc("creates an empty chart"))                                              //
        .def(pybind11::init<const std::filesystem::path&>(), "filename"_a, pybind11::doc("imports chart from a file")) //
        .def(pybind11::init<const Chart&>(), "chart"_a, pybind11::doc("clone chart"))                                  //
        .def("write", &Chart::write, "filename"_a, pybind11::doc("exports chart into a file"))                         //

        .def("__str__", [](const Chart& chart) { return chart.name(); }) //
        .def(
            "name",
            [](const Chart& chart, std::optional<size_t> projection_no) {
                if (projection_no.has_value())
                    return chart.name(projection_index{*projection_no});
                else
                    return chart.name(std::nullopt);
            },
            "projection_no"_a = std::nullopt, pybind11::doc("short name of a chart"))                           //
        .def("name_for_file", &Chart::name_for_file, pybind11::doc("name of a chart to be used as a filename")) //
        .def("number_of_antigens", [](const Chart& chart) -> size_t { return *chart.antigens().size(); })       //
        .def("number_of_sera", [](const Chart& chart) -> size_t { return *chart.sera().size(); })               //
        .def("number_of_projections", [](const Chart& chart) -> size_t { return *chart.projections().size(); }) //
        .def("forced_column_bases", [](const Chart& chart) { return chart.forced_column_bases().data(); })      //

        .def("info", [](std::shared_ptr<Chart> chart) { return new InfoRef{chart}; }) //

        .def(
            "projection",
            [](std::shared_ptr<Chart> chart, size_t projection_no) {
                if (projection_index{projection_no} >= chart->projections().size())
                    throw std::invalid_argument{fmt::format("invalid projection_no {}, number of projections in chart: {}", projection_no, chart->projections().size())};
                return new ProjectionRef{chart, chart->projections()[projection_index{projection_no}]}; // owned by python program
            },
            "projection_no"_a = 0) //

        .def(
            "relax", //
            [](Chart& chart, size_t number_of_dimensions, size_t number_of_optimizations, std::string_view mcb, bool dimension_annealing, bool rough,
               size_t /*number_of_best_distinct_projections_to_keep*/, std::shared_ptr<SelectedAntigens> antigens_to_disconnect, std::shared_ptr<SelectedSera> sera_to_disconnect) {
                if (number_of_optimizations == 0)
                    number_of_optimizations = 100;
                optimization_options opt;
                opt.precision = rough ? optimization_precision::rough : optimization_precision::fine;
                opt.dimension_annealing = use_dimension_annealing_from_bool(dimension_annealing);
                ae::disconnected_points disconnect;
                if (antigens_to_disconnect && !antigens_to_disconnect->empty())
                    disconnect.insert_if_not_present(antigens_to_disconnect->points());
                if (sera_to_disconnect && !sera_to_disconnect->empty())
                    disconnect.insert_if_not_present(sera_to_disconnect->points());
                chart.relax(number_of_optimizations_t{number_of_optimizations}, minimum_column_basis{mcb}, number_of_dimensions_t{number_of_dimensions}, opt, disconnect);
                chart.projections().sort();
            },                                                                                                                                                    //
            "number_of_dimensions"_a = 2, "number_of_optimizations"_a = 0, "minimum_column_basis"_a = "none", "dimension_annealing"_a = false, "rough"_a = false, //
            "unused_number_of_best_distinct_projections_to_keep"_a = 5, "disconnect_antigens"_a = nullptr, "disconnect_sera"_a = nullptr,                         //
            pybind11::doc{"makes one or more antigenic maps from random starting layouts, adds new projections, projections are sorted by stress"})               //

        .def(
            "relax_incremental", //
            [](Chart& chart, size_t projection_no, size_t number_of_optimizations, bool rough, size_t /*number_of_best_distinct_projections_to_keep*/, bool remove_source_projection,
               bool unmovable_non_nan_points) {
                if (number_of_optimizations == 0)
                    number_of_optimizations = 100;
                optimization_options opt;
                opt.precision = rough ? optimization_precision::rough : optimization_precision::fine;
                opt.rsp = remove_source_projection ? ae::chart::v3::remove_source_projection::yes : ae::chart::v3::remove_source_projection::no;
                opt.unnp = unmovable_non_nan_points ? ae::chart::v3::unmovable_non_nan_points::yes : ae::chart::v3::unmovable_non_nan_points::no;
                chart.relax_incremental(projection_index{projection_no}, number_of_optimizations_t{number_of_optimizations}, opt);
                chart.projections().sort();
            }, //
            "projection_no"_a = 0, "number_of_optimizations"_a = 0, "rough"_a = false, "number_of_best_distinct_projections_to_keep"_a = 5, "remove_source_projection"_a = true,
            "unmovable_non_nan_points"_a = false) //

        //         .def("grid_test", &grid_test, "antigens"_a = nullptr, "sera"_a = nullptr, "projection_no"_a = 0, "grid_step"_a = 0.1, "threads"_a = 0) //

        // ----------------------------------------------------------------------

        .def(
            "select_antigens", //
            [](std::shared_ptr<Chart> chart, const std::function<bool(const SelectionData<Antigen>&)>& func, size_t projection_no) {
                return new SelectedAntigens{chart, func, projection_index{projection_no}};
            },                                    //
            "predicate"_a, "projection_no"_a = 0, //
            pybind11::doc("Passed predicate (function with one arg: SelectionDataAntigen object)\n"
                          "is called for each antigen, selects just antigens for which predicate\n"
                          "returns True, returns SelectedAntigens object.")) //
        .def(
            "select_all_antigens",                                                         //
            [](std::shared_ptr<Chart> chart) { return new SelectedAntigens{chart}; },      //
            pybind11::doc(R"(Selects all antigens and returns SelectedAntigens object.)")) //
        .def(
            "select_no_antigens", //
            [](std::shared_ptr<Chart> chart) {
                return new SelectedAntigens{chart, SelectedAntigens::None};
            },                                                                            //
            pybind11::doc(R"(Selects no antigens and returns SelectedAntigens object.)")) //

        // .def("antigens_by_aa_at_pos", &ae::py::antigens_sera_by_aa_at_pos<SelectedAntigens>, "pos"_a,
        //      pybind11::doc(R"(Returns dict with AA at passed pos as keys and SelectedAntigens as values.)")) //
        // .def("sera_by_aa_at_pos", &ae::py::antigens_sera_by_aa_at_pos<SelectedSera>, "pos"_a,
        //      pybind11::doc(R"(Returns dict with AA at passed pos as keys and SelectedSera as values.)")) //

        .def(
            "select_sera", //
            [](std::shared_ptr<Chart> chart, const std::function<bool(const SelectionData<Serum>&)>& func, size_t projection_no) {
                return new SelectedSera{chart, func, projection_index{projection_no}};
            },                                    //
            "predicate"_a, "projection_no"_a = 0, //
            pybind11::doc("Passed predicate (function with one arg: SelectionDataSerum object)\n"
                          "is called for each serum, selects just sera for which predicate\n"
                          "returns True, returns SelectedSera object.")) //
        .def(
            "select_all_sera",                                                     //
            [](std::shared_ptr<Chart> chart) { return new SelectedSera{chart}; },  //
            pybind11::doc(R"(Selects all sera and returns SelectedSera object.)")) //
        .def(
            "select_no_sera", //
            [](std::shared_ptr<Chart> chart) {
                return new SelectedSera{chart, SelectedSera::None};
            },                                                                    //
            pybind11::doc(R"(Selects no sera and returns SelectedSera object.)")) //

        // ----------------------------------------------------------------------

        //         .def(
        //             "table_as_text", //
        //             [](const Chart& chart, int layer_no, bool sort, bool clades, bool org_mode_separators, bool show_aa) {
        //                 const auto layer{layer_no >= 0 ? std::optional<size_t>{static_cast<size_t>(layer_no)} : std::nullopt};
        //                 const show_clades_t show_clades{clades ? show_clades_t::yes : show_clades_t::no};
        //                 const org_mode_separators_t org_mode_sep{org_mode_separators ? org_mode_separators_t::yes : org_mode_separators_t::no};
        //                 return ae::chart::v3::export_table_to_text(chart, layer, sort, show_clades, org_mode_sep, show_aa ? show_aa_t::yes : show_aa_t::no);
        //             },                                                                                                                                                            //
        //             "layer"_a = -1, "sort"_a = false, "show_clades"_a = false, "org_mode_separators"_a = false, "show_aa"_a = true,                                               //
        //             pybind11::doc("returns table as text\nif layer >= 0 shows corresponding layer\nif sort is True sort antigens/sera to be able to compare with another table")) //
        //         .def(
        //             "names_as_text",                                                                                                                  //
        //             [](std::shared_ptr<Chart> chart, const std::string& format) { return ae::chart::v3::export_names_to_text(chart, format); }, //
        //             "format"_a = "{ag_sr} {no0} {name_full}{ }{species}{ }{date_in_brackets}{ }{lab_ids}{ }{ref}\n",                                  //
        //             pybind11::doc("returns antigen and /serum names as text"))                                                                        //
        //         .def(
        //             "names_as_text", //
        //             [](const Chart& chart, const SelectedAntigensModify& antigens, const SelectedSeraModify& sera, const std::string& format) {
        //                 return ae::chart::v3::export_names_to_text(chart, format, antigens, sera);
        //             },                                                                                                                       //
        //             "antigens"_a, "sera"_a, "format"_a = "{ag_sr} {no0} {name_full}{ }{species}{ }{date_in_brackets}{ }{lab_ids}{ }{ref}\n", //
        //             pybind11::doc("returns antigen and /serum names as text for pre-selected antigens/sera"))                                //

        //         .def("subtype", [](const Chart& chart) { return *chart.info()->virus_type(); })                            //
        //         .def("subtype_short", [](const Chart& chart) { return std::string{chart.info()->virus_type().h_or_b()}; }) //
        //         .def("subset", [](const Chart& chart) { return chart.info()->subset(); })                                  //
        //         .def("assay", [](const Chart& chart) { return *chart.info()->assay(); })                                   //
        //         .def("assay_hi_or_neut", [](const Chart& chart) { return chart.info()->assay().hi_or_neut(); })            //
        //         .def("lab", [](const Chart& chart) { return *chart.info()->lab(); })                                       //
        //         .def("rbc", [](const Chart& chart) { return *chart.info()->rbc_species(); })                               //
        //         .def("assay_rbc",
        //              [](const Chart& chart) {
        //                  const auto assay = chart.info()->assay().short_name();
        //                  if (assay == "HI")
        //                      return fmt::format("HI-{}", *chart.info()->rbc_species());
        //                  else
        //                      return assay;
        //              })                                                                                        //
        //         .def("date", [](const Chart& chart) { return *chart.info()->date(Info::Compute::Yes); }) //
        //         .def(
        //             "lineage", [](const Chart& chart) -> std::string { return chart.lineage(); }, pybind11::doc("returns chart lineage: VICTORIA, YAMAGATA")) //
        //         .def("subtype_lineage",
        //              [](const Chart& chart) {
        //                  const auto subtype = chart.info()->virus_type().h_or_b();
        //                  if (subtype == "B")
        //                      return fmt::format("B{}", chart.lineage());
        //                  else
        //                      return std::string{subtype};
        //              }) //

        //         .def("description",                                       //
        //              &Chart::description,                                 //
        //              pybind11::doc("returns chart one line description")) //

        //         .def(
        //             "make_info", //
        //             [](const Chart& chart, size_t max_number_of_projections_to_show, bool column_bases, bool tables, bool tables_for_sera, bool antigen_dates) {
        //                 return chart.make_info(max_number_of_projections_to_show, make_info_data(column_bases, tables, tables_for_sera, antigen_dates));
        //             },                                                                                                                                               //
        //             "max_number_of_projections_to_show"_a = 20, "column_bases"_a = true, "tables"_a = false, "tables_for_sera"_a = false, "antigen_dates"_a = false, //
        //             pybind11::doc("returns detailed chart description"))                                                                                             //

        //         .def("number_of_antigens", &Chart::number_of_antigens)
        //         .def("number_of_sera", &Chart::number_of_sera)
        //         .def("number_of_projections", &Chart::number_of_projections)

        //         .def("populate_from_seqdb", &ae::py::populate_from_seqdb, "remove_old_sequences_clades"_a = false, pybind11::doc("populate with sequences from seqdb")) //

        //         .def(
        //             "projection",                                                                                    //
        //             [](Chart& chart, size_t projection_no) { return chart.projection_modify(projection_no); }, //
        //             "projection_no"_a = 0)                                                                           //

        .def("remove_all_projections",                                      //
             [](Chart& chart) { return chart.projections().remove_all(); }) //
                                                                            //         .def(
        //             "remove_all_projections_except",                                                                                    //
        //             [](Chart& chart, size_t to_keep) { return chart.projections_modify().remove_all_except(to_keep); }, "keep"_a) //
        .def(
            "remove_projection",                                                                                                       //
            [](Chart& chart, size_t to_remove) { return chart.projections().remove(projection_index{to_remove}); }, "projection_no"_a) //
        .def(
            "keep_projections",                                                                               //
            [](Chart& chart, size_t to_keep) { return chart.projections().keep(projection_index{to_keep}); }, //
            "keep"_a)                                                                                         //

        //         .def(
        //             "orient_to",
        //             [](Chart& chart, const Chart& master, size_t projection_no) {
        //                 ae::chart::v3::CommonAntigensSera common(master, chart, CommonAntigensSera::match_level_t::strict);
        //                 const auto procrustes_data = ae::chart::v3::procrustes(*master.projection(0), *chart.projection(projection_no), common.points(), ae::chart::v3::procrustes_scaling_t::no);
        //                 chart.projection_modify(projection_no)->transformation(procrustes_data.transformation);
        //             },                                 //
        //             "master"_a, "projection_no"_a = 0) //

        //         .def(
        //             "export", //
        //             [](Chart& chart, pybind11::object path, pybind11::object program_name) {
        //                 const std::string path_s = pybind11::str(path), pn_s = pybind11::str(program_name);
        //                 ae::chart::v3::export_factory(chart, path_s, pn_s);
        //             },                                            //
        //             "filename"_a, "program_name"_a = "acmacs-py") //

        //         .def("antigen", &Chart::antigen, "antigen_no"_a) //
        //         .def("serum", &Chart::serum, "serum_no"_a)       //

        //         // .def(
        //         //     "select_antigens_by_aa", //
        //         //     [](std::shared_ptr<Chart> chart, const std::vector<std::string>& criteria, bool report) {
        //         //         auto selected = std::make_shared<SelectedAntigensModify>(chart);
        //         //         ae::py::populate_from_seqdb(chart);
        //         //         ae::py::select_by_aa(selected->indexes, *chart->antigens(), criteria);
        //         //         AD_PRINT_L(report, [&selected]() { return selected->report("{ag_sr} {no0:{num_digits}d} {name_full_passage}\n"); });
        //         //         return selected;
        //         //     },                                                                                                        //
        //         //     "criteria"_a, "report"_a = false,                                                                         //
        //         //     pybind11::doc(R"(Criteria is a list of strings, e.g. ["156K", "!145K"], all criteria is the list must match)")) //
        //         // .def(
        //         //     "select_antigens_by_clade", //
        //         //     [](std::shared_ptr<Chart> chart, const std::vector<std::string>& clades, bool report) {
        //         //         auto selected = std::make_shared<SelectedAntigensModify>(chart);
        //         //         ae::py::populate_from_seqdb(chart);
        //         //         const auto pred = [&clades, antigens = chart->antigens()](auto index) { return antigens->at(index)->clades().exists_any_of(clades); };
        //         //         selected->indexes.get().erase(std::remove_if(selected->indexes.begin(), selected->indexes.end(), pred), selected->indexes.end());
        //         //         AD_PRINT_L(report, [&selected]() { return selected->report("{ag_sr} {no0:{num_digits}d} {name_full_passage}\n"); });
        //         //         return selected;
        //         //     },                                                                                                //
        //         //     "clades"_a, "report"_a = false,                                                                   //
        //         //     pybind11::doc(R"(Select antigens with a clade from clades, one or more entries in clades must match)")) //
        //         // .def(
        //         //     "select_sera_by_aa", //
        //         //     [](std::shared_ptr<Chart> chart, const std::vector<std::string>& criteria, bool report) {
        //         //         auto selected = std::make_shared<SelectedSeraModify>(chart);
        //         //         ae::py::populate_from_seqdb(chart);
        //         //         ae::py::select_by_aa(selected->indexes, *chart->sera(), criteria);
        //         //         AD_PRINT_L(report, [&selected]() { return selected->report("{ag_sr} {no0:{num_digits}d} {name_full_passage}\n"); });
        //         //         return selected;
        //         //     },                                                                                                         //
        //         //     "criteria"_a, "report"_a = false,                                                                          //
        //         //     pybind11::doc("Criteria is a list of strings, e.g. [\"156K\", \"!145K\"], all criteria is the list must match")) //
        //         // .def(
        //         //     "select_sera_by_clade", //
        //         //     [](std::shared_ptr<Chart> chart, const std::vector<std::string>& clades, bool report) {
        //         //         auto selected = std::make_shared<SelectedSeraModify>(chart);
        //         //         ae::py::populate_from_seqdb(chart);
        //         //         const auto pred = [&clades, sera = chart->sera()](auto index) { return sera->at(index)->clades().exists_any_of(clades); };
        //         //         selected->indexes.get().erase(std::remove_if(selected->indexes.begin(), selected->indexes.end(), pred), selected->indexes.end());
        //         //         AD_PRINT_L(report, [&selected]() { return selected->report("{ag_sr} {no0:{num_digits}d} {name_full_passage}\n"); });
        //         //         return selected;
        //         //     },                                                                                            //
        //         //     "clades"_a, "report"_a = false,                                                               //
        //         //     pybind11::doc(R"(Select sera with a clade from clades, one or more entries in clades must match)")) //

        //         .def("titers", &Chart::titers_modify_ptr, pybind11::doc("returns Titers oject"))

        //         .def("column_basis", &Chart::column_basis, "serum_no"_a, "projection_no"_a = 0, pybind11::doc("return column_basis for the passed serum"))
        //         .def(
        //             "column_bases", [](const Chart& chart, std::string_view minimum_column_basis) { return chart.column_bases(MinimumColumnBasis{minimum_column_basis})->data(); },
        //             "minimum_column_basis"_a, pybind11::doc("get column bases")) //
        //         .def(
        //             "column_bases", [](Chart& chart, const std::vector<double>& column_bases) { chart.forced_column_bases_modify(ColumnBasesData{column_bases}); }, "column_bases"_a,
        //             pybind11::doc("set forced column bases")) //

        //         .def("plot_spec", [](Chart& chart) { return PlotSpecRef{.plot_spec = chart.plot_spec_modify_ptr(), .number_of_antigens = chart.number_of_antigens()}; }) //

        //         .def("combine_projections", &Chart::combine_projections, "merge_in"_a) //

        //         .def(
        //             "remove_antigens_sera",
        //             [](Chart& chart, std::shared_ptr<SelectedAntigensModify> antigens, std::shared_ptr<SelectedSeraModify> sera, bool remove_projections) {
        //                 if (remove_projections)
        //                     chart.projections_modify().remove_all();
        //                 if (antigens && !antigens->empty())
        //                     chart.remove_antigens(ae::chart::v3::ReverseSortedIndexes{*antigens->indexes});
        //                 if (sera && !sera->empty())
        //                     chart.remove_sera(ae::chart::v3::ReverseSortedIndexes{*sera->indexes});
        //             },                                                                          //
        //             "antigens"_a = nullptr, "sera"_a = nullptr, "remove_projections"_a = false, //
        //             pybind11::doc(R"(
        // Usage:
        //     chart.remove_antigens_sera(antigens=chart.select_antigens(lambda ag: ag.lineage == "VICTORIA"), sera=chart.select_sera(lambda sr: sr.lineage == "VICTORIA"))
        // )"))                                                                                    //
        //         .def(
        //             "keep_antigens_sera",
        //             [](Chart& chart, std::shared_ptr<SelectedAntigensModify> antigens, std::shared_ptr<SelectedSeraModify> sera, bool remove_projections) {
        //                 if (remove_projections)
        //                     chart.projections_modify().remove_all();
        //                 if (antigens && !antigens->empty()) {
        //                     ae::chart::v3::ReverseSortedIndexes antigens_to_remove(chart.number_of_antigens());
        //                     antigens_to_remove.remove(*antigens->indexes);
        //                     // AD_INFO("antigens_to_remove:  {} {}", antigens_to_remove.size(), antigens_to_remove);
        //                     chart.remove_antigens(antigens_to_remove);
        //                 }
        //                 if (sera && !sera->empty()) {
        //                     ae::chart::v3::ReverseSortedIndexes sera_to_remove(chart.number_of_sera());
        //                     sera_to_remove.remove(*sera->indexes);
        //                     // AD_INFO("sera_to_remove:  {} {}", sera_to_remove.size(), sera_to_remove);
        //                     chart.remove_sera(sera_to_remove);
        //                 }
        //             },                                                                          //
        //             "antigens"_a = nullptr, "sera"_a = nullptr, "remove_projections"_a = false, //
        //             pybind11::doc(R"(
        // Usage:
        //     chart.remove_antigens_sera(antigens=chart.select_antigens(lambda ag: ag.lineage == "VICTORIA"), sera=chart.select_sera(lambda sr: sr.lineage == "VICTORIA"))
        // )"))                                                                                    //

        ;

    // ----------------------------------------------------------------------

    pybind11::class_<ProjectionRef>(chart_v3_submodule, "Projection")                            //
        .def("stress", &ProjectionRef::stress)                                                   //
        .def("comment", &ProjectionRef::comment)                                                 //
        .def("minimum_column_basis", &ProjectionRef::minimum_column_basis)                       //
        .def("forced_column_bases", &ProjectionRef::forced_column_bases)                         //
        .def("disconnected", &ProjectionRef::disconnected)                                       //
        .def("unmovable", &ProjectionRef::unmovable)                                             //
        .def("unmovable_in_the_last_dimension", &ProjectionRef::unmovable_in_the_last_dimension) //
        ;

    pybind11::class_<InfoRef>(chart_v3_submodule, "Info")      //
        .def("virus", &InfoRef::virus)                         //
        .def("type_subtype", &InfoRef::type_subtype)           //
        .def("assay", &InfoRef::assay)                         //
        .def("rbc_species", &InfoRef::rbc_species)             //
        .def("lab", &InfoRef::lab)                             //
        .def("date", &InfoRef::date)                           //
        .def("name", &InfoRef::name)                           //
        .def("number_of_sources", &InfoRef::number_of_sources) //
        .def("source", &InfoRef::source, "source_no"_a)        //
        ;

    // ----------------------------------------------------------------------

    pybind11::class_<AntigenSerum>(chart_v3_submodule, "AntigenSerum")                                                                           //
        .def("name", [](const AntigenSerum& ag) { return *ag.name(); })                                                                          //
        .def("name", [](AntigenSerum& ag, std::string_view new_name) { ag.name(virus::Name{new_name}); })                                        //
        .def("passage", [](const AntigenSerum& ag) { return ag.passage(); })                                                                     //
        .def("passage", [](AntigenSerum& ag, std::string_view new_passage) { ag.passage(ae::virus::Passage{new_passage}); })                     //
        .def("passage", [](AntigenSerum& ag, const ae::virus::Passage& new_passage) { ag.passage(new_passage); })                                //
        .def("reassortant", [](const AntigenSerum& ag) { return *ag.reassortant(); })                                                            //
        .def("reassortant", [](AntigenSerum& ag, std::string_view new_reassortant) { ag.reassortant(ae::virus::Reassortant{new_reassortant}); }) //
        .def("annotations", pybind11::overload_cast<>(&AntigenSerum::annotations, pybind11::const_))                                             //
        .def(
            "add_annotation", [](AntigenSerum& ag, std::string_view ann) { ag.annotations().add(ann); }, "annotation"_a) //
        .def(
            "remove_annotation", [](AntigenSerum& ag, const std::string& ann) { ag.annotations().remove(ann); }, "annotation_to_remove"_a) //

        .def("sequence_aa", [](AntigenSerum& ag) { return ag.aa(); })                                                          //
        .def("sequence_aa", [](AntigenSerum& ag, std::string_view sequence) { ag.aa(sequences::sequence_aa_t{sequence}); })    //
        .def("sequence_nuc", [](AntigenSerum& ag) { return ag.nuc(); })                                                        //
        .def("sequence_nuc", [](AntigenSerum& ag, std::string_view sequence) { ag.nuc(sequences::sequence_nuc_t{sequence}); }) //
        ;

    pybind11::class_<Antigen, AntigenSerum>(chart_v3_submodule, "Antigen")                      //
        .def("date", [](const Antigen& ag) { return *ag.date(); })                              //
        .def("date", [](Antigen& ag, const std::string& new_date) { ag.date(Date{new_date}); }) //
        .def("lab_ids", [](const Antigen& ag) { return *ag.lab_ids(); })                        //
        ;

    pybind11::class_<Serum, AntigenSerum>(chart_v3_submodule, "Serum")                                                               //
        .def("serum_id", [](const Serum& sr) { return *sr.serum_id(); })                                                             //
        .def("serum_id", [](Serum& sr, const std::string& new_serum_id) { return sr.serum_id(SerumId{new_serum_id}); })              //
        .def("serum_species", [](const Serum& sr) { return *sr.serum_species(); })                                                   //
        .def("serum_species", [](Serum& sr, const std::string& new_species) { return sr.serum_species(SerumSpecies{new_species}); }) //
        ;

    // ----------------------------------------------------------------------

    pybind11::class_<SelectedAntigens, std::shared_ptr<SelectedAntigens>>(chart_v3_submodule, "SelectedAntigens")
        // .def(
        //     "deselect_by_aa",
        //     [](SelectedAntigens& selected, const std::vector<std::string>& criteria) {
        //         ae::py::populate_from_seqdb(selected.chart);
        //         ae::py::deselect_by_aa(selected.indexes, *selected.chart->antigens(), criteria);
        //         return selected;
        //     },
        //     "criteria"_a, pybind11::doc("Criteria is a list of strings, e.g. [\"156K\", \"!145K\"], all criteria is the list must match")) //
        // .def(
        //     "exclude",
        //     [](SelectedAntigens& selected, const SelectedAntigens& exclude) {
        //         selected.exclude(exclude);
        //         return selected;
        //     },
        //     "exclude"_a, pybind11::doc("Deselect antigens selected by exclusion list")) //
        // .def(
        //     "filter_sequenced",
        //     [](SelectedAntigens& selected) {
        //         ae::py::populate_from_seqdb(selected.chart);
        //         ae::py::deselect_not_sequenced(selected.indexes, *selected.chart->antigens());
        //         return selected;
        //     },
        //     pybind11::doc("deselect not sequenced"))                                                                                   //
        // .def("report", &SelectedAntigens::report, "format"_a = "{no0},")                                                         //
        // .def("report_list", &SelectedAntigens::report_list, "format"_a = "{name}")                                               //
        // .def("__repr__", [](const SelectedAntigens& selected) { return fmt::format("SelectedAntigens ({})", selected.size()); }) //
        // .def("empty", &SelectedAntigens::empty)                                                                                  //
        // .def("size", &SelectedAntigens::size)                                                                                    //
        .def("__len__", &SelectedAntigens::size)                                                              //
        .def("__getitem__", &SelectedAntigens::operator[], pybind11::return_value_policy::reference_internal) //
        .def("__bool_", [](const SelectedAntigens& antigens) { return !antigens.empty(); })                   //
        .def(
            "__iter__", [](const SelectedAntigens& antigens) { return pybind11::make_iterator(antigens.begin(), antigens.end()); }, pybind11::keep_alive<0, 1>()) //
        // .def("indexes", [](const SelectedAntigens& selected) { return *selected.indexes; })                                                                 //
        // .def(
        //     "points", [](const SelectedAntigens& selected) { return *selected.points(); }, pybind11::doc("return point numbers")) //
        // .def(
        //     "area", [](const SelectedAntigens& selected, size_t projection_no) { return selected.area(projection_no); }, "projection_no"_a = 0,
        //     pybind11::doc("return boundaries of the selected points (not transformed)")) //
        // .def(
        //     "area_transformed", [](const SelectedAntigens& selected, size_t projection_no) { return selected.area_transformed(projection_no); }, "projection_no"_a = 0,
        //     pybind11::doc("return boundaries of the selected points (transformed)")) //
        // .def("for_each", &SelectedAntigens::for_each, "modifier"_a,
        //      pybind11::doc("modifier(ag_no, antigen) is called for each selected antigen, antigen fields, e.g. name, can be modified in the function.")) //
        ;

    pybind11::class_<SelectedSera, std::shared_ptr<SelectedSera>>(chart_v3_submodule, "SelectedSera")
        // .def(
        //     "deselect_by_aa",
        //     [](SelectedSera& selected, const std::vector<std::string>& criteria) {
        //         ae::py::populate_from_seqdb(selected.chart);
        //         ae::py::deselect_by_aa(selected.indexes, *selected.chart->sera(), criteria);
        //         return selected;
        //     },
        //     "criteria"_a, pybind11::doc("Criteria is a list of strings, e.g. [\"156K\", \"!145K\"], all criteria is the list must match")) //
        // .def(
        //     "exclude",
        //     [](SelectedSera& selected, const SelectedSera& exclude) {
        //         selected.exclude(exclude);
        //         return selected;
        //     },
        //     "exclude"_a, pybind11::doc("Deselect sera selected by exclusion list")) //
        // .def(
        //     "filter_sequenced",
        //     [](SelectedSera& selected) {
        //         ae::py::populate_from_seqdb(selected.chart);
        //         ae::py::deselect_not_sequenced(selected.indexes, *selected.chart->sera());
        //         return selected;
        //     },
        //     pybind11::doc("deselect not sequenced"))                                                                           //
        // .def("report", &SelectedSera::report, "format"_a = "{no0},")                                                     //
        // .def("report_list", &SelectedSera::report_list, "format"_a = "{name}")                                           //
        // .def("__repr__", [](const SelectedSera& selected) { return fmt::format("SelectedSera ({})", selected.size()); }) //
        // .def("empty", &SelectedSera::empty)                                                                              //
        // .def("size", &SelectedSera::size)                                                                                //
        .def("__len__", &SelectedSera::size)                                                              //
        .def("__getitem__", &SelectedSera::operator[], pybind11::return_value_policy::reference_internal) //
        .def("__bool_", [](const SelectedSera& sera) { return !sera.empty(); })                           //
        .def(
            "__iter__", [](const SelectedSera& sera) { return pybind11::make_iterator(sera.begin(), sera.end()); }, pybind11::keep_alive<0, 1>()) //
        // .def("indexes", [](const SelectedSera& selected) { return *selected.indexes; })                                                     //
        // .def("for_each", &SelectedSera::for_each, "modifier"_a,
        //      pybind11::doc("modifier(sr_no, serum) is called for each selected serum, serum fields, e.g. name, can be modified in the function.")) //
        ;

    pybind11::class_<SelectionData<Antigen>>(chart_v3_submodule, "SelectionData_Antigen")
        .def_property_readonly("no", [](const SelectionData<Antigen>& sd) -> size_t { return *sd.index; })
        .def_property_readonly("point_no", [](const SelectionData<Antigen>& sd) -> size_t { return *sd.index; })
        .def_property_readonly(
            "antigen", [](const SelectionData<Antigen>& sd) -> const Antigen& { return sd.ag_sr; }, pybind11::return_value_policy::reference_internal);

    pybind11::class_<SelectionData<Serum>>(chart_v3_submodule, "SelectionData_Serum")
        .def_property_readonly("no", [](const SelectionData<Serum>& sd) -> size_t { return *sd.index; })
        .def_property_readonly("point_no", [](const SelectionData<Serum>& sd) -> size_t { return *(sd.chart->antigens().size() + sd.index); })
        .def_property_readonly(
            "serum", [](const SelectionData<Serum>& sd) -> const Serum& { return sd.ag_sr; }, pybind11::return_value_policy::reference_internal);

    // ----------------------------------------------------------------------
}

// ----------------------------------------------------------------------
