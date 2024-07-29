#include "utils/string.hh"
#include "ad/enumerate.hh"
#include "chart/v2/number-of-dimensions.hh"
#include "chart/v2/lispmds-export.hh"
#include "chart/v2/lispmds-encode.hh"
#include "chart/v2/chart.hh"

// ----------------------------------------------------------------------

static std::string antigen_names(std::shared_ptr<ae::chart::v2::Antigens> aAntigens, const ae::chart::v2::DisconnectedPoints& disconnected);
static std::string serum_names(std::shared_ptr<ae::chart::v2::Sera> aSera, size_t aNumberOfAntigens, const ae::chart::v2::DisconnectedPoints& disconnected);
static std::string titers(std::shared_ptr<ae::chart::v2::Titers> aTiters, const ae::chart::v2::DisconnectedPoints& disconnected);
static std::string starting_coordss(const ae::chart::v2::Chart& aChart, const ae::chart::v2::DisconnectedPoints& disconnected);
static std::string batch_runs(const ae::chart::v2::Chart& aChart, const ae::chart::v2::DisconnectedPoints& disconnected);
static std::string coordinates(std::shared_ptr<ae::chart::v2::Layout> aLayout, size_t number_of_points, ae::chart::v2::number_of_dimensions_t number_of_dimensions, size_t aIndent, const ae::chart::v2::DisconnectedPoints& disconnected);
static std::string col_and_row_adjusts(const ae::chart::v2::Chart& aChart, std::shared_ptr<ae::chart::v2::Projection> aProjection, size_t aIndent, const ae::chart::v2::DisconnectedPoints& disconnected);
static std::string reference_antigens(std::shared_ptr<ae::chart::v2::Antigens> aAntigens, const ae::chart::v2::DisconnectedPoints& disconnected);
static std::string plot_spec(const ae::chart::v2::Chart& aChart, const ae::chart::v2::DisconnectedPoints& disconnected);
static std::string point_style(const acmacs::PointStyle& aStyle);
static std::string point_shape(const acmacs::PointShape& aShape);
static std::string unmoveable_coords(const ae::chart::v2::UnmovablePoints& unmoveable);

// ----------------------------------------------------------------------

std::string ae::chart::v2::export_lispmds(const Chart& aChart, std::string_view aProgramName)
{
    const auto disconnected = aChart.projections()->empty() ? DisconnectedPoints{} : (*aChart.projections())[0]->disconnected();

    auto result = fmt::format(";; MDS configuration file (version 0.6).\n;; Created by AD {}\n", aProgramName);
    if (!disconnected->empty())
        result += fmt::format(";; Disconnected points (excluded): {}\n", disconnected);
    result += fmt::format(R"(
(MAKE-MASTER-MDS-WINDOW
    (HI-IN
        '({})
        '({})
        '({})
        '{})
    {}
    {}
)",
                          antigen_names(aChart.antigens(), disconnected), serum_names(aChart.sera(), aChart.number_of_antigens(), disconnected), titers(aChart.titers(), disconnected),
                          lispmds_table_name_encode(aChart.info()->name_non_empty()), starting_coordss(aChart, disconnected), batch_runs(aChart, disconnected));

    if (auto projections = aChart.projections(); !projections->empty()) {
        auto projection = (*projections)[0];
        result.append(fmt::format(R"(
    :MDS-DIMENSIONS '{}
    :MOVEABLE-COORDS 'ALL
    :UNMOVEABLE-COORDS '{}
    :CANVAS-COORD-TRANSFORMATIONS '(
        :CANVAS-WIDTH 800 :CANVAS-HEIGHT 800
        :CANVAS-X-COORD-TRANSLATION 0.0 :CANVAS-Y-COORD-TRANSLATION 0.0
        :CANVAS-X-COORD-SCALE 1 :CANVAS-Y-COORD-SCALE 1
        :FIRST-DIMENSION 0 :SECOND-DIMENSION 1
        :BASIS-VECTOR-POINT-INDICES NIL :BASIS-VECTOR-POINT-INDICES-BACKUP NIL
        :BASIS-VECTOR-X-COORD-TRANSLATION 0 :BASIS-VECTOR-Y-COORD-TRANSLATION 0
        :TRANSLATE-TO-FIT-MDS-WINDOW T :SCALE-TO-FIT-MDS-WINDOW T
        :BASIS-VECTOR-X-COORD-SCALE 1 :BASIS-VECTOR-Y-COORD-SCALE 1)", projection->layout()->number_of_dimensions(), unmoveable_coords(projection->unmovable())));

        if (const auto transformation = projection->transformation();
            transformation != ae::draw::v1::Transformation{} && transformation.valid() && transformation.number_of_dimensions == number_of_dimensions_t{2})
            result.append(fmt::format("\n        :CANVAS-BASIS-VECTOR-0 ({:.32f} {:.32f}) :CANVAS-BASIS-VECTOR-1 ({:.32f} {:.32f})", transformation.a(), transformation.c(), transformation.b(), transformation.d()));
        result.append(1, ')');
    }
    result.append(reference_antigens(aChart.antigens(), disconnected));
    result.append(plot_spec(aChart, disconnected));
    result.append(1, ')');
    return result;

} // ae::chart::v2::export_lispmds

// ----------------------------------------------------------------------

std::string antigen_names(std::shared_ptr<ae::chart::v2::Antigens> aAntigens, const ae::chart::v2::DisconnectedPoints& disconnected)
{
    std::string result;
    for (auto [ag_no, antigen] : acmacs::enumerate(*aAntigens)) {
        if (!disconnected.contains(ag_no)) {
            if (!result.empty())
                result += ' ';
            result += lispmds_antigen_name_encode(antigen->name(), antigen->reassortant(), antigen->passage(), antigen->annotations());
        }
    }
    return result;

} // antigen_names

// ----------------------------------------------------------------------

std::string serum_names(std::shared_ptr<ae::chart::v2::Sera> aSera, size_t aNumberOfAntigens, const ae::chart::v2::DisconnectedPoints& disconnected)
{
    std::string result;
    for (auto [sr_no, serum] : acmacs::enumerate(*aSera)) {
        if (!disconnected.contains(sr_no + aNumberOfAntigens)) {
            if (!result.empty())
                result += ' ';
            result += lispmds_serum_name_encode(serum->name(), serum->reassortant(), serum->annotations(), serum->serum_id());
        }
    }
    return result;

} // serum_names

// ----------------------------------------------------------------------

std::string reference_antigens(std::shared_ptr<ae::chart::v2::Antigens> aAntigens, const ae::chart::v2::DisconnectedPoints& disconnected)
{
    std::string result = "    :REFERENCE-ANTIGENS '(";
    bool append_space = false;
    for (auto [ag_no, antigen] : acmacs::enumerate(*aAntigens)) {
        if (antigen->reference() && !disconnected.contains(ag_no)) {
            if (append_space)
                result += ' ';
            else
                append_space = true;
            result += lispmds_antigen_name_encode(antigen->name(), antigen->reassortant(), antigen->passage(), antigen->annotations());
        }
    }
    return result + ")\n";

} // reference_antigens

// ----------------------------------------------------------------------

std::string titers(std::shared_ptr<ae::chart::v2::Titers> aTiters, const ae::chart::v2::DisconnectedPoints& disconnected)
{
    std::string result;
    const size_t number_of_antigens = aTiters->number_of_antigens();
    const size_t number_of_sera = aTiters->number_of_sera();

    for (size_t ag_no = 0; ag_no < number_of_antigens; ++ag_no) {
        if (!disconnected.contains(ag_no)) {
            if (ag_no)
                result.append(1, '\n').append(12, ' ');
            result.append(1, '(');
            for (size_t sr_no = 0; sr_no < number_of_sera; ++sr_no) {
                if (!disconnected.contains(sr_no + number_of_antigens)) {
                    if (sr_no)
                        result.append(1, ' ');
                    result.append(aTiters->titer(ag_no, sr_no).logged_as_string());
                }
            }
            result.append(1, ')');
        }
    }

    return result;

} // titers

// ----------------------------------------------------------------------

std::string starting_coordss(const ae::chart::v2::Chart& aChart, const ae::chart::v2::DisconnectedPoints& disconnected)
{
    auto projections = aChart.projections();
    if (projections->empty())
        return {};
    auto projection = (*projections)[0];
    auto layout = projection->layout();
    const auto number_of_points = layout->number_of_points();
    const auto number_of_dimensions = layout->number_of_dimensions();
    if (number_of_points && number_of_dimensions.get() > 0)
        return fmt::format(":STARTING-COORDSS '(\n        {}\n{})", coordinates(layout, number_of_points, number_of_dimensions, 8, disconnected), col_and_row_adjusts(aChart, projection, 8, disconnected));
    else
        return {};

} // starting_coordss

// ----------------------------------------------------------------------

std::string unmoveable_coords(const ae::chart::v2::UnmovablePoints& unmovable)
{
    if (!unmovable.empty()) {
        return fmt::format("({})", fmt::join(unmovable, " "));
    }
    else
        return "NIL";

} // unmoveable_coords

// ----------------------------------------------------------------------

std::string batch_runs(const ae::chart::v2::Chart& aChart, const ae::chart::v2::DisconnectedPoints& disconnected)
{
    auto projections = aChart.projections();
    if (projections->size() < 2)
        return {};
    std::string result = "  :BATCH-RUNS '(";
    for (size_t projection_no = 1; projection_no < projections->size(); ++projection_no) {
        auto projection = (*projections)[projection_no];
        auto layout = projection->layout();
        if (projection_no > 1)
            result.append("\n                ");
        result.append("((" + coordinates(layout, layout->number_of_points(), layout->number_of_dimensions(), 18, disconnected) + col_and_row_adjusts(aChart, projection, 18, disconnected) + ')');
        std::string stress_s;
        if (auto stress = projection->stress(); !std::isnan(stress) && stress >= 0)
            stress_s = fmt::format("{}", stress);
        else
            stress_s = "0";
        result.append("\n                 " + stress_s + " MULTIPLE-END-CONDITIONS NIL)");
    }
    result.append(1, ')');
    return result;

} // batch_runs

// ----------------------------------------------------------------------

std::string coordinates(std::shared_ptr<ae::chart::v2::Layout> aLayout, size_t number_of_points, ae::chart::v2::number_of_dimensions_t number_of_dimensions, size_t aIndent, const ae::chart::v2::DisconnectedPoints& disconnected)
{
    std::string result;
    for (size_t point_no = 0; point_no < number_of_points; ++point_no) {
        if (!disconnected.contains(point_no)) {
            if (point_no)
                result.append(1, '\n').append(aIndent, ' ');
            result.append(1, '(');
            for (ae::chart::v2::number_of_dimensions_t dim{0}; dim < number_of_dimensions; ++dim) {
                if (dim.get() > 0)
                    result.append(1, ' ');
                const auto c = aLayout->coordinate(point_no, dim);
                if (std::isnan(c))
                    result.append("0"); // disconnected point
                else
                    result.append(fmt::format("{}", c));
            }
            result.append(1, ')');
        }
    }
    return result;

} // coordinates

// ----------------------------------------------------------------------

std::string col_and_row_adjusts(const ae::chart::v2::Chart& aChart, std::shared_ptr<ae::chart::v2::Projection> aProjection, size_t aIndent, const ae::chart::v2::DisconnectedPoints& disconnected)
{
    std::string result{"\n"};
    result.append(aIndent, ' ').append("((COL-AND-ROW-ADJUSTS\n").append(aIndent + 2, ' ').append(1, '(');
    const auto number_of_antigens = aChart.number_of_antigens();
    const auto number_of_sera = aChart.number_of_sera();
    for (size_t ag_no = 0; ag_no < number_of_antigens; ++ag_no) {
        if (!disconnected.contains(ag_no)) {
            if (ag_no)
                result.append(1, ' ');
            result.append("-1.0d+7");
        }
    }
    result.append(1, '\n').append(aIndent + 3, ' ');
    auto cb = aProjection->forced_column_bases();
    if (!cb)
        cb = aChart.computed_column_bases(aProjection->minimum_column_basis());
    for (size_t sr_no = 0; sr_no < number_of_sera; ++sr_no) {
        if (!disconnected.contains(sr_no + number_of_antigens)) {
            if (sr_no)
                result.append(1, ' ');
            result.append(fmt::format("{}", cb->column_basis(sr_no)));
        }
    }
    result.append(1, '\n').append(aIndent + 2, ' ');

    if (auto avidity_adjusts = aProjection->avidity_adjusts(); !avidity_adjusts.empty()) {
        for (auto[point_no, aa] : acmacs::enumerate(avidity_adjusts)) {
            if (!disconnected.contains(point_no))
                result.append(1, ' ').append(fmt::format("{}", std::log2(aa)));
        }
    }
    else {
        for (size_t point_no = 0; point_no < (number_of_antigens + number_of_sera - disconnected->size()); ++point_no)
            result.append(" 0");
    }

    result.append(1, '\n').append(aIndent + 3, ' ');
    result.append(")))");
    return result;

} // col_and_row_adjusts

// ----------------------------------------------------------------------

std::string plot_spec(const ae::chart::v2::Chart& aChart, const ae::chart::v2::DisconnectedPoints& disconnected)
{
    std::string result;
    if (auto plot_spec = aChart.plot_spec(); !plot_spec->empty()) {
        result.append("    :PLOT-SPEC '(");
        auto antigens = aChart.antigens();
        const auto number_of_antigens = antigens->size();
        auto sera = aChart.sera();
        const auto number_of_sera = sera->size();
        for (size_t point_no = 0; point_no < (number_of_antigens + number_of_sera); ++point_no) {
            if (!disconnected.contains(point_no)) {
                if (point_no)
                    result.append("\n               ");
                std::string name, nm;
                if (point_no < number_of_antigens) {
                    auto antigen = (*antigens)[point_no];
                    name = lispmds_antigen_name_encode(antigen->name(), antigen->reassortant(), antigen->passage(), antigen->annotations()) + "-AG";
                    nm = ae::string::join(" ", antigen->name(), *antigen->reassortant(), antigen->passage(), ae::string::join(" ", antigen->annotations()));
                }
                else {
                    auto serum = (*sera)[point_no - number_of_antigens];
                    name = lispmds_serum_name_encode(serum->name(), serum->reassortant(), serum->annotations(), serum->serum_id()) + "-SR";
                    nm = ae::string::join(" ", serum->name(), *serum->reassortant(), ae::string::join(" ", serum->annotations()), *serum->serum_id());
                }
                result.append('(' + name + " :NM \"" + nm + '"' + point_style(plot_spec->style(point_no)) + ')');
            }
        }
        result.append(")");
    }
    return result;

    // :RAISE-POINTS 'NIL
    // :LOWER-POINTS 'NIL

} // plot_spec

// ----------------------------------------------------------------------

std::string point_shape(const acmacs::PointShape& aShape)
{
    switch (static_cast<acmacs::PointShape::Shape>(aShape)) {
      case acmacs::PointShape::Circle:
      case acmacs::PointShape::Egg:
          return "CIRCLE";
      case acmacs::PointShape::Box:
      case acmacs::PointShape::UglyEgg:
          return "RECTANGLE";
      case acmacs::PointShape::Triangle:
          return "TRIANGLE";
    }
    return "CIRCLE"; // gcc 7.2 wants this

} // point_shape

// ----------------------------------------------------------------------

std::string point_style(const acmacs::PointStyle& aStyle)
{
    const auto make_color = [](const char* key, Color color) {
        return fmt::format(" :{} \"{:X}\"", key, color.without_transparency());
    };

    std::string result;
    result.append(fmt::format(" :DS {}", aStyle.size().value() * acmacs::lispmds::DS_SCALE));
    if (aStyle.label().shown)
        result.append(" :WN \"" + static_cast<std::string>(aStyle.label_text()) + "\"");
    else
        result.append(" :WN \"\"");
    result.append(" :SH \"" + point_shape(aStyle.shape()) + '"');
    result.append(fmt::format(" :NS {}", std::lround(aStyle.label().size.value() * acmacs::lispmds::NS_SCALE))); // :NS must be integer (otherwise tk complains)
    result.append(make_color("NC", aStyle.label().color));
    if (aStyle.fill() == TRANSPARENT)
        result.append(" :CO \"{}\"");
    else
        result.append(make_color("CO", aStyle.fill()));
    if (aStyle.outline() == TRANSPARENT)
        result.append(" :OC \"{}\"");
    else
        result.append(make_color("OC", aStyle.outline()));
    // if (const auto alpha = acmacs::color::alpha(aStyle.fill); alpha < 1.0)
    if (const auto alpha = aStyle.fill().alpha(); alpha < 1.0)
        result.append(fmt::format(" :TR {}", 1.0 - alpha));

    return result;

} // point_style

// ----------------------------------------------------------------------
