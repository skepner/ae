#include "py/module.hh"
#include "py/chart-v3.hh"
#include "chart/v3/procrustes.hh"
#include "chart/v3/grid-test.hh"

// ----------------------------------------------------------------------

namespace ae::py
{
    static inline const char* grid_test_diagnosis(const ae::chart::v3::grid_test::result_t& res)
    {
        using namespace ae::chart::v3::grid_test;
        switch (res.diagnosis) {
            case result_t::excluded:
                return "excluded";
            case result_t::not_tested:
                return "not_tested";
            case result_t::normal:
                return "normal";
            case result_t::trapped:
                return "trapped";
            case result_t::hemisphering:
                return "hemisphering";
        }
        return "not_tested";
    }

    static inline std::string grid_test_result_str(const ae::chart::v3::grid_test::result_t& res)
    {
        return fmt::format("{:>12s}: {:4d} dist:{:7.4f} diff:{:7.4f}", grid_test_diagnosis(res), *res.point_no, res.distance, res.contribution_diff);
    }

}

// ----------------------------------------------------------------------

void ae::py::chart_v3_tests(pybind11::module_& chart_v3_submodule)
{
    using namespace pybind11::literals;
    using namespace ae::chart::v3;

    pybind11::class_<procrustes_data_t>(chart_v3_submodule, "ProcrustesData")                                             //
        .def("transformation", [](const procrustes_data_t& data) -> Transformation { return data.transformation; })       //
        .def("rms", [](const procrustes_data_t& data) { return data.rms; })                                               //
        .def("apply", &procrustes_data_t::apply, "layout"_a)                                                              //
        .def("scale", [](const procrustes_data_t& data) { return data.scale; })                                           //
        .def("secondary_transformed", [](const procrustes_data_t& data) -> Layout { return data.secondary_transformed; }) //
        ;

    // ----------------------------------------------------------------------

    pybind11::class_<grid_test::results_t>(chart_v3_submodule, "GridTestResults")             //
        .def("trapped_hemisphering", &grid_test::results_t::trapped_hemisphering)             //
        .def("count_trapped_hemisphering", &grid_test::results_t::count_trapped_hemisphering) //
        .def("count_trapped", &grid_test::results_t::count_trapped)                           //
        .def(
            "apply", [](const grid_test::results_t& results, ProjectionRef& projection_ref) { results.apply(projection_ref.projection); }, "projection"_a,
            pybind11::doc("move points to their better locations")) //
        ;

    pybind11::class_<grid_test::result_t>(chart_v3_submodule, "GridTestResult")                          //
        .def_property_readonly("point_no", [](const grid_test::result_t& res) { return *res.point_no; }) //
        .def_readonly("pos", &grid_test::result_t::pos)                                                  //
        .def_readonly("distance", &grid_test::result_t::distance)                                        //
        .def_readonly("contribution_diff", &grid_test::result_t::contribution_diff)                      //
        .def_property_readonly("diagnosis", &grid_test_diagnosis)                                        //
        .def("__str__", &grid_test_result_str)                                                           //
        ;

    // ----------------------------------------------------------------------

    pybind11::class_<avidity_test::results_t>(chart_v3_submodule, "AvidityTestResults") //
        .def(
            "__getitem__", [](const avidity_test::results_t& results, size_t antigen_no) { return results.get(antigen_index{antigen_no}); }, "antigen_no"_a,
            pybind11::return_value_policy::reference_internal) //
        .def(
            "__iter__", [](const avidity_test::results_t& results) { return pybind11::make_iterator(results.begin(), results.end()); }, pybind11::return_value_policy::reference_internal) //
        ;

    pybind11::class_<avidity_test::result_t>(chart_v3_submodule, "AvidityTestResult")                                 //
        .def_property_readonly("antigen_no", [](const avidity_test::result_t& result) { return *result.antigen_no; }) //
        .def_readonly("best_logged_adjust", &avidity_test::result_t::best_logged_adjust)                              //
        .def(
            "best_adjust", [](const avidity_test::result_t& result) { return result.best_adjust(); }, pybind11::return_value_policy::reference_internal) //
        .def(
            "__getitem__", [](const avidity_test::result_t& result, size_t index) { return result.adjusts[index]; }, "index"_a, pybind11::keep_alive<0, 1>()) //
        .def(
            "__iter__", [](const avidity_test::result_t& result) { return pybind11::make_iterator(result.begin(), result.end()); }, pybind11::return_value_policy::reference_internal) //
        ;

    pybind11::class_<avidity_test::per_adjust_t>(chart_v3_submodule, "AvidityTestResult_PerAdjust")                                                      //
        .def_readonly("logged_adjust", &avidity_test::per_adjust_t::logged_adjust)                                                                       //
        .def_readonly("distance_test_antigen", &avidity_test::per_adjust_t::distance_test_antigen)                                                       //
        .def_readonly("angle_test_antigen", &avidity_test::per_adjust_t::angle_test_antigen)                                                             //
        .def_readonly("average_procrustes_distances_except_test_antigen", &avidity_test::per_adjust_t::average_procrustes_distances_except_test_antigen) //
        .def_readonly("stress_diff", &avidity_test::per_adjust_t::stress_diff)                                                                           //
        .def_readonly("final_coordinates", &avidity_test::per_adjust_t::final_coordinates)                                                               //
        .def(
            "__getitem__",
            [](const avidity_test::per_adjust_t& result, size_t index) -> std::pair<size_t, double> {
                return {*result.most_moved[index].antigen_no, result.most_moved[index].distance};
            },
            "most_moved_index"_a, pybind11::return_value_policy::reference_internal) //
        ;

    // ----------------------------------------------------------------------

    pybind11::class_<serum_circles_for_serum_t>(chart_v3_submodule, "SerumCircleForSerum")                                                          //
        .def_property_readonly("serum_no", [](const serum_circles_for_serum_t& cs) { return *cs.serum_no; })                                        //
        .def("theoretical", &serum_circles_for_serum_t::theoretical)                                                                                //
        .def("empirical", &serum_circles_for_serum_t::empirical)                                                                                    //
        .def_readonly("column_basis", &serum_circles_for_serum_t::column_basis)                                                                     //
        .def_property_readonly("fold", [](const serum_circles_for_serum_t& cs) { return *cs.fold; })                                                //
        .def("valid", &serum_circles_for_serum_t::valid)                                                                                            //
        .def("number_of_homologous_antigens", [](const serum_circles_for_serum_t& circles_for_serum) { return circles_for_serum.antigens.size(); }) //
        .def(
            "__iter__", [](const serum_circles_for_serum_t& circles_for_serum) { return pybind11::make_iterator(circles_for_serum.antigens.begin(), circles_for_serum.antigens.end()); },
            pybind11::return_value_policy::reference_internal) //
        ;

    pybind11::class_<serum_circle_antigen_t>(chart_v3_submodule, "SerumCircleForAntigen")                     //
        .def_property_readonly("antigen_no", [](const serum_circle_antigen_t& cs) { return *cs.antigen_no; }) //
        .def_readonly("titer", &serum_circle_antigen_t::titer)                                                //
        .def_readonly("theoretical", &serum_circle_antigen_t::theoretical)                                    //
        .def_readonly("empirical", &serum_circle_antigen_t::empirical)                                        //
        .def("status",
             [](const serum_circle_antigen_t& cs) {
                 switch (cs.status) {
                     case serum_circle_status::not_calculated:
                         return "not calculated";
                     case serum_circle_status::good:
                         return "good";
                     case serum_circle_status::non_regular_homologous_titer:
                         return "non regular homologous titer";
                     case serum_circle_status::titer_too_low:
                         return "titer too low";
                     case serum_circle_status::serum_disconnected:
                         return "serum disconnected";
                     case serum_circle_status::antigen_disconnected:
                         return "antigen disconnected";
                 }
                 return "unknown";
             }) //
        ;
}

// ----------------------------------------------------------------------
