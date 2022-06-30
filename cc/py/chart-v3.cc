#include "py/module.hh"
#include "py/chart-v3.hh"
#include "chart/v3/common.hh"
#include "chart/v3/selected-antigens-sera.hh"
#include "chart/v3/procrustes.hh"
#include "chart/v3/grid-test.hh"
#include "chart/v3/chart-seqdb.hh"
#include "pybind11/detail/common.h"

// ======================================================================

namespace ae::py
{
    using Chart = ae::chart::v3::Chart;

    struct InfoRef
    {
        std::shared_ptr<Chart> chart;
        ae::chart::v3::TableSource& table_source;
        std::vector<ae::chart::v3::TableSource>* sources;

        InfoRef(std::shared_ptr<Chart> a_chart) : chart{a_chart}, table_source{a_chart->info()}, sources{&a_chart->info().sources()} {}
        InfoRef(std::shared_ptr<Chart> a_chart, ae::chart::v3::TableSource& a_table_source) : chart{a_chart}, table_source{a_table_source}, sources{nullptr} {}
        InfoRef(const InfoRef&) = delete;
        InfoRef& operator=(const InfoRef&) = delete;

        std::string_view virus() const { return *table_source.virus(); }
        std::string_view type_subtype() const { return *table_source.type_subtype(); }
        std::string_view assay() const { return *table_source.assay(); }
        std::string assay_HI_or_Neut() const { return table_source.assay().HI_or_Neut(ae::chart::v3::Assay::no_hi::no); }
        std::string assay_hi_or_neut() const { return table_source.assay().hi_or_neut(ae::chart::v3::Assay::no_hi::no); }
        std::string assay_Neut() const { return table_source.assay().HI_or_Neut(ae::chart::v3::Assay::no_hi::yes); }
        std::string assay_neut() const { return table_source.assay().hi_or_neut(ae::chart::v3::Assay::no_hi::yes); }
        std::string_view rbc_species() const { return *table_source.rbc_species(); }
        std::string_view lab() const { return *table_source.lab(); }
        std::string_view date() const { return *table_source.date(); }
        std::string_view name() const { return table_source.name(); }
        std::string_view name_or_date() const { return table_source.name_or_date(); }

        size_t number_of_sources() const { return sources ? sources->size() : 0ul; }
        InfoRef* source(size_t no) {
            if (!sources || sources->empty())
                throw std::invalid_argument{"chart table has no sources"};
            else if (no >= sources->size())
                throw std::invalid_argument{fmt::format("invalid source_no {}, number of sources in the chart table: {}", no, sources->size())};
            return new InfoRef{chart, (*sources)[no]};
        }
    };

    // ----------------------------------------------------------------------

    struct TiterMergeReport
    {
        ae::chart::v3::Titers::titer_merge_report report;

        TiterMergeReport(ae::chart::v3::Titers::titer_merge_report&& a_report) : report{std::move(a_report)} {}

        std::string brief(size_t ag_no, size_t sr_no) const
        {
            if (const auto found = std::find_if(report.begin(), report.end(),
                                                [ag_no = antigen_index{ag_no}, sr_no = serum_index{sr_no}](const auto& entry) { return entry.antigen == ag_no && entry.serum == sr_no; });
                found != report.end())
                return value_brief(found->report);
            else
                return {};
        }

        static inline std::string value_brief(ae::chart::v3::Titers::titer_merge data)
        {
            using titer_merge = ae::chart::v3::Titers::titer_merge;
            switch (data) {
                case titer_merge::all_dontcare:
                    return "*";
                case titer_merge::less_and_more_than:
                    return "<>";
                case titer_merge::less_than_only:
                    return "<";
                case titer_merge::more_than_only_adjust_to_next:
                    return ">+1";
                case titer_merge::more_than_only_to_dontcare:
                    return ">*";
                case titer_merge::sd_too_big:
                    return "sd>";
                case titer_merge::regular_only:
                    return "+";
                case titer_merge::max_less_than_bigger_than_max_regular:
                    return "<<+";
                case titer_merge::less_than_and_regular:
                    return "<+";
                case titer_merge::min_more_than_less_than_min_regular:
                    return ">>+";
                case titer_merge::more_than_and_regular:
                    return ">+";
            }
            return "???";
        }

        static inline std::string value_long(ae::chart::v3::Titers::titer_merge data)
        {
            using titer_merge = ae::chart::v3::Titers::titer_merge;
            switch (data) {
                case titer_merge::all_dontcare:
                    return "all source titers are dont-care";
                case titer_merge::less_and_more_than:
                    return "both less-than and more-than present";
                case titer_merge::less_than_only:
                    return "less-than only";
                case titer_merge::more_than_only_adjust_to_next:
                    return "more-than only, adjust to next";
                case titer_merge::more_than_only_to_dontcare:
                    return "more-than only, convert to dont-care";
                case titer_merge::sd_too_big:
                    return "standard deviation is too big";
                case titer_merge::regular_only:
                    return "regular only";
                case titer_merge::max_less_than_bigger_than_max_regular:
                    return "max of less than is bigger than max of regulars";
                case titer_merge::less_than_and_regular:
                    return "less than and regular";
                case titer_merge::min_more_than_less_than_min_regular:
                    return "min of more-than is less than min of regular";
                case titer_merge::more_than_and_regular:
                    return "more than and regular";
            }
            return "(internal error)";
        }

        static inline std::string brief_description()
        {
            using titer_merge = ae::chart::v3::Titers::titer_merge;
            fmt::memory_buffer out;
            for (auto tm : {titer_merge::all_dontcare, titer_merge::less_and_more_than, titer_merge::less_than_only, titer_merge::more_than_only_adjust_to_next,
                            titer_merge::more_than_only_to_dontcare, titer_merge::sd_too_big, titer_merge::regular_only, titer_merge::max_less_than_bigger_than_max_regular,
                            titer_merge::less_than_and_regular, titer_merge::min_more_than_less_than_min_regular, titer_merge::more_than_and_regular}) {
                fmt::format_to(std::back_inserter(out), "{:>3s}  {}\n", value_brief(tm), value_long(tm));
            }
            return fmt::to_string(out);
        }
    };

    // ----------------------------------------------------------------------

    static inline ae::chart::v3::procrustes_data_t procrustes(const ProjectionRef& proj1, const ProjectionRef& proj2, const ae::chart::v3::common_antigens_sera_t& common, bool scaling)
    {
        return ae::chart::v3::procrustes(proj1.projection, proj2.projection, common, scaling ? ae::chart::v3::procrustes_scaling_t::yes : ae::chart::v3::procrustes_scaling_t::no);
    }

    static inline serum_indexes make_serum_indexes(const std::vector<size_t>& indexes)
    {
        serum_indexes sera;
        for (const auto sr_no : indexes)
            sera.push_back(serum_index{sr_no});
        return sera;
    }

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
        .def(
            "export", [](const Chart& chart) -> pybind11::bytes { return chart.export_to_json(); }, pybind11::doc("exports chart into json uncompressed, bytes")) //

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
        .def(
            "column_bases", [](const Chart& chart, std::string_view mcb) { return chart.column_bases(minimum_column_basis{mcb}).data(); }, "minimum_column_basis"_a = "none") //

        .def("info", [](std::shared_ptr<Chart> chart) { return new InfoRef{chart}; }) //

        .def("populate_from_seqdb", &populate_from_seqdb, pybind11::doc("populate with sequences from seqdb, returns number of antigens and number of sera that have sequences")) //
        .def("lineage", [](const Chart& chart) -> std::string_view { return chart.lineage(); })                                                                                   //
        .def(
            "subtype_lineage",
            [](const Chart& chart) -> std::string {
                if (const auto subtype = chart.info().type_subtype(); subtype.h_or_b() == "B")
                    return fmt::format("{}{}", subtype, chart.lineage());
                else
                    return std::string{*subtype};
            },
            pybind11::doc("returns A(H1N1), A(H3N2), BV, BY")) //

        // ----------------------------------------------------------------------

        .def(
            "projection",
            [](std::shared_ptr<Chart> chart, size_t projection_no) {
                if (projection_index{projection_no} >= chart->projections().size()) {
                    if (chart->projections().empty())
                        throw std::invalid_argument{"chart has no projections"};
                    else
                        throw std::invalid_argument{fmt::format("invalid projection no: {}, number of projections in chart: {}", projection_no, chart->projections().size())};
                }
                return new ProjectionRef{chart, chart->projections()[projection_index{projection_no}]}; // owned by python program
            },
            "projection_no"_a = 0) //

        .def("combine_projections", &Chart::combine_projections, "merge_in"_a) //

        // ----------------------------------------------------------------------

        .def("titers", pybind11::overload_cast<>(&Chart::titers), pybind11::return_value_policy::reference_internal) //

        .def("styles", pybind11::overload_cast<>(&Chart::styles), pybind11::return_value_policy::reference_internal) //
        // implement in kateri! .def("semantic_style_to_legacy", &Chart::semantic_style_to_legacy, "style_name"_a) //

        // ----------------------------------------------------------------------

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
                chart.projections().sort(chart);
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
                chart.projections().sort(chart);
            }, //
            "projection_no"_a = 0, "number_of_optimizations"_a = 0, "rough"_a = false, "number_of_best_distinct_projections_to_keep"_a = 5, "remove_source_projection"_a = true,
            "unmovable_non_nan_points"_a = false) //

        // ----------------------------------------------------------------------

        .def(
            "grid_test", [](Chart& chart, size_t projection_no) { return grid_test::test(chart, projection_index{projection_no}); }, "projection_no"_a = 0) //

        // ----------------------------------------------------------------------

        .def(
            "antigen", [](Chart& chart, size_t antigen_no) -> Antigen& { return chart.antigens()[antigen_index{antigen_no}]; }, "antigen_no"_a, pybind11::return_value_policy::reference_internal) //
        .def(
            "serum", [](Chart& chart, size_t serum_no) -> Serum& { return chart.sera()[serum_index{serum_no}]; }, "serum_no"_a, pybind11::return_value_policy::reference_internal) //

        .def(
            "antigen_date_range", [](const Chart& chart, bool test_only) { return chart.antigens().date_range(test_only, chart.reference()); }, "test_only"_a = true,
            pybind11::doc("returns pair of dates (str), date range is inclusive!")) //

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
            "select_all_antigens",                                                    //
            [](std::shared_ptr<Chart> chart) { return new SelectedAntigens{chart}; }, //
            pybind11::doc(R"(Select all antigens.)"))                                 //
        .def(
            "select_no_antigens", //
            [](std::shared_ptr<Chart> chart) {
                return new SelectedAntigens{chart, SelectedAntigens::None};
            },                                       //
            pybind11::doc(R"(Select no antigens.)")) //
        .def(
            "select_reference_antigens", //
            [](std::shared_ptr<Chart> chart) {
                return new SelectedAntigens{chart, chart->reference()};
            },                                              //
            pybind11::doc(R"(Select reference antigens.)")) //
        .def(
            "select_test_antigens", //
            [](std::shared_ptr<Chart> chart) {
                return new SelectedAntigens{chart, chart->test()};
            },                                         //
            pybind11::doc(R"(Select test antigens.)")) //
        .def(
            "select_new_antigens", //
            [](std::shared_ptr<Chart> chart, const Chart& previous_chart) {
                auto* selected = new SelectedAntigens{chart};
                selected->filter_new(previous_chart.antigens());
                return selected;
            },
            "previous_chart"_a,                                                   //
            pybind11::doc(R"(Select antigens not found in the previous_chart.)")) //

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
            "select_all_sera",                                                    //
            [](std::shared_ptr<Chart> chart) { return new SelectedSera{chart}; }, //
            pybind11::doc(R"(Select all sera.)"))                                 //
        .def(
            "select_no_sera", //
            [](std::shared_ptr<Chart> chart) {
                return new SelectedSera{chart, SelectedSera::None};
            },                                   //
            pybind11::doc(R"(Select no sera.)")) //

        .def("duplicates_distinct", &Chart::duplicates_distinct, pybind11::doc("make duplicating antigens/sera distinct")) //

        // ----------------------------------------------------------------------

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

        .def(
            "orient_to",
            [](Chart& chart, const Chart& master, size_t projection_no) {
                const auto procrustes_data = ae::chart::v3::procrustes(master.projections()[projection_index{0}], chart.projections()[projection_index{projection_no}],
                                                                       common_antigens_sera_t{master, chart, antigens_sera_match_level_t::automatic}, ae::chart::v3::procrustes_scaling_t::no);
                chart.projections()[projection_index{projection_no}].transformation() = procrustes_data.transformation;
            },                                 //
            "master"_a, "projection_no"_a = 0) //

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

        //         .def("column_basis", &Chart::column_basis, "serum_no"_a, "projection_no"_a = 0, pybind11::doc("return column_basis for the passed serum"))
        //         .def(
        //             "column_bases", [](const Chart& chart, std::string_view minimum_column_basis) { return chart.column_bases(MinimumColumnBasis{minimum_column_basis})->data(); },
        //             "minimum_column_basis"_a, pybind11::doc("get column bases")) //
        //         .def(
        //             "column_bases", [](Chart& chart, const std::vector<double>& column_bases) { chart.forced_column_bases_modify(ColumnBasesData{column_bases}); }, "column_bases"_a,
        //             pybind11::doc("set forced column bases")) //

        //         .def("plot_spec", [](Chart& chart) { return PlotSpecRef{.plot_spec = chart.plot_spec_modify_ptr(), .number_of_antigens = chart.number_of_antigens()}; }) //

        .def(
            "remove_antigens_sera",
            [](Chart& chart, std::shared_ptr<SelectedAntigens> antigens, std::shared_ptr<SelectedSera> sera, bool remove_projections) {
                if (remove_projections)
                    chart.projections().remove_all();
                if (antigens && !antigens->empty())
                    chart.remove_antigens(*antigens);
                if (sera && !sera->empty())
                    chart.remove_sera(*sera);
            },                                                                          //
            "antigens"_a = nullptr, "sera"_a = nullptr, "remove_projections"_a = false, //
            pybind11::doc(R"(
        Usage:
            chart.remove_antigens_sera(antigens=chart.select_antigens(lambda ag: ag.lineage == "VICTORIA"), sera=chart.select_sera(lambda sr: sr.lineage == "VICTORIA"))
        )"))                                                                            //
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

    pybind11::class_<Titers>(chart_v3_submodule, "Titers")                                        //
        .def("number_of_layers", [](const Titers& titers) { return *titers.number_of_layers(); }) //
        .def(
            "titer", [](const Titers& titers, size_t ag_no, size_t sr_no) { return titers.titer(antigen_index{ag_no}, serum_index{sr_no}); }, "antigen_no"_a, "serum_no"_a) //
        .def(
            "set_titer", [](Titers& titers, size_t ag_no, size_t sr_no, std::string_view new_titer) { return titers.set_titer(antigen_index{ag_no}, serum_index{sr_no}, Titer{new_titer}); }, "antigen_no"_a, "serum_no"_a, "titer"_a) //
        .def(
            "titer_of_layer", [](const Titers& titers, size_t layer_no, size_t ag_no, size_t sr_no) { return titers.titer_of_layer(layer_index{layer_no}, antigen_index{ag_no}, serum_index{sr_no}); },
            "layer_no"_a, "antigen_no"_a, "serum_no"_a)                                                                            //
        .def("set_from_layers_report", [](const Titers& titers) { return new TiterMergeReport{titers.set_from_layers_report()}; }) //
        .def(
            "titrations_for_antigen", [](const Titers& titers, size_t antigen_no) { return titers.titrations_for_antigen(antigen_index{antigen_no}); }, "antigen_no"_a) //
        .def(
            "titrations_for_serum", [](const Titers& titers, size_t serum_no) { return titers.titrations_for_serum(serum_index{serum_no}); }, "serum_no"_a) //
        .def(
            "layers_with_antigen", [](const Titers& titers, size_t antigen_no) { return to_vector_base_t(titers.layers_with_antigen(antigen_index{antigen_no})); }, "antigen_no"_a) //
        .def(
            "layers_with_serum", [](const Titers& titers, size_t serum_no) { return to_vector_base_t(titers.layers_with_serum(serum_index{serum_no})); }, "serum_no"_a) //
        .def(
            "serum_coverage",
            [](const Titers& titers, size_t antigen_no, size_t serum_no, double fold) { return serum_coverage(titers, antigen_index{antigen_no}, serum_index{serum_no}, serum_circle_fold{fold}); },
            "antigen_no"_a, "serum_no"_a, "fold"_a = 2.0) //
        ;

    pybind11::class_<Titer>(chart_v3_submodule, "Titer")                                                                      //
        .def("__str__", pybind11::overload_cast<>(&Titer::get, pybind11::const_))                                             //
        .def("logged", &Titer::logged)                                                                                        //
        .def("logged_with_thresholded", &Titer::logged_with_thresholded)                                                      //
        .def("value", &Titer::value)                                                                                          //
        .def("value_with_thresholded", &Titer::value_with_thresholded, pybind11::doc("returns 20 for <40, 20480 for >10240")) //
        .def("is_dont_care", &Titer::is_dont_care)                                                                            //
        .def("is_regular", &Titer::is_regular)                                                                                //
        .def("is_less_than", &Titer::is_less_than)                                                                            //
        .def("is_more_than", &Titer::is_more_than)                                                                            //
        ;

    pybind11::class_<TiterMergeReport>(chart_v3_submodule, "TiterMergeReport") //
        .def("brief", &TiterMergeReport::brief, "antigen_no"_a, "serum_no"_a)  //
        .def_static("brief_description", &TiterMergeReport::brief_description) //
        ;

    // ----------------------------------------------------------------------

    pybind11::class_<ProjectionRef>(chart_v3_submodule, "Projection")                                                                                         //
        .def("stress", &ProjectionRef::stress)                                                                                                                //
        .def("recalculate_stress", &ProjectionRef::recalculate_stress)                                                                                        //
        .def("comment", &ProjectionRef::comment)                                                                                                              //
        .def("minimum_column_basis", &ProjectionRef::minimum_column_basis)                                                                                    //
        .def("forced_column_bases", &ProjectionRef::forced_column_bases)                                                                                      //
        .def("disconnected", &ProjectionRef::disconnected)                                                                                                    //
        .def("unmovable", &ProjectionRef::unmovable)                                                                                                          //
        .def("unmovable_in_the_last_dimension", &ProjectionRef::unmovable_in_the_last_dimension)                                                              //
        .def("connect_all_disconnected", &ProjectionRef::connect_all_disconnected, pybind11::doc("reconnected points still have NaN coordinates after call")) //
        .def("layout", &ProjectionRef::layout, pybind11::return_value_policy::reference_internal)                                                             //
        .def("transformation", &ProjectionRef::transformation, pybind11::return_value_policy::reference_internal)                                             //
        .def(
            "relax", [](ProjectionRef& projection, bool rough) { return projection.relax(rough ? optimization_precision::rough : optimization_precision::fine); }, "rough"_a = false) //
        .def("avidity_test", &ProjectionRef::avidity_test, "adjust_step"_a, "min_adjust"_a, "max_adjust"_a, "rough"_a)                                                                //
        .def("serum_circles", &ProjectionRef::serum_circles, "fold"_a = 2.0)                                                                                                          //
        .def(
            "serum_circle_for_multiple_sera",
            [](const ProjectionRef& projection, const std::vector<size_t>& serum_no, double fold, bool conservative) {
                return projection.serum_circle_for_multiple_sera(make_serum_indexes(serum_no), fold, conservative);
            },
            "sera"_a, "fold"_a = 2.0, "conservative"_a = false, pybind11::doc("description is in serum_circles.cc")) //
        ;

    pybind11::class_<Layout>(chart_v3_submodule, "Layout")                                                          //
        .def("__len__", [](const Layout& layout) -> size_t { return *layout.number_of_points(); })                  //
        .def("number_of_dimensions", [](const Layout& layout) -> size_t { return *layout.number_of_dimensions(); }) //
        .def(
            "__getitem__",
            [](Layout& layout, ssize_t index) {
                if (index >= 0 && point_index{index} < layout.number_of_points())
                    return layout[point_index{index}];
                else if (index < 0 && point_index{-index} <= layout.number_of_points())
                    return layout[layout.number_of_points() + index];
                else
                    throw std::invalid_argument{fmt::format("wrong index: {}, number of points in layout: {}", index, layout.number_of_points())};
            },
            "index"_a, pybind11::doc("negative index counts from the layout end")) //
        .def(
            "__iter__", [](const Layout& layout) { return pybind11::make_iterator(layout.begin(), layout.end()); }, pybind11::keep_alive<0, 1>()) //
        .def("minmax", &Layout::minmax)                                                                                                           //
        .def(
            "connected", [](const Layout& layout, size_t index) { return layout.point_has_coordinates(point_index{index}); }, "index"_a) //
        // .def("__str__", [](const Layout& layout) { return fmt::format("{}", layout); }) //
        ;

    pybind11::class_<point_coordinates>(chart_v3_submodule, "PointCoordinates")                           //
        .def("__len__", [](const point_coordinates& pc) -> size_t { return *pc.number_of_dimensions(); }) //
        .def(
            "__getitem__", [](point_coordinates& pc, size_t dim_no) -> double& { return pc[number_of_dimensions_t{dim_no}]; }, "dim"_a) //
        .def(
            "__iter__", [](const point_coordinates& pc) { return pybind11::make_iterator(pc.begin(), pc.end()); }, pybind11::keep_alive<0, 1>()) //
        .def("__str__", [](const point_coordinates& pc) { return fmt::format("{}", pc); })                                                       //
        ;

    // pybind11::class_<point_coordinates_ref>(chart_v3_submodule, "PointCoordinatesInLayout")                   //
    //     .def("__len__", [](const point_coordinates_ref& pc) -> size_t { return *pc.number_of_dimensions(); }) //
    //     .def(
    //         "__getitem__", [](const point_coordinates_ref& pc, size_t dim_no) -> double { return pc[number_of_dimensions_t{dim_no}]; }, "dim"_a) //
    //     .def(
    //         "__iter__", [](const point_coordinates_ref& pc) { return pybind11::make_iterator(pc.begin(), pc.end()); }, pybind11::keep_alive<0, 1>()) //
    //     .def("__str__", [](const point_coordinates_ref& pc) { return fmt::format("{}", pc); })                                                       //
    //     ;

    pybind11::class_<point_coordinates_ref_const>(chart_v3_submodule, "PointCoordinatesInLayout")                   //
        .def("__len__", [](const point_coordinates_ref_const& pc) -> size_t { return *pc.number_of_dimensions(); }) //
        .def(
            "__getitem__", [](const point_coordinates_ref_const& pc, size_t dim_no) -> double { return pc[number_of_dimensions_t{dim_no}]; }, "dim"_a) //
        .def(
            "__iter__", [](const point_coordinates_ref_const& pc) { return pybind11::make_iterator(pc.begin(), pc.end()); }, pybind11::keep_alive<0, 1>()) //
        .def("__str__", [](const point_coordinates_ref_const& pc) { return fmt::format("{}", pc); })                                                       //
        ;

    pybind11::class_<Transformation>(chart_v3_submodule, "Transformation")                                      //
        .def("__str__", [](const Transformation& transformation) { return fmt::format("{}", transformation); }) //
        .def(
            "rotate", [](Transformation& transformation, double angle) { transformation.rotate(ae::draw::v2::rotation(angle)); }, "angle"_a,
            pybind11::doc("angle < 3.15 - radians, angle >= 3.15 - degrees"))                  //
        .def("flip_ew", [](Transformation& transformation) { transformation.flip(0.0, 1.0); }) //
        .def("flip_ns", [](Transformation& transformation) { transformation.flip(1.0, 0.0); }) //
        ;

    // ----------------------------------------------------------------------

    pybind11::class_<InfoRef>(chart_v3_submodule, "Info")      //
        .def("virus", &InfoRef::virus)                         //
        .def("type_subtype", &InfoRef::type_subtype)           //
        .def("assay", &InfoRef::assay)                         //
        .def("assay_HI_or_Neut", &InfoRef::assay_HI_or_Neut)   //
        .def("assay_hi_or_neut", &InfoRef::assay_hi_or_neut)   //
        .def("assay_Neut", &InfoRef::assay_Neut)               //
        .def("assay_neut", &InfoRef::assay_neut)               //
        .def("rbc_species", &InfoRef::rbc_species)             //
        .def("lab", &InfoRef::lab)                             //
        .def("date", &InfoRef::date)                           //
        .def("name", &InfoRef::name)                           //
        .def("name_or_date", &InfoRef::name_or_date)           //
        .def("number_of_sources", &InfoRef::number_of_sources) //
        .def("source", &InfoRef::source, "source_no"_a)        //
        ;

    // ----------------------------------------------------------------------

    chart_v3_antigens(chart_v3_submodule);
    chart_v3_plot_spec(chart_v3_submodule);
    chart_v3_tests(chart_v3_submodule);
    chart_v3_submodule.def("procrustes", &ae::py::procrustes, "chart1"_a, "chart2"_a, "common"_a, "scaling"_a = false);

    // ----------------------------------------------------------------------
}

// ----------------------------------------------------------------------
