#include "py/module.hh"
#include "locdb/v3/locdb.hh"

// ======================================================================

namespace ae::py
{
}


// ======================================================================

void ae::py::locdb_v3(pybind11::module_& mdl)
{
    using namespace std::string_view_literals;
    using namespace pybind11::literals;
    using namespace ae::locdb::v3;

    // ----------------------------------------------------------------------

    auto locdb_v3_submodule = mdl.def_submodule("locdb_v3", "locdb_v3 api");

    locdb_v3_submodule.def("locdb", &get, pybind11::return_value_policy::reference);

    pybind11::class_<Db, std::shared_ptr<Db>>(locdb_v3_submodule, "Locdb")              //
        .def("continent", &Db::continent, "country"_a)                                  //
        .def("country", &Db::country, "location_name"_a)                                //
        .def("abbreviation", &Db::abbreviation, "location_name"_a)                      //
        .def("cdc_abbreviation", &Db::find_cdc_abbreviation_by_name, "location_name"_a) //
        .def("find", &Db::find, "location_name"_a)                                      //
        ;
}

// ======================================================================
