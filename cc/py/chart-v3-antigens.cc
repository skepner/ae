#include "py/module.hh"
#include "chart/v3/selected-antigens-sera.hh"
#include "chart/v3/merge.hh"

// ----------------------------------------------------------------------

namespace ae::py
{
    using Chart = ae::chart::v3::Chart;

    static inline ae::chart::v3::antigens_sera_match_level_t antigens_sera_match_level(std::string_view match)
    {
        using namespace ae::chart::v3;
        if (match == "auto")
            return antigens_sera_match_level_t::automatic;
        else if (match == "strict")
            return antigens_sera_match_level_t::strict;
        else if (match == "relaxed")
            return antigens_sera_match_level_t::relaxed;
        else if (match == "ignored")
            return antigens_sera_match_level_t::ignored;
        else
            AD_WARNING("unrecognized merge match level \"{}\"", match);
        return antigens_sera_match_level_t::automatic;
    }

    // ----------------------------------------------------------------------

    static inline std::pair<std::shared_ptr<Chart>, ae::chart::v3::merge_data_t> merge(std::shared_ptr<Chart> chart1, std::shared_ptr<Chart> chart2, std::string_view match,
                                                                                       std::string_view merge_type, bool cca)
    {
        using namespace ae::chart::v3;
        merge_settings_t settings{
            .match_level = antigens_sera_match_level(match),
            .combine_cheating_assays_ = cca ? combine_cheating_assays::yes : combine_cheating_assays::no,
        };

        if (merge_type == "simple" || merge_type == "type1")
            settings.projection_merge = projection_merge_t::type1;
        else if (merge_type == "incremental" || merge_type == "type2")
            settings.projection_merge = projection_merge_t::type2;
        else if (merge_type == "overlay" || merge_type == "type3")
            settings.projection_merge = projection_merge_t::type3;
        else if (merge_type == "type4")
            settings.projection_merge = projection_merge_t::type4;
        else if (merge_type == "type5")
            settings.projection_merge = projection_merge_t::type5;
        else
            AD_WARNING("unrecognized merge type1 \"{}\"", merge_type);

        return ae::chart::v3::merge(chart1, chart2, settings);
    }

    // ----------------------------------------------------------------------

    template <typename Index> static inline std::vector<std::vector<size_t>> convert_common(const std::vector<std::pair<Index, Index>>& source)
    {
        std::vector<std::vector<size_t>> converted(source.size());
        std::transform(source.begin(), source.end(), converted.begin(), [](const auto& src) { return std::vector<size_t>{*src.first, *src.second}; });
        return converted;
    }

    static inline std::vector<std::vector<size_t>> common_antigens(const ae::chart::v3::common_antigens_sera_t& common)
    {
        return convert_common(common.antigens());
    }

    static inline std::vector<std::vector<size_t>> common_sera(const ae::chart::v3::common_antigens_sera_t& common)
    {
        return convert_common(common.sera());
    }

    static inline void common_antigens_sera_only(ae::chart::v3::common_antigens_sera_t& common, std::string_view only)
    {
        if (only == "antigens")
            common.antigens_only();
        else if (only == "sera")
            common.sera_only();
        else
            throw std::invalid_argument{R"(use "antigens" or "sera")"};
    }

}

// ----------------------------------------------------------------------

void ae::py::chart_v3_antigens(pybind11::module_& chart_v3_submodule)
{
    using namespace pybind11::literals;
    using namespace ae::chart::v3;

    pybind11::class_<AntigenSerum>(chart_v3_submodule, "AntigenSerum")                                                                           //
        .def("name", [](const AntigenSerum& ag) { return *ag.name(); })                                                                          //
        .def("name", [](AntigenSerum& ag, std::string_view new_name) { ag.name(virus::Name{new_name}); })                                        //
        .def("passage", [](const AntigenSerum& ag) { return ag.passage(); })                                                                     //
        .def("passage", [](AntigenSerum& ag, std::string_view new_passage) { ag.passage(ae::virus::Passage{new_passage}); })                     //
        .def("passage", [](AntigenSerum& ag, const ae::virus::Passage& new_passage) { ag.passage(new_passage); })                                //
        .def("reassortant", [](const AntigenSerum& ag) { return *ag.reassortant(); })                                                            //
        .def("reassortant", [](AntigenSerum& ag, std::string_view new_reassortant) { ag.reassortant(ae::virus::Reassortant{new_reassortant}); }) //
        .def("annotations", [](const AntigenSerum& ag) { return *ag.annotations(); })                                                            //
        .def(
            "add_annotation", [](AntigenSerum& ag, std::string_view ann) { ag.annotations().add(ann); }, "annotation"_a) //
        .def(
            "remove_annotation", [](AntigenSerum& ag, const std::string& ann) { ag.annotations().remove(ann); }, "annotation_to_remove"_a) //

        .def("sequence_aa", [](AntigenSerum& ag) { return ag.aa(); })                                                                                       //
        .def("sequence_aa", [](AntigenSerum& ag, std::string_view sequence) { ag.aa(sequences::sequence_aa_t{sequence}); })                                 //
        .def("sequence_nuc", [](AntigenSerum& ag) { return ag.nuc(); })                                                                                     //
        .def("sequence_nuc", [](AntigenSerum& ag, std::string_view sequence) { ag.nuc(sequences::sequence_nuc_t{sequence}); })                              //
        .def_property("semantic", pybind11::overload_cast<>(&AntigenSerum::semantic, pybind11::const_), pybind11::overload_cast<>(&AntigenSerum::semantic)) //
        ;

    pybind11::class_<Antigen, AntigenSerum>(chart_v3_submodule, "Antigen")                      //
        .def("date", [](const Antigen& ag) { return *ag.date(); })                              //
        .def("date", [](Antigen& ag, const std::string& new_date) { ag.date(Date{new_date}); }) //
        .def("lab_ids", [](const Antigen& ag) { return *ag.lab_ids(); })                        //
        .def("designation", &Antigen::designation)                                              //
        ;

    pybind11::class_<Serum, AntigenSerum>(chart_v3_submodule, "Serum")                                                               //
        .def("serum_id", [](const Serum& sr) { return *sr.serum_id(); })                                                             //
        .def("serum_id", [](Serum& sr, const std::string& new_serum_id) { return sr.serum_id(SerumId{new_serum_id}); })              //
        .def("serum_species", [](const Serum& sr) { return *sr.serum_species(); })                                                   //
        .def("serum_species", [](Serum& sr, const std::string& new_species) { return sr.serum_species(SerumSpecies{new_species}); }) //
        .def("designation", &Serum::designation)                                                                                     //
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
        .def("__repr__",
             [](const SelectedAntigens& selected) {
                 if (selected.size() == 0)
                     return std::string{"SelectedAntigens:none"};
                 else if (selected.size() < 20)
                     return fmt::format("SelectedAntigens ({}): {}", selected.size(), selected.indexes);
                 else
                     return fmt::format("SelectedAntigens ({})", selected.size());
             }) //
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
        .def("__repr__",
             [](const SelectedSera& selected) {
                 if (selected.size() == 0)
                     return std::string{"SelectedSera:none"};
                 else if (selected.size() < 20)
                     return fmt::format("SelectedSera ({}): {}", selected.size(), selected.indexes);
                 else
                     return fmt::format("SelectedSera ({})", selected.size());
             }) //
        ;

    pybind11::class_<SelectionData<Antigen>>(chart_v3_submodule, "SelectionData_Antigen")
        .def_property_readonly("no", [](const SelectionData<Antigen>& sd) -> size_t { return *sd.index; })
        .def_property_readonly("point_no", [](const SelectionData<Antigen>& sd) -> size_t { return *sd.index; })
        .def_property_readonly(
            "antigen", [](const SelectionData<Antigen>& sd) -> const Antigen& { return sd.ag_sr; }, pybind11::return_value_policy::reference_internal)             //
        .def_property_readonly("name", [](const SelectionData<Antigen>& sd) { return *sd.ag_sr.name(); })                                                          //
        .def_property_readonly("aa", [](const SelectionData<Antigen>& sd) { return sd.ag_sr.aa(); })                                                               //
        .def("has_clade", [](const SelectionData<Antigen>& sd, std::string_view clade) { return sd.ag_sr.semantic().has_clade(clade); })                           //
        .def("layers", [](const SelectionData<Antigen>& sd) -> std::vector<size_t> { return to_vector_base_t(sd.chart->titers().layers_with_antigen(sd.index)); }) //
        ;

    pybind11::class_<SelectionData<Serum>>(chart_v3_submodule, "SelectionData_Serum")
        .def_property_readonly("no", [](const SelectionData<Serum>& sd) -> size_t { return *sd.index; })
        .def_property_readonly("point_no", [](const SelectionData<Serum>& sd) -> size_t { return *(sd.chart->antigens().size() + sd.index); })
        .def_property_readonly(
            "serum", [](const SelectionData<Serum>& sd) -> const Serum& { return sd.ag_sr; }, pybind11::return_value_policy::reference_internal)               //
        .def_property_readonly("name", [](const SelectionData<Serum>& sd) { return *sd.ag_sr.name(); })                                                        //
        .def_property_readonly("aa", [](const SelectionData<Serum>& sd) { return sd.ag_sr.aa(); })                                                             //
        .def("has_clade", [](const SelectionData<Serum>& sd, std::string_view clade) { return sd.ag_sr.semantic().has_clade(clade); })                         //
        .def("layers", [](const SelectionData<Serum>& sd) -> std::vector<size_t> { return to_vector_base_t(sd.chart->titers().layers_with_serum(sd.index)); }) //
        ;

    // ----------------------------------------------------------------------

    pybind11::class_<SemanticAttributes>(chart_v3_submodule, "SemanticAttributes")
        .def("add_clade", &SemanticAttributes::add_clade, "clade"_a)                                       //
        .def("vaccine", pybind11::overload_cast<std::string_view>(&SemanticAttributes::vaccine), "year"_a) //
        ;

    // ----------------------------------------------------------------------

    pybind11::class_<common_antigens_sera_t>(chart_v3_submodule, "CommonAntigensSera") //
        .def(pybind11::init([](const Chart& chart1, const Chart& chart2, std::string_view match) {
                 return new common_antigens_sera_t{chart1, chart2, antigens_sera_match_level(match)};
             }),
             "chart1"_a, "chart2"_a, "match"_a = "auto", pybind11::doc(R"(match: "strict", "relaxed", "ignored", "auto")")) //
        .def("antigens_only", &common_antigens_sera_t::antigens_only)                                                       //
        .def("sera_only", &common_antigens_sera_t::sera_only)                                                               //
        .def("only", &common_antigens_sera_only, "only"_a, pybind11::doc(R"(only: "antigens", "sera")"))                    //
        .def("empty", &common_antigens_sera_t::empty)                                                                       //
        .def("number_of_antigens", &common_antigens_sera_t::common_antigens)                                                //
        .def("number_of_sera", &common_antigens_sera_t::common_sera)                                                        //
        .def("antigens", &common_antigens)                                                                                  //
        .def("sera", &common_sera)                                                                                          //
        .def("report", &common_antigens_sera_t::report, "indent"_a = 0)                                                     //
        ;
    chart_v3_submodule.def("merge", &ae::py::merge, "chart1"_a, "chart2"_a, "match"_a = "auto", "merge_type"_a = "simple", "combine_cheating_assays"_a = false);

    // ----------------------------------------------------------------------

    pybind11::class_<merge_data_t>(chart_v3_submodule, "MergeData")  //
        .def("common", &merge_data_t::common_report, "indent"_a = 0) //
        ;

    // ----------------------------------------------------------------------
}

// ----------------------------------------------------------------------
