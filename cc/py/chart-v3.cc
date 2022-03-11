#include "py/module.hh"
#include "chart/v3/chart.hh"

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
               size_t /*number_of_best_distinct_projections_to_keep*/
                      //, std::shared_ptr<SelectedAntigensModify> antigens_to_disconnect, std::shared_ptr<SelectedSeraModify> sera_to_disconnect
            ) {
                if (number_of_optimizations == 0)
                    number_of_optimizations = 100;
                optimization_options opt;
                opt.precision = rough ? optimization_precision::rough : optimization_precision::fine;
                opt.dimension_annealing = use_dimension_annealing_from_bool(dimension_annealing);
                ae::disconnected_points disconnect;
                // if (antigens_to_disconnect && !antigens_to_disconnect->empty())
                //     disconnect.extend(antigens_to_disconnect->points());
                // if (sera_to_disconnect && !sera_to_disconnect->empty())
                //     disconnect.extend(sera_to_disconnect->points());
                chart.relax(number_of_optimizations_t{number_of_optimizations}, minimum_column_basis{mcb}, number_of_dimensions_t{number_of_dimensions}, opt, disconnect);
                chart.projections().sort();
            },                                                                                                                                                    //
            "number_of_dimensions"_a = 2, "number_of_optimizations"_a = 0, "minimum_column_basis"_a = "none", "dimension_annealing"_a = false, "rough"_a = false, //
            "unused_number_of_best_distinct_projections_to_keep"_a = 5,
            // "disconnect_antigens"_a = nullptr, "disconnect_sera"_a = nullptr,                         //
            pybind11::doc{"makes one or more antigenic maps from random starting layouts, adds new projections, projections are sorted by stress"}) //

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

        //         .def(
        //             "table_as_text", //
        //             [](const ChartModify& chart, int layer_no, bool sort, bool clades, bool org_mode_separators, bool show_aa) {
        //                 const auto layer{layer_no >= 0 ? std::optional<size_t>{static_cast<size_t>(layer_no)} : std::nullopt};
        //                 const show_clades_t show_clades{clades ? show_clades_t::yes : show_clades_t::no};
        //                 const org_mode_separators_t org_mode_sep{org_mode_separators ? org_mode_separators_t::yes : org_mode_separators_t::no};
        //                 return ae::chart::v3::export_table_to_text(chart, layer, sort, show_clades, org_mode_sep, show_aa ? show_aa_t::yes : show_aa_t::no);
        //             },                                                                                                                                                            //
        //             "layer"_a = -1, "sort"_a = false, "show_clades"_a = false, "org_mode_separators"_a = false, "show_aa"_a = true,                                               //
        //             pybind11::doc("returns table as text\nif layer >= 0 shows corresponding layer\nif sort is True sort antigens/sera to be able to compare with another table")) //
        //         .def(
        //             "names_as_text",                                                                                                                  //
        //             [](std::shared_ptr<ChartModify> chart, const std::string& format) { return ae::chart::v3::export_names_to_text(chart, format); }, //
        //             "format"_a = "{ag_sr} {no0} {name_full}{ }{species}{ }{date_in_brackets}{ }{lab_ids}{ }{ref}\n",                                  //
        //             pybind11::doc("returns antigen and /serum names as text"))                                                                        //
        //         .def(
        //             "names_as_text", //
        //             [](const ChartModify& chart, const SelectedAntigensModify& antigens, const SelectedSeraModify& sera, const std::string& format) {
        //                 return ae::chart::v3::export_names_to_text(chart, format, antigens, sera);
        //             },                                                                                                                       //
        //             "antigens"_a, "sera"_a, "format"_a = "{ag_sr} {no0} {name_full}{ }{species}{ }{date_in_brackets}{ }{lab_ids}{ }{ref}\n", //
        //             pybind11::doc("returns antigen and /serum names as text for pre-selected antigens/sera"))                                //

        //         .def("subtype", [](const ChartModify& chart) { return *chart.info()->virus_type(); })                            //
        //         .def("subtype_short", [](const ChartModify& chart) { return std::string{chart.info()->virus_type().h_or_b()}; }) //
        //         .def("subset", [](const ChartModify& chart) { return chart.info()->subset(); })                                  //
        //         .def("assay", [](const ChartModify& chart) { return *chart.info()->assay(); })                                   //
        //         .def("assay_hi_or_neut", [](const ChartModify& chart) { return chart.info()->assay().hi_or_neut(); })            //
        //         .def("lab", [](const ChartModify& chart) { return *chart.info()->lab(); })                                       //
        //         .def("rbc", [](const ChartModify& chart) { return *chart.info()->rbc_species(); })                               //
        //         .def("assay_rbc",
        //              [](const ChartModify& chart) {
        //                  const auto assay = chart.info()->assay().short_name();
        //                  if (assay == "HI")
        //                      return fmt::format("HI-{}", *chart.info()->rbc_species());
        //                  else
        //                      return assay;
        //              })                                                                                        //
        //         .def("date", [](const ChartModify& chart) { return *chart.info()->date(Info::Compute::Yes); }) //
        //         .def(
        //             "lineage", [](const ChartModify& chart) -> std::string { return chart.lineage(); }, pybind11::doc("returns chart lineage: VICTORIA, YAMAGATA")) //
        //         .def("subtype_lineage",
        //              [](const ChartModify& chart) {
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
        //             [](const ChartModify& chart, size_t max_number_of_projections_to_show, bool column_bases, bool tables, bool tables_for_sera, bool antigen_dates) {
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
        //             [](ChartModify& chart, size_t projection_no) { return chart.projection_modify(projection_no); }, //
        //             "projection_no"_a = 0)                                                                           //

        //         .def("remove_all_projections",                                                   //
        //              [](ChartModify& chart) { return chart.projections_modify().remove_all(); }) //
        //         .def(
        //             "remove_all_projections_except",                                                                                    //
        //             [](ChartModify& chart, size_t to_keep) { return chart.projections_modify().remove_all_except(to_keep); }, "keep"_a) //
        //         .def(
        //             "remove_projection",                                                                                                  //
        //             [](ChartModify& chart, size_t to_remove) { return chart.projections_modify().remove(to_remove); }, "projection_no"_a) //
        //         .def(
        //             "keep_projections",                                                                               //
        //             [](ChartModify& chart, size_t to_keep) { return chart.projections_modify().keep_just(to_keep); }, //
        //             "keep"_a)                                                                                         //

        //         .def(
        //             "orient_to",
        //             [](ChartModify& chart, const ChartModify& master, size_t projection_no) {
        //                 ae::chart::v3::CommonAntigensSera common(master, chart, CommonAntigensSera::match_level_t::strict);
        //                 const auto procrustes_data = ae::chart::v3::procrustes(*master.projection(0), *chart.projection(projection_no), common.points(), ae::chart::v3::procrustes_scaling_t::no);
        //                 chart.projection_modify(projection_no)->transformation(procrustes_data.transformation);
        //             },                                 //
        //             "master"_a, "projection_no"_a = 0) //

        //         .def(
        //             "export", //
        //             [](ChartModify& chart, pybind11::object path, pybind11::object program_name) {
        //                 const std::string path_s = pybind11::str(path), pn_s = pybind11::str(program_name);
        //                 ae::chart::v3::export_factory(chart, path_s, pn_s);
        //             },                                            //
        //             "filename"_a, "program_name"_a = "acmacs-py") //

        //         .def("antigen", &ChartModify::antigen, "antigen_no"_a) //
        //         .def("serum", &ChartModify::serum, "serum_no"_a)       //

        //         .def(
        //             "select_antigens", //
        //             [](std::shared_ptr<ChartModify> chart, const std::function<bool(const SelectionData<Antigen>&)>& func, size_t projection_no, bool report) {
        //                 auto selected = std::make_shared<SelectedAntigensModify>(chart, func, projection_no);
        //                 AD_PRINT_L(report, [&selected]() { return selected->report("{ag_sr} {no0:{num_digits}d} {name_full_passage}\n"); });
        //                 return selected;
        //             },                                                        //
        //             "predicate"_a, "projection_no"_a = 0, "report"_a = false, //
        //             pybind11::doc("Passed predicate (function with one arg: SelectionDataAntigen object)\n"
        //                           "is called for each antigen, selects just antigens for which predicate\n"
        //                           "returns True, returns SelectedAntigens object.")) //
        //         // .def(
        //         //     "select_antigens_by_aa", //
        //         //     [](std::shared_ptr<ChartModify> chart, const std::vector<std::string>& criteria, bool report) {
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
        //         //     [](std::shared_ptr<ChartModify> chart, const std::vector<std::string>& clades, bool report) {
        //         //         auto selected = std::make_shared<SelectedAntigensModify>(chart);
        //         //         ae::py::populate_from_seqdb(chart);
        //         //         const auto pred = [&clades, antigens = chart->antigens()](auto index) { return antigens->at(index)->clades().exists_any_of(clades); };
        //         //         selected->indexes.get().erase(std::remove_if(selected->indexes.begin(), selected->indexes.end(), pred), selected->indexes.end());
        //         //         AD_PRINT_L(report, [&selected]() { return selected->report("{ag_sr} {no0:{num_digits}d} {name_full_passage}\n"); });
        //         //         return selected;
        //         //     },                                                                                                //
        //         //     "clades"_a, "report"_a = false,                                                                   //
        //         //     pybind11::doc(R"(Select antigens with a clade from clades, one or more entries in clades must match)")) //
        //         .def(
        //             "select_all_antigens",                                                                              //
        //             [](std::shared_ptr<ChartModify> chart) { return std::make_shared<SelectedAntigensModify>(chart); }, //
        //             pybind11::doc(R"(Selects all antigens and returns SelectedAntigens object.)"))                      //
        //         .def(
        //             "select_no_antigens",                                                                                                             //
        //             [](std::shared_ptr<ChartModify> chart) { return std::make_shared<SelectedAntigensModify>(chart, SelectedAntigensModify::None); }, //
        //             pybind11::doc(R"(Selects no antigens and returns SelectedAntigens object.)"))                                                     //

        //         .def("antigens_by_aa_at_pos", &ae::py::antigens_sera_by_aa_at_pos<SelectedAntigensModify>, "pos"_a,
        //              pybind11::doc(R"(Returns dict with AA at passed pos as keys and SelectedAntigens as values.)")) //
        //         .def("sera_by_aa_at_pos", &ae::py::antigens_sera_by_aa_at_pos<SelectedSeraModify>, "pos"_a,
        //              pybind11::doc(R"(Returns dict with AA at passed pos as keys and SelectedSera as values.)")) //

        //         .def(
        //             "select_sera", //
        //             [](std::shared_ptr<ChartModify> chart, const std::function<bool(const SelectionData<Serum>&)>& func, size_t projection_no, bool report) {
        //                 auto selected = std::make_shared<SelectedSeraModify>(chart, func, projection_no);
        //                 AD_PRINT_L(report, [&selected]() { return selected->report("{ag_sr} {no0:{num_digits}d} {name_full_passage}\n"); });
        //                 return selected;
        //             },                                                        //
        //             "predicate"_a, "projection_no"_a = 0, "report"_a = false, //
        //             pybind11::doc("Passed predicate (function with one arg: SelectionDataSerum object)\n"
        //                           "is called for each serum, selects just sera for which predicate\n"
        //                           "returns True, returns SelectedSera object.")) //
        //         // .def(
        //         //     "select_sera_by_aa", //
        //         //     [](std::shared_ptr<ChartModify> chart, const std::vector<std::string>& criteria, bool report) {
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
        //         //     [](std::shared_ptr<ChartModify> chart, const std::vector<std::string>& clades, bool report) {
        //         //         auto selected = std::make_shared<SelectedSeraModify>(chart);
        //         //         ae::py::populate_from_seqdb(chart);
        //         //         const auto pred = [&clades, sera = chart->sera()](auto index) { return sera->at(index)->clades().exists_any_of(clades); };
        //         //         selected->indexes.get().erase(std::remove_if(selected->indexes.begin(), selected->indexes.end(), pred), selected->indexes.end());
        //         //         AD_PRINT_L(report, [&selected]() { return selected->report("{ag_sr} {no0:{num_digits}d} {name_full_passage}\n"); });
        //         //         return selected;
        //         //     },                                                                                            //
        //         //     "clades"_a, "report"_a = false,                                                               //
        //         //     pybind11::doc(R"(Select sera with a clade from clades, one or more entries in clades must match)")) //
        //         .def(
        //             "select_all_sera",                                                                              //
        //             [](std::shared_ptr<ChartModify> chart) { return std::make_shared<SelectedSeraModify>(chart); }, //
        //             pybind11::doc(R"(Selects all sera and returns SelectedSera object.)"))                          //
        //         .def(
        //             "select_no_sera",                                                                                                         //
        //             [](std::shared_ptr<ChartModify> chart) { return std::make_shared<SelectedSeraModify>(chart, SelectedSeraModify::None); }, //
        //             pybind11::doc(R"(Selects no sera and returns SelectedSera object.)"))                                                     //

        //         .def("titers", &ChartModify::titers_modify_ptr, pybind11::doc("returns Titers oject"))

        //         .def("column_basis", &ChartModify::column_basis, "serum_no"_a, "projection_no"_a = 0, pybind11::doc("return column_basis for the passed serum"))
        //         .def(
        //             "column_bases", [](const ChartModify& chart, std::string_view minimum_column_basis) { return chart.column_bases(MinimumColumnBasis{minimum_column_basis})->data(); },
        //             "minimum_column_basis"_a, pybind11::doc("get column bases")) //
        //         .def(
        //             "column_bases", [](ChartModify& chart, const std::vector<double>& column_bases) { chart.forced_column_bases_modify(ColumnBasesData{column_bases}); }, "column_bases"_a,
        //             pybind11::doc("set forced column bases")) //

        //         .def("plot_spec", [](ChartModify& chart) { return PlotSpecRef{.plot_spec = chart.plot_spec_modify_ptr(), .number_of_antigens = chart.number_of_antigens()}; }) //

        //         .def("combine_projections", &ChartModify::combine_projections, "merge_in"_a) //

        //         .def(
        //             "remove_antigens_sera",
        //             [](ChartModify& chart, std::shared_ptr<SelectedAntigensModify> antigens, std::shared_ptr<SelectedSeraModify> sera, bool remove_projections) {
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
        //             [](ChartModify& chart, std::shared_ptr<SelectedAntigensModify> antigens, std::shared_ptr<SelectedSeraModify> sera, bool remove_projections) {
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

    pybind11::class_<ProjectionRef>(chart_v3_submodule, "Projection")      //
        .def("stress", &ProjectionRef::stress)                             //
        .def("comment", &ProjectionRef::comment)                           //
        .def("minimum_column_basis", &ProjectionRef::minimum_column_basis) //
        .def("forced_column_bases", &ProjectionRef::forced_column_bases)   //
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
}

// ----------------------------------------------------------------------
