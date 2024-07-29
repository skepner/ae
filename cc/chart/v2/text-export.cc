#include "utils/string.hh"
#include "ext/range-v3.hh"
#include "chart/v2/text-export.hh"
// #include "chart/v2/chart.hh"

// ----------------------------------------------------------------------

static std::string export_forced_column_bases_to_text(const ae::chart::v2::Chart& chart);
static std::string export_projections_to_text(const ae::chart::v2::Chart& chart);
static std::string export_plot_spec_to_text(const ae::chart::v2::Chart& chart);
static std::string export_extensions_to_text(const ae::chart::v2::Chart& chart);
static std::string export_style_to_text(const acmacs::PointStyle& aStyle);

// ----------------------------------------------------------------------

std::string ae::chart::v2::export_text(const Chart& chart)
{
    return ae::string::join("\n\n",
                            export_info_to_text(chart),                //
                            export_table_to_text(chart),               //
                            export_forced_column_bases_to_text(chart), //
                            export_projections_to_text(chart),         //
                            export_plot_spec_to_text(chart),           //
                            export_extensions_to_text(chart)           //
    );

} // ae::chart::v2::export_text

// ----------------------------------------------------------------------

std::string ae::chart::v2::export_table_to_text(const Chart& chart, std::optional<size_t> just_layer, bool sort, show_clades_t show_clades, org_mode_separators_t org_mode_separators, show_aa_t show_aa)
{
    using namespace std::string_view_literals;
    fmt::memory_buffer result;
    auto antigens = chart.antigens();
    auto sera = chart.sera();
    auto titers = chart.titers();
    // const auto max_antigen_name = max_full_name(*antigens);
    const char* separator = org_mode_separators == org_mode_separators_t::yes ? "|" : "";

    if (just_layer.has_value()) {
        if (*just_layer >= titers->number_of_layers())
            throw std::runtime_error(fmt::format("Invalid layer: {}, number of layers in the chart: {}", *just_layer, titers->number_of_layers()));
        fmt::format_to(std::back_inserter(result), "Layer {}   {}\n\n", *just_layer, chart.info()->source(*just_layer)->make_name());
    }

    auto antigen_order = range_from_0_to(antigens->size()) | ranges::to_vector;
    auto serum_order = range_from_0_to(sera->size()) | ranges::to_vector;
    if (sort) {
        ranges::sort(antigen_order, [&antigens](auto i1, auto i2) { return antigens->at(i1)->name_full() < antigens->at(i2)->name_full(); });
        ranges::sort(serum_order, [&sera](auto i1, auto i2) { return sera->at(i1)->name_full() < sera->at(i2)->name_full(); });
    }

    if (org_mode_separators == org_mode_separators_t::yes)
        fmt::format_to(std::back_inserter(result), "-*- Org -*-\n\n");

    // row: ag_no, antigen_name, ag_passage, ag_date, ag_clades, :ref, :aa, :nuc, serum ...
    enum table_column : size_t {tc_ag_no = 0, tc_antigen_name, tc_ag_passage, tc_ag_date, tc_ag_clades, tc_ref, tc_aa, tc_nuc, antigen_data_columns};
    // column: sr_no, serum_name, sr_passage, serum_id, sr_clades, sep, antigens
    enum table_row : size_t {tr_sr_no = 0, tr_serum_name, tr_sr_passage, tr_serum_id, tr_sr_clades, tr_sr_sep, serum_data_rows};

    const auto number_of_columns = sera->size() + antigen_data_columns;
    const auto number_of_rows = antigens->size() + serum_data_rows;
    std::vector<std::vector<std::string>> table(number_of_rows, std::vector<std::string>(number_of_columns));
    // header
    auto& row_serum_no = table[tr_sr_no];
    auto& row_serum_name = table[tr_serum_name];
    auto& row_serum_passage = table[tr_sr_passage];
    auto& row_serum_id = table[tr_serum_id];
    auto& row_serum_clades = table[tr_sr_clades];
    for (auto index : range_from_0_to(sera->size())) {
        const auto serum_no = serum_order[index];
        row_serum_no[antigen_data_columns + index] = fmt::format("{}", index);
        row_serum_name[antigen_data_columns + index] = sera->at(serum_no)->format("{location_abbreviated}/{year2}");
        row_serum_passage[antigen_data_columns + index] = sera->at(serum_no)->format("{passage}");
        row_serum_id[antigen_data_columns + index] = sera->at(serum_no)->format("{serum_id}");
        if (show_clades == show_clades_t::yes)
            row_serum_clades[antigen_data_columns + index] = ae::string::join(" ", sera->at(serum_no)->clades());
    }
    table[tr_sr_sep][0] = "---";
    // antigens
    for (auto [ag_no, antigen_no] : acmacs::enumerate(antigen_order)) {
        auto antigen = antigens->at(antigen_no);
        auto& row = table[ag_no + serum_data_rows];
        if (!sort)
            row[0] = fmt::format("{}", ag_no);
        row[tc_antigen_name] = antigen->format("{name_anntotations_reassortant}");
        row[tc_ag_passage] = antigen->format("{passage}");
        row[tc_ag_date] = antigen->format("{date}");
        if (antigen->reference())
            row[tc_ref] = ":ref";
        if (show_aa == show_aa_t::yes) {
            if (!antigen->sequence_aa().empty())
                row[tc_aa] = ":aa";
            if (!antigen->sequence_nuc().empty())
                row[tc_nuc] = ":nuc";
        }
        if (show_clades == show_clades_t::yes)
            row[tc_ag_clades] = ae::string::join(" ", antigen->clades());
    }
    if (!just_layer.has_value()) { // merged table
        for (auto [ag_no, antigen_no] : acmacs::enumerate(antigen_order)) {
            auto antigen = antigens->at(antigen_no);
            auto& row = table[ag_no + serum_data_rows];
            for (auto [sr_no, serum_no] : acmacs::enumerate(serum_order))
                row[sr_no + antigen_data_columns] = fmt::format("{:>6s} ", *titers->titer(antigen_no, serum_no));
        }
    }
    else { // just layer
        const auto [antigens_of_layer, sera_of_layer] = titers->antigens_sera_of_layer(*just_layer);
        for (auto ag_no : antigens_of_layer) {
            auto& row = table[ag_no + serum_data_rows];
            auto antigen = antigens->at(ag_no);
            for (auto [sr_no, serum_no] : acmacs::enumerate(serum_order))
                row[sr_no + antigen_data_columns] = fmt::format("{:>6s} ", *titers->titer_of_layer(*just_layer, ag_no, serum_no));
        }
    }
    // align
    for (const auto col_no : range_from_0_to(number_of_columns)) {
        size_t col_width = 0;
        for (const auto& row : table)
            col_width = std::max(col_width, row[col_no].size());
        if (col_width) {
            for (auto [row_no, row] : acmacs::enumerate(table)) {
                if (col_no == 0) // ag_no, right aligned
                    row[col_no] = fmt::format("{:>{}s}", row[col_no], col_width);
                else if (row_no < serum_data_rows) // centered
                    row[col_no] = fmt::format("{:^{}s}", row[col_no], col_width);
                else // left aligned
                    row[col_no] = fmt::format("{:<{}s}", row[col_no], col_width);
            }
        }
    }
    // output
    for (const auto& row : table) {
        bool sep = false;
        if (org_mode_separators == org_mode_separators_t::yes)
            fmt::format_to(std::back_inserter(result), "{}", separator);
        for (const auto& cell : row) {
            if (sep)
                fmt::format_to(std::back_inserter(result), " {} ", separator);
            else
                sep = true;
            fmt::format_to(std::back_inserter(result), "{}", cell);
        }
        fmt::format_to(std::back_inserter(result), "\n");
    }
    fmt::format_to(std::back_inserter(result), "\n");

    // serum table
    const size_t serum_table_columns = 5; // no, name, passage, id, clades
    std::vector<std::vector<std::string>> serum_table(sera->size(), std::vector<std::string>(serum_table_columns));
    for (auto [sr_no, serum_no] : acmacs::enumerate(serum_order)) {
        auto serum = sera->at(serum_no);
        auto& row = serum_table[sr_no];
        if (!sort)
            row[0] = fmt::format("{}", sr_no);
        row[1] = serum->format("{name_anntotations_reassortant}");
        row[2] = serum->format("{passage}");
        row[3] = serum->format("{serum_id}");
        if (show_clades == show_clades_t::yes)
            row[4] = ae::string::join(" ", serum->clades());
    }
    for (const auto col_no : range_from_0_to(serum_table_columns)) {
        size_t col_width = 0;
        for (const auto& row : serum_table)
            col_width = std::max(col_width, row[col_no].size());
        if (col_width) {
            for (auto [row_no, row] : acmacs::enumerate(serum_table)) {
                if (col_no == 0) // sr_no, right aligned
                    row[col_no] = fmt::format("{:>{}s}", row[col_no], col_width);
                else // left aligned
                    row[col_no] = fmt::format("{:<{}s}", row[col_no], col_width);
            }
        }
    }
    for (const auto& row : serum_table) {
        bool sep = false;
        if (org_mode_separators == org_mode_separators_t::yes)
            fmt::format_to(std::back_inserter(result), "{}", separator);
        else
            fmt::format_to(std::back_inserter(result), "               ");
        for (const auto& cell : row) {
            if (sep)
                fmt::format_to(std::back_inserter(result), " {} ", separator);
            else
                sep = true;
            fmt::format_to(std::back_inserter(result), "{}", cell);
        }
        fmt::format_to(std::back_inserter(result), "\n");
    }
    fmt::format_to(std::back_inserter(result), "\n");

    return fmt::to_string(result);

} // ae::chart::v2::export_table_to_text

// ----------------------------------------------------------------------

std::string ae::chart::v2::export_info_to_text(const Chart& chart)
{
    fmt::memory_buffer result;

    const auto do_export = [&result](ae::chart::v2::InfoP info) {
        fmt::format_to(std::back_inserter(result), "{}", ae::string::join(" ", *info->virus(), info->virus_type(), *info->assay(), *info->date(), info->name(), *info->lab(), *info->rbc_species(), info->subset()));
        // info->table_type()
    };

    auto info = chart.info();
    do_export(info);
    if (const auto number_of_sources = info->number_of_sources(); number_of_sources) {
        for (size_t source_no = 0; source_no < number_of_sources; ++source_no) {
            fmt::format_to(std::back_inserter(result), "\n{:2d}  ", source_no);
            do_export(info->source(source_no));
        }
    }
    return fmt::to_string(result);

} // ae::chart::v2::export_info_to_text

// ----------------------------------------------------------------------

std::string export_forced_column_bases_to_text(const ae::chart::v2::Chart& chart)
{
    fmt::memory_buffer result;
    if (const auto column_bases = chart.forced_column_bases(ae::chart::v2::MinimumColumnBasis{}); column_bases) {
        fmt::format_to(std::back_inserter(result), "forced-column-bases:");
        for (size_t sr_no = 0; sr_no < column_bases->size(); ++sr_no)
            fmt::format_to(std::back_inserter(result), "{}", column_bases->column_basis(sr_no));
    }
    return fmt::to_string(result);

} // export_forced_column_bases_to_text

// ----------------------------------------------------------------------

std::string export_projections_to_text(const ae::chart::v2::Chart& chart)
{
    fmt::memory_buffer result;
    if (auto projections = chart.projections(); !projections->empty()) {
        fmt::format_to(std::back_inserter(result), "projections: {}\n\n", projections->size());
        for (size_t projection_no = 0; projection_no < projections->size(); ++projection_no) {
            fmt::format_to(std::back_inserter(result), "projection {}\n", projection_no);
            auto projection = (*projections)[projection_no];
            if (const auto comment = projection->comment(); !comment.empty())
                fmt::format_to(std::back_inserter(result), "  comment: {}\n", comment);
            if (const auto stress = projection->stress(); !std::isnan(stress) && stress >= 0)
                fmt::format_to(std::back_inserter(result), "  stress: {:.10f}\n", stress); // to avoid problems comparing exported charts during test on linux
            if (const auto minimum_column_basis = projection->minimum_column_basis(); !minimum_column_basis.is_none())
                fmt::format_to(std::back_inserter(result), "  minimum-column-basis: {}\n", minimum_column_basis);
            if (const auto column_bases = projection->forced_column_bases(); column_bases) {
                fmt::format_to(std::back_inserter(result), "  forced-column-bases:");
                for (size_t sr_no = 0; sr_no < column_bases->size(); ++sr_no)
                    fmt::format_to(std::back_inserter(result), "{}", column_bases->column_basis(sr_no));
                fmt::format_to(std::back_inserter(result), "\n");
            }
            if (const auto transformation = projection->transformation(); transformation != ae::draw::v1::Transformation{} && transformation.valid())
                fmt::format_to(std::back_inserter(result), "  transformation: {}\n", transformation.as_vector());
            if (projection->dodgy_titer_is_regular() == ae::chart::v2::dodgy_titer_is_regular::yes)
                fmt::format_to(std::back_inserter(result), "  dodgy-titer-is-regular: {}\n", true);
            if (projection->stress_diff_to_stop() > 0)
                fmt::format_to(std::back_inserter(result), "  stress-diff-to-stop: {}\n", projection->stress_diff_to_stop());
            if (const auto unmovable = projection->unmovable(); !unmovable->empty())
                fmt::format_to(std::back_inserter(result), "  unmovable: {}\n", unmovable);
            if (const auto disconnected = projection->disconnected(); !disconnected->empty())
                fmt::format_to(std::back_inserter(result), "  disconnected: {}\n", disconnected);
            if (const auto unmovable_in_the_last_dimension = projection->unmovable_in_the_last_dimension(); !unmovable_in_the_last_dimension->empty())
                fmt::format_to(std::back_inserter(result), "  unmovable-in-the-last-dimension: {}\n", unmovable_in_the_last_dimension);
            if (const auto avidity_adjusts = projection->avidity_adjusts(); !avidity_adjusts.empty())
                fmt::format_to(std::back_inserter(result), "  avidity-adjusts: {}\n", avidity_adjusts);

            auto layout = projection->layout();
            const auto number_of_dimensions = layout->number_of_dimensions();
            fmt::format_to(std::back_inserter(result), "  layout {} x {}\n", layout->number_of_points(), number_of_dimensions);
            if (const auto number_of_points = layout->number_of_points(); number_of_points && number_of_dimensions.get() > 0) {
                for (size_t p_no = 0; p_no < number_of_points; ++p_no)
                    fmt::format_to(std::back_inserter(result), fmt::runtime("    {:4d} {:13.10f}\n"), p_no, layout->at(p_no)); // to avoid problems comparing exported charts during test on linux
            }
        }
    }
    return fmt::to_string(result);

} // export_projections_to_text

// ----------------------------------------------------------------------

std::string export_plot_spec_to_text(const ae::chart::v2::Chart& chart)
{
    fmt::memory_buffer result;

    if (auto plot_spec = chart.plot_spec(); !plot_spec->empty()) {
        fmt::format_to(std::back_inserter(result), "plot-spec:\n");
        if (const auto drawing_order = plot_spec->drawing_order(); !drawing_order->empty()) {
            fmt::format_to(std::back_inserter(result), "  drawing-order:");
            for (const auto& index : drawing_order)
                fmt::format_to(std::back_inserter(result), " {}", index);
            fmt::format_to(std::back_inserter(result), "\n");
        }
        if (const auto color = plot_spec->error_line_positive_color(); color != RED)
            fmt::format_to(std::back_inserter(result), "  error-line-positive-color: {}\n", color);
        if (const auto color = plot_spec->error_line_negative_color(); color != BLUE)
            fmt::format_to(std::back_inserter(result), "  error-line-negative-color: {}\n", color);

        const auto compacted = plot_spec->compacted();

        fmt::format_to(std::back_inserter(result), "  plot-style-per-point:");
        for (const auto& index : compacted.index)
            fmt::format_to(std::back_inserter(result), " {}", index);
        fmt::format_to(std::back_inserter(result), "\n");

        fmt::format_to(std::back_inserter(result), "  styles: {}\n", compacted.styles.size());
        for (size_t style_no = 0; style_no < compacted.styles.size(); ++style_no)
            fmt::format_to(std::back_inserter(result), "    {:2d} {}\n", style_no, export_style_to_text(compacted.styles[style_no]));

        // "g": {},                  // ? grid data
        // "l": [],                  // ? for each procrustes line, index in the "L" list
        // "L": []                    // ? list of procrustes lines styles
        // "s": [],                  // list of point indices for point shown on all maps in the time series
        // "t": {}                    // title style?
    }
    return fmt::to_string(result);

} // export_plot_spec_to_text

// ----------------------------------------------------------------------

std::string export_style_to_text(const acmacs::PointStyle& aStyle)
{
    fmt::memory_buffer result;
    fmt::format_to(std::back_inserter(result), "shown:{} fill:\"{}\" outline:\"{}\" outline_width:{} size:{} rotation:{} aspect:{} shape:{}", aStyle.shown(), aStyle.fill(), aStyle.outline(),
                   aStyle.outline_width().value(), aStyle.size().value(), *aStyle.rotation(), *aStyle.aspect(), aStyle.shape());
    fmt::format_to(std::back_inserter(result), " label:{{ shown:{} text:\"{}\" font_family:\"{}\" slant:\"{}\" weight:\"{}\" size:{} color:\"{}\" rotation:{} interline:{} offset:{}",
                   aStyle.label().shown, aStyle.label_text(), aStyle.label().style.font_family, aStyle.label().style.slant, aStyle.label().style.weight, aStyle.label().size.value(),
                   aStyle.label().color, *aStyle.label().rotation, aStyle.label().interline, aStyle.label().offset);

    return fmt::to_string(result);

} // export_style_to_text

// ----------------------------------------------------------------------

std::string export_extensions_to_text(const ae::chart::v2::Chart& chart)
{
    fmt::memory_buffer result;
    if (const auto& ext = chart.extension_fields(); ext.is_object())
        fmt::format_to(std::back_inserter(result), "extensions:\n{}\n", rjson::pretty(ext));
    return fmt::to_string(result);

} // export_extensions_to_text

// ----------------------------------------------------------------------
