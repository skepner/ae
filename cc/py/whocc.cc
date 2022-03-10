#include "py/module.hh"
#include "utils/log.hh"
#include "whocc/xlsx/xlsx.hh"
#include "whocc/xlsx/sheet-extractor.hh"

// ======================================================================

namespace ae::xlsx::inline v1
{
    inline detect_result_t sheet_detected(pybind11::object detected)
    {
        detect_result_t result;
        std::string date;
        for (const auto key : detected) {
            if (const auto key_s = key.cast<std::string>(); key_s == "lab")
                result.lab = detected[key].cast<std::string>();
            else if (key_s == "assay")
                result.assay = detected[key].cast<std::string>();
            else if (key_s == "subtype")
                result.subtype = detected[key].cast<std::string>();
            else if (key_s == "lineage")
                result.lineage = detected[key].cast<std::string>();
            else if (key_s == "rbc")
                result.rbc = detected[key].cast<std::string>();
            else if (key_s == "date")
                date = detected[key].cast<std::string>();
            else if (key_s == "sheet_format")
                result.sheet_format = detected[key].cast<std::string>();
            else if (key_s == "ignore")
                result.ignore = detected[key].cast<bool>();
            else
                AD_WARNING("py function detect returned unrecognized key/value: \"{}\": {}", key_s, static_cast<std::string>(pybind11::str(detected[key])));
        }
        if (!date.empty())
            result.date = ae::date::from_string(date, date::allow_incomplete::no, date::throw_on_error::no, result.lab == "CDC" ? date::month_first::yes : date::month_first::no);
        return result;
    }

    inline pybind11::dict serum_fields(const serum_fields_t& fields)
    {
        using namespace pybind11::literals;
        return pybind11::dict(
            "name"_a = fields.name,
            "serum_id"_a = fields.serum_id,
            "passage"_a = fields.passage,
            "species"_a = fields.species,
            "conc"_a = fields.conc,
            "dilut"_a = fields.dilut,
            "boosted"_a = fields.boosted
        );
    }

    inline pybind11::dict antigen_fields(const antigen_fields_t& fields)
    {
        using namespace pybind11::literals;
        return pybind11::dict(
            "name"_a = fields.name,
            "date"_a = fields.date,
            "passage"_a = fields.passage,
            "lab_id"_a = fields.lab_id
        );
    }

} // namespace ae::xlsx::inline v1

// ----------------------------------------------------------------------

void ae::py::whocc(pybind11::module_& mdl)
{
    using namespace pybind11::literals;

    auto whocc_submodule = mdl.def_submodule("whocc", "WHO CC related stuff");

    // ----------------------------------------------------------------------

    auto xlsx_submodule = whocc_submodule.def_submodule("xlsx", "xlsx access");

    xlsx_submodule.def("open", &ae::xlsx::open, "filename"_a);

    xlsx_submodule.def(
        "extractor",
        [](std::shared_ptr<ae::xlsx::Sheet> sheet, pybind11::object detected, bool winf) {
            return extractor_factory(sheet, ae::xlsx::sheet_detected(detected), winf ? ae::xlsx::Extractor::warn_if_not_found::yes : ae::xlsx::Extractor::warn_if_not_found::no);
        },
        "sheet"_a, "detected"_a, "warn_if_not_found"_a = true);

    pybind11::class_<ae::xlsx::Doc, std::shared_ptr<ae::xlsx::Doc>>(xlsx_submodule, "Doc") //
        .def("number_of_sheets", &ae::xlsx::Doc::number_of_sheets)                         //
        .def("sheet", &ae::xlsx::Doc::sheet, "sheet_no"_a)                                 //
        ;

    pybind11::class_<ae::xlsx::Sheet, std::shared_ptr<ae::xlsx::Sheet>>(xlsx_submodule, "Sheet")           //
        .def("name", &ae::xlsx::Sheet::name)                                                               //
        .def("number_of_rows", [](const ae::xlsx::Sheet& sheet) { return *sheet.number_of_rows(); })       //
        .def("number_of_columns", [](const ae::xlsx::Sheet& sheet) { return *sheet.number_of_columns(); }) //
        .def(
            "cell_as_str", [](const ae::xlsx::Sheet& sheet, size_t row, size_t column) { return fmt::format("{}", sheet.cell(ae::xlsx::nrow_t{row}, ae::xlsx::ncol_t{column})); }, "row"_a,
            "column"_a) //
        .def(
            "grep",
            [](const ae::xlsx::Sheet& sheet, const std::string& rex, size_t min_row, size_t max_row, size_t min_col, size_t max_col) {
                if (max_row == ae::xlsx::max_row_col)
                    max_row = *sheet.number_of_rows();
                else
                    ++max_row;
                if (max_col == ae::xlsx::max_row_col)
                    max_col = *sheet.number_of_columns();
                else
                    ++max_col;
                return sheet.grep(std::regex(rex, std::regex::icase | std::regex::ECMAScript | std::regex::optimize), {ae::xlsx::nrow_t{min_row}, ae::xlsx::ncol_t{min_col}},
                                  {ae::xlsx::nrow_t{max_row}, ae::xlsx::ncol_t{max_col}});
            },                                                                                                                     //
            "regex"_a, "min_row"_a = 0, "max_row"_a = ae::xlsx::max_row_col, "min_col"_a = 0, "max_col"_a = ae::xlsx::max_row_col, //
            pybind11::doc("max_row and max_col are the last row and col to look in"))                                              //
        ;

    pybind11::class_<ae::xlsx::cell_match_t>(xlsx_submodule, "cell_match_t")                                                                   //
        .def_property_readonly("row", [](const ae::xlsx::cell_match_t& cm) { return *cm.row; })                                                //
        .def_property_readonly("col", [](const ae::xlsx::cell_match_t& cm) { return *cm.col; })                                                //
        .def_readonly("matches", &ae::xlsx::cell_match_t::matches)                                                                             //
        .def("__repr__", [](const ae::xlsx::cell_match_t& cm) { return fmt::format("<cell_match_t: {}:{} {}>", cm.row, cm.col, cm.matches); }) //
        ;

    pybind11::class_<ae::xlsx::Extractor, std::shared_ptr<ae::xlsx::Extractor>>(xlsx_submodule, "Extractor") //
        .def("number_of_antigens", &ae::xlsx::Extractor::number_of_antigens)                                 //
        .def("number_of_sera", &ae::xlsx::Extractor::number_of_sera)                                         //
        .def(
            "antigen", [](const ae::xlsx::Extractor& extractor, size_t antigen_no) { return ae::xlsx::antigen_fields(extractor.antigen(antigen_no)); }, "antigen_no"_a) //
        .def(
            "serum", [](const ae::xlsx::Extractor& extractor, size_t serum_no) { return ae::xlsx::serum_fields(extractor.serum(serum_no)); }, "serum_no"_a) //
        .def("titer", &ae::xlsx::Extractor::titer, "antigen_no"_a, "serum_no"_a)                                                                            //
        .def("format_assay_data", &ae::xlsx::Extractor::format_assay_data, "format"_a)                                                                      //
        .def("report_data_anchors", &ae::xlsx::Extractor::report_data_anchors)                                                                              //
        .def("check_export_possibility", &ae::xlsx::Extractor::check_export_possibility)                                                                    //
        .def("lab", pybind11::overload_cast<>(&ae::xlsx::Extractor::lab, pybind11::const_))                                                                 //
        .def("date", pybind11::overload_cast<>(&ae::xlsx::Extractor::date, pybind11::const_))                                                               //
        .def("assay", pybind11::overload_cast<>(&ae::xlsx::Extractor::assay, pybind11::const_))                                                             //
        .def("subtype_without_lineage", pybind11::overload_cast<>(&ae::xlsx::Extractor::subtype_without_lineage, pybind11::const_))                         //
        .def("rbc", pybind11::overload_cast<>(&ae::xlsx::Extractor::rbc, pybind11::const_))                                                                 //
        .def("lineage", pybind11::overload_cast<>(&ae::xlsx::Extractor::lineage, pybind11::const_))                                                         //
        .def("titer_comment", &ae::xlsx::Extractor::titer_comment)                                                                                          //
        .def(
            "force_serum_name_row",
            [](ae::xlsx::Extractor& extractor, ssize_t row_no) {
                if (row_no < 0)
                    throw std::invalid_argument{fmt::format("force_serum_name_row: invalid row: {}", row_no)};
                extractor.force_serum_name_row(ae::xlsx::nrow_t{row_no});
            },
            "row_no"_a) //
        .def(
            "force_serum_passage_row",
            [](ae::xlsx::Extractor& extractor, ssize_t row_no) {
                if (row_no < 0)
                    throw std::invalid_argument{fmt::format("force_serum_passage_row: invalid row: {}", row_no)};
                extractor.force_serum_passage_row(ae::xlsx::nrow_t{row_no});
            },
            "row_no"_a) //
        .def(
            "force_serum_id_row",
            [](ae::xlsx::Extractor& extractor, ssize_t row_no) {
                if (row_no < 0)
                    throw std::invalid_argument{fmt::format("force_serum_id_row: invalid row: {}", row_no)};
                extractor.force_serum_id_row(ae::xlsx::nrow_t{row_no});
            },
            "row_no"_a) //
        ;
}

// ======================================================================
