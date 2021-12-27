#include "ext/range-v3.hh"
#include "utils/log.hh"
#include "utils/regex.hh"
#include "utils/string.hh"
#include "virus/name-parse.hh"
#include "virus/passage.hh"
#include "whocc/xlsx/sheet-extractor.hh"
#include "whocc/xlsx/error.hh"

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

constexpr auto regex_icase = std::regex::icase | std::regex::ECMAScript | std::regex::optimize;

// static const std::regex re_ac_ignore_sheet{"^AC-IGNORE", regex_icase};

// static const std::regex re_table_title_crick{R"(^Table\s+[XY0-9-]+\s*\.\s*Antigenic analys[ie]s of influenza ([AB](?:\(H3N2\)|\(H1N1\)pdm09)?)\s*viruses\s*-?\s*\(?(Plaque\s+Reduction\s+Neutralisation\s*\(MDCK-SIAT\)|(?:Victoria|Yamagata)\s+lineage)?\)?\s*\(?(20[0-2][0-9]-[01][0-9]-[0-3][0-9])\)?)", regex_icase};

// static const std::regex re_antigen_passage{"^(MDCK|QMC|C|SIAT|S|E|HCK|X)[0-9X]", regex_icase};
static const std::regex re_serum_passage{"^(MDCK|QMC|C|SIAT|S|E|HCK|CELL|EGG)", regex_icase};

// static const std::regex re_CDC_antigen_passage{R"(^((?:MDCK|SIAT|S|E|HCK|QMC|C|X)[0-9X][^\s\(]*)\s*(?:\(([\d/]+)\))?[A-Z]*$)", regex_icase};
static const std::regex re_CDC_antigen_lab_id{"^[0-9]{10}$", regex_icase};
static const std::regex re_CDC_serum_index{"^([A-Z]|EGG)$", regex_icase}; // EGG is excel auto-correction artefact
static const std::regex re_CDC_serum_control{R"(^\s*SERUM\s+CONTROL\s*$)", regex_icase};
static const std::regex re_CDC_lot_label{R"(^\s*LOT\s*#?\s*$)", regex_icase};
static const std::regex re_CDC_date_label{R"(^\s*DATE\s*$)", regex_icase};
static const std::regex re_CDC_treated_label{R"(^\s*TREATED\s*$)", regex_icase};
static const std::regex re_CDC_date_treated_label{R"(^\s*DATE\s+TREATED\s*$)", regex_icase};
static const std::regex re_CDC_species_label{R"(^\s*SPECIES\s*$)", regex_icase};
static const std::regex re_CDC_boosted_label{R"(^\s*BOOSTED\s*$)", regex_icase};
static const std::regex re_CDC_conc_label{R"(^\s*CONC\s*$)", regex_icase};
static const std::regex re_CDC_dilut_label{R"(^\s*DILUT\s*$)", regex_icase};
static const std::regex re_CDC_passage_label{R"(^\s*PASSAGE\s*$)", regex_icase};
static const std::regex re_CDC_pool_label{R"(^\s*POOL\s*$)", regex_icase};
static const std::regex re_CDC_titer_label{R"(^\s*(BACK)?\s*TITER\b)", regex_icase};
static const std::regex re_CDC_ha_group_label{R"(^\s*HA\s*GROUP\b)", regex_icase};
static const std::regex re_CDC_antigen_control{R"(\bCONTROL\b)", regex_icase};

static const std::regex re_AC21_serum_index{R"(^[0-9]+$)", regex_icase};
static const std::regex re_AC21_ID_label{R"(^\s*ID\s*$)", regex_icase};
static const std::regex re_AC21_serum_label{R"(^\s*serum\s*$)", regex_icase};
static const std::regex re_AC21_date_label{R"(^\s*date\s*$)", regex_icase};
static const std::regex re_AC21_species_label{R"(^\s*SPECIES\s*$)", regex_icase};
static const std::regex re_AC21_treat_label{R"(^\s*treat\.?\s*$)", regex_icase};
static const std::regex re_AC21_type_label{R"(^\s*TYPE\s*$)", regex_icase};
static const std::regex re_AC21_batch_label{R"(^\s*BATCH\s*#?\s*$)", regex_icase};
static const std::regex re_AC21_comment_label{R"(^\s*COMMENT\s*$)", regex_icase};
static const std::regex re_AC21_empty{R"(^\s*$)", regex_icase};

static const std::regex re_CRICK_serum_name_1{"^([AB]/[A-Z '_-]+|NYMC\\s+X-[0-9]+[A-Z]*)$", regex_icase};
static const std::regex re_CRICK_serum_name_2{"^[A-Z0-9-/]+$", regex_icase};
#define pattern_CRICK_serum_id "F[0-9]+/[0-2][0-9]"
static const std::regex re_CRICK_serum_id{R"(^(?:[A-Z\s]+\s+)?\s*(F[0-9]+/[0-2][0-9]|SH[\s\d,/]+)(?:\*(\d)(?:,\d)?)?$)", regex_icase};
static const std::regex re_CRICK_less_than{R"(^\s*<\s*=\s*(<\d+)\s*$)", regex_icase};
static const std::regex re_CRICK_less_than_2{R"(^Superscripts.*\s+(\d)\s*<\s*=\s*(<\d+)\s*$)", regex_icase};
static const std::regex re_CRICK_less_than_multi{R"(^\s*\d\s*<\s*=\s*<\d+\s*[;,])", regex_icase};
static const std::regex re_CRICK_less_than_multi_entry{R"(^\s*(\d)\s*<\s*=\s*(<\d+)\s*$)", regex_icase};

static const std::regex re_CRICK_prn_2fold{"^2-fold$", regex_icase};
static const std::regex re_CRICK_prn_read{"^read$", regex_icase};

static const std::regex re_NIID_serum_name{R"(^\s*(?:\d+[A-Z]\s+)?)"           // [clade]
                                           R"(([A-Z][A-Z\d\s\-_\./\(\)]+)\s+)" // name with reassortant $1
                                           // R"((?:(EGG|CELL|HCK)\s+)?)"         // passage type (sometimes absent for reassortants) $2
                                           // R"((?:NIID\s+)?)"                   // NIID artefact
                                           R"(NO\s*\.\s*([\d\-]+)$)",          // serum_id $2
                                           regex_icase};
static const std::regex re_NIID_serum_passage{R"(\s*(EGG|CELL|HCK)?\s*(?:NIID)?\s*$)", regex_icase};

static const std::regex re_NIID_serum_name_fix{R"(\s*([\-/])\s*)", regex_icase}; // remove spaces around - and /
static const std::regex re_NIID_lab_id_label{"^\\s*NIID-ID\\s*$", regex_icase};
static const std::regex re_NIID_serum_name_row_non_serum_label{R"((HA\s*group))", regex_icase};

static const std::regex re_VIDRL_antigen_lab_id{"^(SL|VW)[0-9]{8}$", regex_icase};
static const std::regex re_VIDRL_antigen_date_column_title{"^\\s*Sample\\s*Date\\s*$", regex_icase};
static const std::regex re_VIDRL_antigen_lab_id_column_title{"^\\s*VW\\s*$", regex_icase};
static const std::regex re_VIDRL_serum_name{"^(?:[AB]/)?([A-Z][A-Z ]+)/?([0-9]+)(?:_.*)?$", regex_icase}; // optional mutant info at the end
#define pattern_VIDRL_serum_id "[AF][0-9][0-9][0-9][0-9](?:-[0-9]+D)?"
static const std::regex re_VIDRL_serum_id{"^(" pattern_VIDRL_serum_id "|" pattern_CRICK_serum_id ")$", regex_icase};
static const std::regex re_VIDRL_serum_id_with_days{"^[AF][0-9][0-9][0-9][0-9]-[0-9]+D$", regex_icase};

static const std::regex re_human_who_serum{R"(^\s*(.*(HUMAN|WHO|NORMAL)|GOAT|POST? VAX)\b)", regex_icase}; // "POST VAX" is in VIDRL H3 HI 2021

#pragma GCC diagnostic pop

static const std::string_view LineageVictoria{"VICTORIA"};
static const std::string_view LineageYamagata{"YAMAGATA"};

// ----------------------------------------------------------------------

std::shared_ptr<ae::xlsx::Extractor> ae::xlsx::v1::extractor_factory(std::shared_ptr<Sheet> sheet, const detect_result_t& detected, Extractor::warn_if_not_found winf)
{
    try {
        std::unique_ptr<Extractor> extractor;
        if (detected.ignore) {
            AD_INFO("Sheet \"{}\": ignored on request in cell A1", sheet->name());
            return nullptr;
        }

        if (sheet->number_of_rows() < nrow_t{5} || sheet->number_of_columns() < ncol_t{5}) {
            AD_INFO("Sheet \"{}\": is too small, ignored", sheet->name());
            return nullptr;
        }

        AD_INFO("{}", detected);
        if (detected.sheet_format == "ac-21") {
            extractor = std::make_unique<ExtractorAc21>(sheet);
            extractor->lab(detected.lab);
            extractor->subtype(detected.subtype);
            extractor->lineage(detected.lineage);
            extractor->assay(detected.assay);
            extractor->rbc(detected.rbc);
        }
        else if (detected.lab == "CDC") {
            extractor = std::make_unique<ExtractorCDC>(sheet);
            extractor->subtype(detected.subtype);
            extractor->lineage(detected.lineage);
            extractor->assay(detected.assay);
            extractor->rbc(detected.rbc);
        }
        else if (detected.lab == "CRICK") {
            if (detected.assay == "HI") {
                extractor = std::make_unique<ExtractorCrick>(sheet);
                extractor->subtype(detected.subtype);
                extractor->lineage(detected.lineage);
                extractor->rbc(detected.rbc);
            }
            else if (detected.assay == "PRN")
                extractor = std::make_unique<ExtractorCrickPRN>(sheet);
            else
                throw std::exception{};
        }
        else if (detected.lab == "NIID") {
            extractor = std::make_unique<ExtractorNIID>(sheet);
            extractor->subtype(detected.subtype);
            extractor->lineage(detected.lineage);
            extractor->assay(detected.assay);
            extractor->rbc(detected.rbc);
        }
        else if (detected.lab == "VIDRL") {
            extractor = std::make_unique<ExtractorVIDRL>(sheet);
            extractor->subtype(detected.subtype);
            extractor->lineage(detected.lineage);
            extractor->assay(detected.assay);
            extractor->rbc(detected.rbc);
        }
        else
            throw std::exception{};
        extractor->date(detected.date);
        extractor->preprocess(winf);
        return extractor;
    }
    catch (std::exception& err) {
        throw std::runtime_error{fmt::format("Sheet \"{}\": no specific extractor found, detected: {} (exception: {})", sheet->name(), detected, err.what())};
    }

} // ae::xlsx::v1::extractor_factory

// ----------------------------------------------------------------------

std::string_view ae::xlsx::v1::Extractor::subtype_without_lineage() const
{
    if (subtype_ == "A(H1N1)PDM09")
        return std::string_view(subtype_.data(), 7);
    else
        return subtype_;

} // ae::xlsx::v1::Extractor::subtype_without_lineage

// ----------------------------------------------------------------------

std::string ae::xlsx::v1::Extractor::subtype_short() const
{
    if (subtype_ == "A(H1N1)")
        return "h1";
    if (subtype_ == "A(H1N1)PDM09")
        return "h1pdm";
    if (subtype_ == "A(H3N2)")
        return "h3";
    if (subtype_ == "B") {
        if (lineage_ == LineageVictoria)
            return "bvic";
        if (lineage_ == LineageYamagata)
            return "byam";
        return "b";
    }
    return subtype_;

} // ae::xlsx::v1::Extractor::subtype_short

// ----------------------------------------------------------------------

ae::xlsx::v1::antigen_fields_t ae::xlsx::v1::Extractor::antigen(size_t ag_no) const
{
    const auto make = [this, row = antigen_rows().at(ag_no)](std::optional<ncol_t> col) -> std::string {
        if (col.has_value()) {
            if (const auto cell = sheet().cell(row, *col); !is_empty(cell))
                return fmt::format("{}", cell);
        }
        return {};
    };

    return antigen_fields_t{
        .name = make(antigen_name_column()),                     //
        .date = make_date(make(antigen_date_column())),          //
        .passage = make_passage(make(antigen_passage_column())), //
        .lab_id = make_lab_id(make(antigen_lab_id_column()))     //
    };

} // ae::xlsx::v1::Extractor::antigen

// ----------------------------------------------------------------------

void ae::xlsx::v1::Extractor::preprocess(warn_if_not_found winf)
{
    find_titers(winf);
    find_antigen_name_column(winf);
    find_antigen_date_column(winf);
    find_antigen_passage_column(winf);
    find_antigen_lab_id_column(winf);
    find_serum_rows(winf);
    exclude_control_sera(winf); // remove human, WHO, pooled sera

} // ae::xlsx::v1::Extractor::preprocess

// ----------------------------------------------------------------------

template <ae::xlsx::NRowCol nrowcol> using number_ranges = std::vector<std::pair<nrowcol, nrowcol>>;

template <ae::xlsx::NRowCol nrowcol> inline number_ranges<nrowcol> make_ranges(const std::vector<nrowcol>& numbers)
{
    number_ranges<nrowcol> rngs;
    for (const auto num : numbers) {
        if (rngs.empty() || num != (rngs.back().second + nrowcol{1}))
            rngs.emplace_back(num, num);
        else
            rngs.back().second = num;
    }
    return rngs;

} // make_ranges

template <ae::xlsx::NRowCol nrowcol> inline std::string format(const number_ranges<nrowcol>& rngs)
{
    fmt::memory_buffer out;
    bool space{false};
    for (const auto& en : rngs) {
        if (space)
            fmt::format_to(std::back_inserter(out), " ");
        else
            space = true;
        fmt::format_to(std::back_inserter(out), "{}-{}", en.first, en.second);
    }
    return fmt::to_string(out);
}

// template <typename nrowcol> requires NRowCol<nrowcol> struct fmt::formatter<number_ranges<nrowcol>> : fmt::formatter<acmacs::fmt_helper::default_formatter>
// {
//     template <typename FormatCtx> auto format(const number_ranges<nrowcol>& rngs, FormatCtx& ctx)
//     {
//         std::string prefix;
//         for (const auto& en : rngs) {
//             format_to(ctx.out(), "{}", prefix);
//             format_to(ctx.out(), "{}-{}", en.first, en.second);
//             prefix = " ";
//         }
//         return ctx.out();
//     }
// };

// ----------------------------------------------------------------------

void ae::xlsx::v1::Extractor::report_data_anchors() const
{
    AD_INFO("Sheet Data Anchors:\n  Antigen columns:\n    Name:    {}\n    Date:    {}\n    Passage: {}\n    LabId:   {}\n  Antigen rows: {}\n  Number of antigens: {}\n\n{}", //
            antigen_name_column_, antigen_date_column_, antigen_passage_column_, antigen_lab_id_column_, format(make_ranges(antigen_rows_)), antigen_rows_.size(), report_serum_anchors());

} // ae::xlsx::v1::Extractor::report_data_anchors

// ----------------------------------------------------------------------

void ae::xlsx::v1::Extractor::check_export_possibility() const // throws Error if exporting is not possible
{
    std::string msg;
    if (!antigen_name_column_.has_value())
        msg += " [no antigen name column]";
    if (antigen_rows_.size() < 3)
        msg += fmt::format(" [too few antigen rows detetcted: {}]", antigen_rows_);
    if (!msg.empty())
        throw Error(msg);

} // ae::xlsx::v1::Extractor::check_export_possibility

// ----------------------------------------------------------------------

std::string ae::xlsx::v1::Extractor::titer(size_t ag_no, size_t sr_no) const
{
    const auto cell = sheet().cell(antigen_rows().at(ag_no), serum_columns().at(sr_no));
    return std::visit(
        [&cell]<typename Content>(const Content& cont) -> std::string {
            if constexpr (std::is_same_v<Content, std::string>)
                return ae::string::remove_spaces(cont); // NIID has titers with spaces, e.g. "< 10"
            else if constexpr (std::is_same_v<Content, long>)
                return fmt::format("{}", cont);
            else if constexpr (std::is_same_v<Content, double>)
                return fmt::format("{}", std::lround(cont)); // crick sometimes has real number titers
            else
                return fmt::format("{}", cell);
        },
        cell);

} // ae::xlsx::v1::Extractor::titer

// ----------------------------------------------------------------------

void ae::xlsx::v1::Extractor::find_titers(warn_if_not_found winf)
{
    std::vector<std::pair<nrow_t, range<ncol_t>>> rows;
    // AD_DEBUG("Sheet {}", sheet().name());
    for (nrow_t row{0}; row < sheet().number_of_rows(); ++row) {
        auto titers = sheet().titer_range(row);
        adjust_titer_range(row, titers);
        if (titers.valid() && titers.length() > 2 && titers.first > ncol_t{0} && valid_titer_row(row, titers))
            rows.emplace_back(row, std::move(titers));
    }

    if (!ranges::all_of(rows, [&rows](const auto& en) { return en.second == rows[0].second; })) {
        fmt::memory_buffer report; // fmt::format(rows, "{}", "\n  "));
        for (const auto& [row_no, rng] : rows)
            fmt::format_to(std::back_inserter(report), "    {}: {} ({})\n", row_no, rng, rng.length());
        if (winf == warn_if_not_found::yes)
            AD_WARNING(winf == warn_if_not_found::yes, "sheet \"{}\": variable titer row ranges:\n{}", sheet().name(), fmt::to_string(report));
    }

    for (ncol_t col{rows[0].second.first}; col <= rows[0].second.second; ++col)
        serum_columns_.push_back(col);
    antigen_rows_.resize(rows.size(), nrow_t{0});
    ranges::transform(rows, std::begin(antigen_rows_), [](const auto& row) { return row.first; });

} // ae::xlsx::v1::Extractor::find_titers

// ----------------------------------------------------------------------

bool ae::xlsx::v1::Extractor::is_virus_name(nrow_t row, ncol_t col) const
{
    return ae::virus::name::is_good(fmt::format("{}", sheet().cell(row, col)));

} // ae::xlsx::v1::Extractor::is_virus_name

// ----------------------------------------------------------------------

void ae::xlsx::v1::Extractor::find_antigen_name_column(warn_if_not_found winf)
{
    for (ncol_t col{0}; col < serum_columns()[0]; ++col) { // to the left from titers
        if (static_cast<size_t>(ranges::count_if(antigen_rows_, [col, this](nrow_t row) { return is_virus_name(row, col); })) > (antigen_rows_.size() / 2)) {
            antigen_name_column_ = col;
            break;
        }
    }

    if (antigen_name_column_.has_value())
        remove_redundant_antigen_rows(winf);
    else
        AD_WARNING(winf == warn_if_not_found::yes, "Antigen name column not found");

} // ae::xlsx::v1::Extractor::find_antigen_name_column

// ----------------------------------------------------------------------

void ae::xlsx::v1::Extractor::remove_redundant_antigen_rows(warn_if_not_found winf)
{
    const auto cell_is_number_equal_to = [](const auto& cell, long num) {
        return std::visit(
            [num]<typename Content>(const Content& val) {
                if constexpr (std::is_same_v<Content, long>)
                    return val == num;
                else
                    return false;
            },
            cell);
    };

    // VIDRL has row with serum indexes
    const auto are_titers_increasing_numers = [this, cell_is_number_equal_to](nrow_t row) {
        long num{1};
        for (const auto col : serum_columns_) {
            if (!cell_is_number_equal_to(sheet().cell(row, col), num))
                return false;
            ++num;
        }
        return true;
    };

    if (antigen_name_column_.has_value()) {
        AD_INFO("Antigen name column: {}", *antigen_name_column_);
        // remote antigen rows that have no name
        ranges::actions::remove_if(antigen_rows_, [this, are_titers_increasing_numers, winf](nrow_t row) {
            const auto no_name = !is_virus_name(row, *antigen_name_column_);
            if (no_name && !are_titers_increasing_numers(row))
                AD_WARNING(winf == warn_if_not_found::yes, "row {} has titers but no name: {}", row, sheet().cell(row, *antigen_name_column_));
            return no_name;
        });
    }
    else
        AD_WARNING("Extractor::remove_redundant_antigen_rows: no antigen_name_column");

} // ae::xlsx::v1::Extractor::remove_redundant_antigen_rows

// ----------------------------------------------------------------------

template <typename F> inline std::optional<ae::xlsx::ncol_t> find_column(const ae::xlsx::Sheet& sheet, const std::vector<ae::xlsx::nrow_t>& rows, F valid_cell)
{
    std::vector<std::tuple<ae::xlsx::ncol_t, ssize_t>> number_per_column;
    for (ae::xlsx::ncol_t col{0}; col < sheet.number_of_columns(); ++col) {
        if (const auto number = ranges::count_if(rows, [col, &valid_cell, &sheet](ae::xlsx::nrow_t row) { return valid_cell(sheet.cell(row, col)); }); number > 0)
            number_per_column.emplace_back(col, number);
    }
    if (!number_per_column.empty())
        return std::get<ae::xlsx::ncol_t>(*ranges::max_element(number_per_column, [](const auto& e1, const auto& e2) { return std::get<ssize_t>(e1) < std::get<ssize_t>(e2); }));
    else
        return std::nullopt;
}

// ----------------------------------------------------------------------

void ae::xlsx::v1::Extractor::find_antigen_date_column(warn_if_not_found winf)
{
    const auto is_date = [](const auto& cell) {
        // VIDRL uses string values DD/MM/YYYY for antigen dates
        return ae::xlsx::is_date(cell) || (ae::xlsx::is_string(cell) && ae::date::from_string(fmt::format("{}", cell), ae::date::allow_incomplete::no, ae::date::throw_on_error::no).ok());
    };

    antigen_date_column_ = ::find_column(sheet(), antigen_rows_, is_date);
    if (antigen_date_column_.has_value())
        AD_INFO("Antigen date column: {}", *antigen_date_column_);
    else
        AD_WARNING(winf == warn_if_not_found::yes, "Antigen date column not found");

} // ae::xlsx::v1::Extractor::find_antigen_date_column

// ----------------------------------------------------------------------

// bool ae::xlsx::v1::Extractor::is_passage(nrow_t row, ncol_t col) const
// {
//     return sheet().matches(re_antigen_passage, row, col);

// } // ae::xlsx::v1::Extractor::is_passage

// ----------------------------------------------------------------------

void ae::xlsx::v1::Extractor::find_antigen_passage_column(warn_if_not_found winf)
{
    antigen_passage_column_ = ::find_column(sheet(), antigen_rows_, [](const auto& cell) { return ae::virus::passage::is_good(fmt::format("{}", cell)); });
    if (antigen_passage_column_.has_value())
        AD_INFO("Antigen passage column: {}", *antigen_passage_column_);
    else
        AD_WARNING(winf == warn_if_not_found::yes, "Antigen passage column not found");

} // ae::xlsx::v1::Extractor::find_antigen_passage_column

// ----------------------------------------------------------------------

void ae::xlsx::v1::Extractor::find_antigen_lab_id_column(warn_if_not_found winf)
{
    antigen_lab_id_column_ = ::find_column(sheet(), antigen_rows_, [this](const auto& cell) { return is_lab_id(cell); });
    if (antigen_lab_id_column_.has_value())
        AD_INFO("Antigen lab_id column: {}", *antigen_lab_id_column_);
    else
        AD_WARNING(winf == warn_if_not_found::yes, "Antigen lab_id column not found");

} // ae::xlsx::v1::Extractor::find_antigen_lab_id_column

// ----------------------------------------------------------------------

std::optional<ae::xlsx::v1::nrow_t> ae::xlsx::v1::Extractor::find_serum_row(const std::regex& re, std::string_view row_name, warn_if_not_found winf, std::optional<nrow_t> ignore) const
{
    std::optional<nrow_t> found;
    for (nrow_t row{1}; row < antigen_rows()[0]; ++row) {
        if (!ignore || row != *ignore) {
            if (const auto num_columns = static_cast<size_t>(ranges::count_if(serum_columns(), [row, this, re](ncol_t col) { return sheet().matches(re, row, col); }));
                num_columns >= (number_of_sera() / 2)) {
                found = row;
                break;
            }
            else if (num_columns > 0) {
                if (row_name == "id")
                    AD_WARNING("find_serum_row {} (too few columns): row:{} columns:{} number of sera: {}", row_name, row, num_columns, number_of_sera());
            }
        }
    }

    if (found.has_value())
        AD_INFO("[{}]: Serum {} row: {}", lab(), row_name, *found);
    else
        AD_WARNING(winf == warn_if_not_found::yes, "[{}]: Serum {} row not found", lab(), row_name);
    return found;

} // ae::xlsx::v1::Extractor::find_serum_row

// ----------------------------------------------------------------------

bool ae::xlsx::v1::Extractor::is_control_serum_cell(const cell_t& cell) const
{
    if (is_string(cell)) {
        const auto text = fmt::format("{}", cell);
        if (std::regex_search(text, re_human_who_serum))
            return true;
    }
    return false;

} // ae::xlsx::v1::Extractor::is_control_serum_cell

// ----------------------------------------------------------------------

std::string ae::xlsx::v1::Extractor::make_date(const std::string& src) const
{
    // AD_DEBUG("make_date \"{}\"", src);
    if (!src.empty())
        return ae::date::parse_and_format(src, ae::date::allow_incomplete::no, ae::date::throw_on_error::yes, lab() == "CDC" ? ae::date::month_first::yes : ae::date::month_first::no);
    else
        return src;

} // ae::xlsx::v1::Extractor::make_date

// ----------------------------------------------------------------------

std::string ae::xlsx::v1::Extractor::make_passage(const std::string& src) const
{
    ae::virus::Passage passage(src);
    if (!passage.good()) {
        AD_WARNING("passage parsing failed: \"{}\" -> \"{}\"", src, passage);
        return src;
    }
    else
        return passage;

} // ae::xlsx::v1::Extractor::make_passage

// ----------------------------------------------------------------------

std::string ae::xlsx::v1::Extractor::make_lab_id(const std::string& src) const
{
    return src;

} // ae::xlsx::v1::Extractor::make_lab_id

// ----------------------------------------------------------------------

void ae::xlsx::v1::Extractor::force_serum_name_row(nrow_t /*row*/)
{
    AD_WARNING("[{}]: forcing serum name row unsupported by this extractor", lab());

} // ae::xlsx::v1::Extractor::force_serum_name_row

// ----------------------------------------------------------------------

void ae::xlsx::v1::Extractor::force_serum_passage_row(nrow_t /*row*/)
{
    AD_WARNING("[{}]: forcing serum passage row unsupported by this extractor", lab());

} // ae::xlsx::v1::Extractor::force_serum_passage_row

// ----------------------------------------------------------------------

void ae::xlsx::v1::Extractor::force_serum_id_row(nrow_t /*row*/)
{
    AD_WARNING("[{}]: forcing serum id row unsupported by this extractor", lab());

} // ae::xlsx::v1::Extractor::force_serum_id_row

// ----------------------------------------------------------------------

std::string ae::xlsx::v1::Extractor::format_assay_data(std::string_view format) const
{
    using namespace fmt::literals;

    const auto assay_rbc = [this]() -> std::string {
        if (const auto assy = assay(); assy == "HI")
            return fmt::format("hi-{}", ae::string::lowercase(rbc()));
        else
            return ae::string::lowercase(assy);
        // else if (assay == "HINT")
        //     return "hint";
        // else
        //     return "neut";
    };

    return fmt::format(fmt::runtime(format),                     //
                       "virus_type"_a = subtype(),               //
                       "lineage"_a = lineage(),                  //
                       "virus_type_lineage"_a = subtype_short(), //
                       // "subset"_a = ,
                       "virus_type_lineage_subset_short_low"_a = subtype_short(), //
                       "assay_full"_a = assay(),                                  //
                       "assay"_a = assay(),                                       //
                       "assay_low"_a = ae::string::lowercase(assay()),            //
                       "assay_low_rbc"_a = assay_rbc(),                           //
                       "lab"_a = lab(),                                           //
                       "lab_low"_a = ae::string::lowercase(lab()),                //
                       "rbc"_a = rbc(),                                           //
                       "table_date"_a = date()                                    //
    );
}

// ----------------------------------------------------------------------

ae::xlsx::v1::ExtractorCDC::ExtractorCDC(std::shared_ptr<Sheet> a_sheet)
    : Extractor(a_sheet)
{
    lab("CDC");

} // ae::xlsx::v1::ExtractorCDC::ExtractorCDC

// ----------------------------------------------------------------------

std::string ae::xlsx::v1::ExtractorCDC::titer(size_t ag_no, size_t sr_no) const
{
    auto result = Extractor::titer(ag_no, sr_no);
    if (result == "5")
        result = "<10";
    return result;

} // ae::xlsx::v1::ExtractorCDC::titer

// ----------------------------------------------------------------------

bool ae::xlsx::v1::ExtractorCDC::is_lab_id(const cell_t& cell) const
{
    return sheet().matches(re_CDC_antigen_lab_id, cell);

} // ae::xlsx::v1::ExtractorCDC::is_lab_id

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorCDC::remove_redundant_antigen_rows(warn_if_not_found winf)
{
    if (antigen_name_column_.has_value()) {
        // remove CONTROL antigen rows, e.g. "INFLUENZA B CONTROL AG, YAM LINEAGE"
        ranges::actions::remove_if(antigen_rows_, [this](nrow_t row) {
            if (sheet().matches(re_CDC_antigen_control, row, *antigen_name_column_)) {
                // AD_DEBUG("CONTROL antigen removed: {}", row, sheet().cell(row, *antigen_name_column_));
                return true;
            }
            else
                return false;
        });
    }

    Extractor::remove_redundant_antigen_rows(winf);

} // ae::xlsx::v1::ExtractorCDC::remove_redundant_antigen_rows

// ----------------------------------------------------------------------

bool ae::xlsx::v1::ExtractorCDC::serum_index_matches(const cell_t& at_row, const cell_t& at_column) const
{
    if (is_empty(at_row) || is_empty(at_column))
        return false;
    return fmt::format("{}", at_row)[0] == fmt::format("{}", at_column)[0];

} // ae::xlsx::v1::ExtractorCDC::serum_index_matches

// ----------------------------------------------------------------------

ae::xlsx::v1::nrow_t ae::xlsx::v1::ExtractorCDC::find_serum_row_by_col(ncol_t col) const
{
    if (serum_index_row_.has_value() && serum_index_column_.has_value()) {
        if (const auto serum_index = sheet().cell(*serum_index_row_, col); !is_empty(serum_index)) {
            for (nrow_t row{serum_rows_[0]}; row < sheet().number_of_rows(); ++row) {
                if (const auto index_cell = sheet().cell(row, *serum_index_column_); serum_index_matches(serum_index, index_cell))
                    return row;
            }
        }
    }
    AD_WARNING("{} cannot find serum for column {}", extractor_name(), col);
    return nrow_t{max_row_col};

} // ae::xlsx::v1::ExtractorCDC::find_serum_row_by_col

// ----------------------------------------------------------------------

ae::xlsx::v1::serum_fields_t ae::xlsx::v1::ExtractorCDC::serum(size_t sr_no) const
{
    if (const auto row = find_serum_row_by_col(serum_columns().at(sr_no)); valid(row)) {

        const auto make = [this, row](std::optional<ncol_t> col) -> std::string {
            if (col.has_value()) {
                if (const auto cell = sheet().cell(row, *col); !is_empty(cell))
                    return fmt::format("{}", cell);
            }
            return {};
        };

        const auto make_serum_id = [this](const std::string& src) {
            if (!src.empty() && !ae::string::startswith(src, lab()))
                return fmt::format("{} {}", lab(), src);
            else
                return src;
        };

        return {.name = make(serum_name_column_),                     //
                .serum_id = make_serum_id(make(serum_id_column_)),    //
                .passage = make_passage(make(serum_passage_column_)), //
                .species = make(serum_species_column_),               //
                .conc = make(serum_conc_column_),                     //
                .dilut = make(serum_dilut_column_),                   //
                .boosted = serum_boosted_column_.has_value() && !is_empty(sheet().cell(row, *serum_boosted_column_)) && fmt::format("{}", sheet().cell(row, *serum_boosted_column_))[0] == 'Y'};
    }
    else
        return {};

} // ae::xlsx::v1::ExtractorCDC::serum

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorCDC::find_serum_rows(warn_if_not_found winf)
{
    find_serum_index_row(winf, re_CDC_serum_index);
    find_serum_name_column(winf, re_CDC_serum_index);
    find_serum_columns(winf);

} // ae::xlsx::v1::ExtractorCDC::find_serum_rows

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorCDC::find_serum_index_row(warn_if_not_found winf, const std::regex& re_serum_index)
{
    fmt::memory_buffer report;
    for (nrow_t row{1}; row < antigen_rows()[0]; ++row) {
        if (const size_t matches = static_cast<size_t>(ranges::count_if(serum_columns(), [row, re_serum_index, this](ncol_t col) { return sheet().matches(re_serum_index, row, col); }));
            matches == number_of_sera()) {
            serum_index_row_ = row;
            break;
        }
        else if (matches)
            fmt::format_to(std::back_inserter(report), "    re_serum_index row:{} matches:{}\n", row, matches);
    }

    if (serum_index_row_.has_value())
        AD_INFO("{} Serum index row: {}", extractor_name(), serum_index_row_);
    else
        AD_WARNING(winf == warn_if_not_found::yes, "{} No serum index row found (number of sera: {})\n{}", extractor_name(), number_of_sera(), fmt::to_string(report));

} // ae::xlsx::v1::ExtractorCDC::find_serum_index_row

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorCDC::find_serum_name_column(warn_if_not_found winf, const std::regex& re_serum_index)
{
    for (ncol_t col{0}; col < ncol_t{5} && !serum_name_column_; ++col) {
        serum_rows_.clear();
        for (nrow_t row{antigen_rows().back() + nrow_t{1}}; row < sheet().number_of_rows(); ++row) {
            if (is_virus_name(row, col))
                serum_rows_.push_back(row);
        }
        if (serum_rows_.size() > (serum_columns().size() / 2))
            serum_name_column_ = col;
    }

    if (!serum_name_column_.has_value()) {
        AD_WARNING(winf == warn_if_not_found::yes, "serum name column not found");
        return;
    }

    AD_INFO("{} Serum name column: {}", extractor_name(), serum_name_column_);

    if (*serum_name_column_ > ncol_t{0}) {
        serum_index_column_ = *serum_name_column_ - ncol_t{1};
        for (const nrow_t row : serum_rows_) {
            if (!sheet().matches(re_serum_index, row, *serum_index_column_))
                AD_WARNING("{} unrecognized serum index at {}{}: \"{}\" for serum \"{}\"", extractor_name(), row, serum_index_column_, sheet().cell(row, *serum_index_column_),
                           sheet().cell(row, *serum_name_column_));
        }
    }
    else
        AD_WARNING("{} unexpected serum name column: {} -> no place for serum indexes", extractor_name(), serum_name_column_);

} // ae::xlsx::v1::ExtractorCDC::find_serum_name_column

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorCDC::find_serum_columns(warn_if_not_found /*winf*/)
{
    find_serum_column_label(re_CDC_lot_label, serum_id_column_, "LOT");
    find_serum_column_label(re_CDC_species_label, serum_species_column_, "SPECIES");
    find_serum_column_label(re_CDC_boosted_label, serum_boosted_column_, "BOOSTED");
    find_serum_column_label(re_CDC_conc_label, serum_conc_column_, "CONC");
    find_serum_column_label(re_CDC_dilut_label, serum_dilut_column_, "DILUT");
    find_serum_column_label(re_CDC_passage_label, serum_passage_column_, "PASSAGE");
    find_serum_column_label(re_CDC_pool_label, serum_pool_column_, "POOL");

    if (const auto matches1 = sheet().grep(re_CDC_date_treated_label, {serum_rows_[0] - nrow_t{1}, *serum_name_column_ + ncol_t{1}}, {serum_rows_[0], sheet().number_of_columns()});
        matches1.size() == 1) {
        serum_treated_column_ = matches1[0].col;
    }
    else if (const auto matches2 = sheet().grep(re_CDC_treated_label, {serum_rows_[0] - nrow_t{1}, *serum_name_column_ + ncol_t{1}}, {serum_rows_[0], sheet().number_of_columns()});
             !matches2.empty()) {
        for (const auto& mm2 : matches2) {
            if (sheet().matches(re_CDC_date_label, serum_rows_[0] - nrow_t{2}, mm2.col)) {
                serum_treated_column_ = mm2.col;
                break;
            }
        }
    }

} // ae::xlsx::v1::ExtractorCDC::find_serum_columns

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorCDC::find_serum_column_label(const std::regex& re, std::optional<ncol_t>& col, std::string_view label_name)
{
    if (const auto matches = sheet().grep(re, {serum_rows_[0] - nrow_t{1}, *serum_name_column_ + ncol_t{1}}, {serum_rows_[0], sheet().number_of_columns()}); matches.size() == 1)
        col = matches[0].col;
    else if (matches.size() > 1)
        AD_WARNING("{} unclear {} label matches: {}", extractor_name(), label_name, matches);

} // ae::xlsx::v1::ExtractorCDC::find_serum_column_label

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorCDC::find_serum_column_label(const std::regex& re1, const std::regex& re2, std::optional<ncol_t>& col, std::string_view label_name)
{
    const cell_addr_t min{serum_rows_[0] - nrow_t{2}, *serum_name_column_ + ncol_t{1}}, max{serum_rows_[0], sheet().number_of_columns()};
    if (const auto matches = sheet().grepv(re1, re2, min, max); matches.size() == 1)
        col = matches[0].col;
    else if (matches.size() > 1)
        AD_WARNING("{} unclear {} label matches: {}", extractor_name(), label_name, matches);
    else
        AD_WARNING("{} serum column {} not found in {}-{}", extractor_name(), label_name, min, max);

} // ae::xlsx::v1::ExtractorCDC::find_serum_column_label

// ----------------------------------------------------------------------

bool ae::xlsx::v1::ExtractorCDC::valid_titer_row(nrow_t row, const column_range& /*cr*/) const
{
    return sheet().grep(re_CDC_serum_control, {row, ncol_t{0}}, {row + nrow_t{1}, sheet().number_of_columns()}).empty()
        && row > nrow_t{3};     // at least 4 rows must be above (sheet title, clade, passage, serum index)

} // ae::xlsx::v1::ExtractorCDC::valid_titer_row

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorCDC::exclude_control_sera(warn_if_not_found /*winf*/)
{
    if (serum_index_row_.has_value() && serum_index_column_.has_value() && serum_name_column_.has_value()) {
        const auto exclude = [this](ncol_t col) {
            if (const auto row = find_serum_row_by_col(col); valid(row)) {
                if (const auto cell = sheet().cell(row, *serum_name_column_); is_control_serum_cell(cell)) {
                    AD_INFO("[{}]: serum excluded (HUMAN or WHO or NORMAL serum): \"{}\"", lab(), cell);
                    return true;
                }
                else
                    return false;
            }
            else
                return true;
        };

        ranges::actions::remove_if(serum_columns_, exclude);
    }

} // ae::xlsx::v1::ExtractorCDC::exclude_control_sera

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorCDC::adjust_titer_range(nrow_t row, column_range& cr)
{
    if (cr.valid()) {
        while (cr.second >= cr.first && !sheet().grep(re_CDC_titer_label, {nrow_t{0}, cr.second}, {row, cr.second + ncol_t{1}}).empty()) // ignore TITER and BACK TITER columns
            --cr.second;
        if (cr.second >= cr.first && !sheet().grep(re_CDC_ha_group_label, {nrow_t{0}, cr.first}, {row, cr.first + ncol_t{1}}).empty()) // ignore HA GROUP looking like titer
            ++cr.first;
    }

} // ae::xlsx::v1::ExtractorCDC::adjust_titer_range

// ----------------------------------------------------------------------

std::string ae::xlsx::v1::ExtractorCDC::report_serum_anchors() const
{
    return fmt::format("  Serum rows/columns:\n    Index:   {} -> {}\n    Rows:    {}\n    Name:    {}\n    Id:      {}\n"            //
                       "    Treated: {}\n    Species: {}\n    Boosted: {}\n    Conc:    {}\n    Dilut:   {}\n"                        //
                       "    Passage: {}\n    Pool:    {}\n  Serum columns:   {}\n  Number of sera: {}\n",                                 //
                       serum_index_row_, serum_index_column_, format(make_ranges(serum_rows_)), serum_name_column_, serum_id_column_, //
                       serum_treated_column_, serum_species_column_, serum_boosted_column_, serum_conc_column_, serum_dilut_column_,  //
                       serum_passage_column_, serum_pool_column_, format(make_ranges(serum_columns_)), serum_columns_.size());

} // ae::xlsx::v1::ExtractorCDC::report_serum_anchors

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorCDC::check_export_possibility() const
{
    std::string msg;
    try {
        Extractor::check_export_possibility();
    }
    catch (Error& err) {
        msg = err.what();
    }

    if (!serum_name_column_.has_value())
        msg += " [no serum name column]";
    if (serum_rows_.size() < 3)
        msg += fmt::format(" [too few serum rows detetcted: {}]", serum_rows_);

    if (!msg.empty())
        throw Error(msg);

} // ae::xlsx::v1::ExtractorCDC::check_export_possibility

// ----------------------------------------------------------------------

ae::xlsx::v1::ExtractorAc21::ExtractorAc21(std::shared_ptr<Sheet> a_sheet)
    : ExtractorCDC(a_sheet)
{
    lab("CNIC");

} // ae::xlsx::v1::ExtractorCDC::ExtractorAc21

// ----------------------------------------------------------------------

// bool ae::xlsx::v1::ExtractorAc21::is_lab_id(const cell_t& cell) const
// {
//     return true;

// } // ae::xlsx::v1::ExtractorAc21::is_lab_id

// ----------------------------------------------------------------------

bool ae::xlsx::v1::ExtractorAc21::serum_index_matches(const cell_t& at_row, const cell_t& at_column) const
{
    return !is_empty(at_row) && fmt::format("{}", at_row) == fmt::format("{}", at_column);

} // ae::xlsx::v1::ExtractorAc21::serum_index_matches

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorAc21::find_antigen_lab_id_column(warn_if_not_found winf)
{
    if (const auto found = sheet().grep(re_AC21_ID_label, {nrow_t{5}, ncol_t{1}}, {antigen_rows_.front(), sheet().number_of_columns()}); !found.empty()) {
        for (const auto& cell_match : found) {
            if (fmt::format("{}", sheet().cell(cell_match.row - nrow_t{1}, cell_match.col)) == "Strain") {
                antigen_lab_id_column_ = cell_match.col;
                break;
            }
        }
    }
    if (!antigen_lab_id_column_)
        AD_WARNING(winf == warn_if_not_found::yes, "Antigen lab_id column not found");

} // ae::xlsx::v1::ExtractorAc21::find_antigen_lab_id_column

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorAc21::find_serum_rows(warn_if_not_found winf)
{
    find_serum_index_row(winf, re_AC21_serum_index);
    find_serum_name_column(winf, re_AC21_serum_index);
    find_serum_columns(winf);

} // ae::xlsx::v1::ExtractorAc21::find_serum_rows

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorAc21::find_serum_columns(warn_if_not_found /*winf*/)
{
    find_serum_column_label(re_AC21_serum_label, re_AC21_ID_label, serum_id_column_, "SERUM-ID");
    find_serum_column_label(re_AC21_serum_label, re_AC21_species_label, serum_species_column_, "SPECIES");

} // ae::xlsx::v1::ExtractorAc21::find_serum_columns

// ----------------------------------------------------------------------

ae::xlsx::v1::serum_fields_t ae::xlsx::v1::ExtractorWithSerumRowsAbove::serum(size_t sr_no) const
{
    const auto make = [this, col = serum_columns().at(sr_no)](std::optional<nrow_t> row) -> std::string {
        if (row.has_value()) {
            if (const auto cell = sheet().cell(*row, col); !is_empty(cell))
                return fmt::format("{}", cell);
        }
        return {};
    };

    return serum_fields_t{
        // .serum_name is set by lab specific extractor
        .serum_id = make(serum_id_row()),                  //
        .passage = make_passage(make(serum_passage_row())) //
    };

} // ae::xlsx::v1::ExtractorWithSerumRowsAbove::serum

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorWithSerumRowsAbove::exclude_control_sera(warn_if_not_found /*winf*/)
{
    ranges::actions::remove_if(serum_columns_, [this](ncol_t col) {
        if ((serum_name_row_.has_value() && is_control_serum_cell(sheet().cell(*serum_name_row_, col))) || (serum_id_row_.has_value() && is_control_serum_cell(sheet().cell(*serum_id_row_, col))) || (serum_passage_row_.has_value() && is_control_serum_cell(sheet().cell(*serum_passage_row_, col)))) {
            AD_INFO("[{}]: serum column {} excluded: HUMAN or WHO or NORMAL serum", lab(), col);
            return true;
        }
        return false;
    });

} // ae::xlsx::v1::ExtractorWithSerumRowsAbove::exclude_control_sera

// ----------------------------------------------------------------------

std::string ae::xlsx::v1::ExtractorWithSerumRowsAbove::report_serum_anchors() const
{
    return fmt::format("  Serum rows:\n    Name:    {}\n    Passage: {}\n    Id:      {}\nSerum columns:   {}\n  Number of sera: {}\n", //
                       serum_name_row_, serum_passage_row_, serum_id_row_, format(make_ranges(serum_columns_)), serum_columns_.size());

} // ae::xlsx::v1::ExtractorWithSerumRowsAbove::report_serum_anchors

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorWithSerumRowsAbove::force_serum_name_row(nrow_t row)
{
    AD_INFO("forced serum name row: {}", row);
    serum_name_row_ = row;

} // ae::xlsx::v1::ExtractorWithSerumRowsAbove::force_serum_name_row

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorWithSerumRowsAbove::force_serum_passage_row(nrow_t row)
{
    AD_INFO("forced serum passage row: {}", row);
    serum_passage_row_ = row;

} // ae::xlsx::v1::ExtractorWithSerumRowsAbove::force_serum_passage_row

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorWithSerumRowsAbove::force_serum_id_row(nrow_t row)
{
    AD_INFO("forced serum id row: {}", row);
    serum_id_row_ = row;

} // ae::xlsx::v1::ExtractorWithSerumRowsAbove::force_serum_id_row

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorWithSerumRowsAbove::check_export_possibility() const
{
    std::string msg;
    try {
        Extractor::check_export_possibility();
    }
    catch (Error& err) {
        msg = err.what();
    }

    if (!serum_name_row_.has_value())
        msg += " [no serum name row]";
    if (serum_columns_.size() < 2)
        msg += fmt::format(" [too few serum columns detetcted: {}]", serum_columns_);

    if (!msg.empty())
        throw Error(msg);

} // ae::xlsx::v1::ExtractorWithSerumRowsAbove::check_export_possibility

// ----------------------------------------------------------------------

ae::xlsx::v1::ExtractorCrick::ExtractorCrick(std::shared_ptr<Sheet> a_sheet)
    : ExtractorWithSerumRowsAbove(a_sheet)
{
    lab("CRICK");

} // ae::xlsx::v1::ExtractorCrick::ExtractorCrick

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorCrick::find_serum_rows(warn_if_not_found winf)
{
    find_serum_name_rows(winf);
    find_serum_passage_row(re_serum_passage, winf);
    find_serum_id_row(re_CRICK_serum_id, winf);
    find_serum_less_than_substitutions(winf);

} // ae::xlsx::v1::ExtractorCrick::find_serum_rows


// ======================================================================

void ae::xlsx::v1::ExtractorCrick::find_serum_name_rows(warn_if_not_found winf)
{
    fmt::memory_buffer report;
    const auto number_of_sera_threshold = number_of_sera() / 3 * 2;
    for (nrow_t row{1}; row < antigen_rows()[0]; ++row) {
        if (const size_t matches = static_cast<size_t>(ranges::count_if(serum_columns(), [row, this](ncol_t col) { return sheet().matches(re_CRICK_serum_name_1, row, col); }));
            matches > number_of_sera_threshold) {
            serum_name_1_row_ = row;
            break;
        }
        else if (matches)
            fmt::format_to(std::back_inserter(report), "    re_CRICK_serum_name_1 row:{} matches:{}\n", row, matches);
    }

    if (serum_name_1_row_.has_value())
        AD_INFO("[{}]: Serum name row 1: {}", lab(), serum_name_1_row_);
    else
        AD_WARNING(winf == warn_if_not_found::yes, "[{}]: No serum name row 1 found (number of sera: {})\n{}", lab(), number_of_sera(), fmt::to_string(report));

    if (serum_name_1_row_.has_value() &&
        static_cast<size_t>(ranges::count_if(serum_columns(), [this](ncol_t col) { return sheet().matches(re_CRICK_serum_name_2, *serum_name_1_row_ + nrow_t{1}, col); })) > number_of_sera_threshold)
        serum_name_2_row_ = *serum_name_1_row_ + nrow_t{1};
    else
        AD_DEBUG("re_CRICK_serum_name_2 {}: {}", *serum_name_1_row_ + nrow_t{1},
                 static_cast<size_t>(ranges::count_if(serum_columns(), [this](ncol_t col) { return sheet().matches(re_CRICK_serum_name_2, *serum_name_1_row_ + nrow_t{1}, col); })));

    if (serum_name_2_row_.has_value())
        AD_INFO("[{}]: Serum name row 2: {}", lab(), serum_name_2_row_);
    else
        AD_WARNING(winf == warn_if_not_found::yes, "[{}]: No serum name row 2 found", lab());

} // ae::xlsx::v1::ExtractorCrick::find_serum_name_rows

// ----------------------------------------------------------------------

const std::string& ae::xlsx::v1::ExtractorCrick::get_footnote(const std::string& key, const std::string& if_not_found) const
{
    for (const auto& [e1, e2] : footnote_index_subst_) {
        if (e1 == key)
            return e2;
    }
    return if_not_found;
}

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorCrick::find_serum_less_than_substitutions(warn_if_not_found winf)
{
    if (!antigen_rows_.empty()) {
        if (const auto found = sheet().grep(re_CRICK_less_than, {antigen_rows_.back(), ncol_t{1}}, {sheet().number_of_rows(), ncol_t{2}}); !found.empty()) {
            for (const auto& cell_match : found)
                footnote_index_subst_.emplace_back(ae::string::strip(fmt::format("{}", sheet().cell(cell_match.row, cell_match.col - ncol_t{1}))), cell_match.matches[1]);
        }
        else if (const auto found2 = sheet().grep(re_CRICK_less_than_multi, {antigen_rows_.back(), ncol_t{1}}, {sheet().number_of_rows(), ncol_t{2}}); !found2.empty()) {
            // AD_DEBUG("[Crick]: less than subst (multi): {}", sheet().cell(found2[0].row, found2[0].col));
            const auto cell = fmt::format("{}", sheet().cell(found2[0].row, found2[0].col)); // do not move inside split below, cannot survive within loop
            const auto split = [&cell]() {
                if (cell.find(";") != std::string::npos)
                    return ae::string::split(cell, ";");
                else
                    return ae::string::split(cell, ",");
            };

            for (const auto& entry : split()) {
                if (std::cmatch match; ae::regex::search(entry, match, re_CRICK_less_than_multi_entry))
                    footnote_index_subst_.emplace_back(match[1], match[2]);
            }
        }
        else if (const auto found3 = sheet().grep(re_CRICK_less_than_2, {antigen_rows_.back(), ncol_t{1}}, {sheet().number_of_rows(), ncol_t{2}}); !found3.empty()) {
            for (const auto& cell_match : found3)
                footnote_index_subst_.emplace_back(cell_match.matches[1], cell_match.matches[2]);
        }
        else
            AD_WARNING(winf == warn_if_not_found::yes, "[{}]: No less than substitution footnote", lab());

        if (!footnote_index_subst_.empty()) {
            AD_INFO("[{}]: less than subst: {}", lab(), footnote_index_subst_);
            serum_less_than_substitutions_.resize(number_of_sera(), "<");
            if (serum_id_row_.has_value()) {
                for (const auto sr_no : range_from_0_to(number_of_sera())) {
                    const auto cell = sheet().cell(*serum_id_row_, serum_columns().at(sr_no));
                    if (std::smatch match; sheet().matches(re_CRICK_serum_id, match, cell)) {
                        serum_less_than_substitutions_[sr_no] = get_footnote(match.str(2), std::string{"<"});
                        AD_INFO("[{}]:     SR: {} replacing \"<\" with \"{}\" (serum id footnote match: \"{}\")", lab(), sr_no, serum_less_than_substitutions_[sr_no], match.str(2));
                    }
                }
            }
        }
    }

} // ae::xlsx::v1::ExtractorCrick::find_serum_less_than_substitutions

// ----------------------------------------------------------------------

ae::xlsx::v1::serum_fields_t ae::xlsx::v1::ExtractorCrick::serum(size_t sr_no) const
{
    auto serum = ExtractorWithSerumRowsAbove::serum(sr_no);
    if (serum_name_1_row_ && serum_name_2_row_) {
        const auto n1{fmt::format("{}", sheet().cell(*serum_name_1_row_, serum_columns().at(sr_no)))}, n2{fmt::format("{}", sheet().cell(*serum_name_2_row_, serum_columns().at(sr_no)))};
        if (n1.size() > 2 && n1[1] == '/')
            serum.name = fmt::format("{}/{}", n1, n2);
        else
            serum.name = fmt::format("{} {}", n1, n2);
    }
    else
        serum.name = "*no serum_name_[12]_row_*";

    if (std::smatch match; std::regex_match(serum.serum_id, match, re_CRICK_serum_id)) {
        // move to whocc-tables/*crick/whocc-xlsx-to-torg.py
        serum.serum_id = ae::string::uppercase(ae::string::replace(match.str(1), " ", "", ",", "/")); // remove spaces, replace , with /, e.g. "Sh  539, 540, 543, 544, 570, 571, 574" -> "SH539/540/543/544/570/571/574"
    }

    return serum;

} // ae::xlsx::v1::ExtractorCrick::serum

// ----------------------------------------------------------------------

std::string ae::xlsx::v1::ExtractorCrick::titer(size_t ag_no, size_t sr_no) const
{
    auto result = ExtractorWithSerumRowsAbove::titer(ag_no, sr_no);
    if ((result == "<" || result == ">") && sr_no < serum_less_than_substitutions_.size())
        result = serum_less_than_substitutions_[sr_no];
    else if (result == "ND")
        result = "*";
    return result;

} // ae::xlsx::v1::ExtractorCrick::titer

// ----------------------------------------------------------------------

std::string ae::xlsx::v1::ExtractorCrick::report_serum_anchors() const
{
    return fmt::format("  Serum rows:\n    Name:      {}+{}\n    Passage:   {}\n    Id:        {}\n    Less than: {}\nSerum columns:   {}\n  Number of sera: {}\n", //
                       serum_name_1_row_, serum_name_2_row_, serum_passage_row_, serum_id_row_, footnote_index_subst_, format(make_ranges(serum_columns_)), serum_columns_.size());

} // ae::xlsx::v1::ExtractorCrick::report_serum_anchors

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorCrick::check_export_possibility() const
{
    std::string msg;
    try {
        // NO ExtractorWithSerumRowsAbove::check_export_possibility!
        Extractor::check_export_possibility();
    }
    catch (Error& err) {
        msg = err.what();
    }

    if (!serum_name_1_row_.has_value() ||!serum_name_2_row_.has_value())
        msg += " [no serum name rows]";
    if (serum_columns_.size() < 2)
        msg += fmt::format(" [too few serum columns detetcted: {}]", serum_columns_);

    if (!msg.empty())
        throw Error(msg);

} // ae::xlsx::v1::ExtractorCrick::check_export_possibility

// ----------------------------------------------------------------------

ae::xlsx::v1::ExtractorCrickPRN::ExtractorCrickPRN(std::shared_ptr<Sheet> a_sheet)
    : ExtractorCrick(a_sheet)
{
    assay("PRN");
    subtype("A(H3N2)");

} // ae::xlsx::v1::ExtractorCrick::ExtractorCrick

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorCrickPRN::find_serum_rows(warn_if_not_found winf)
{
    find_two_fold_read_row();
    ExtractorCrick::find_serum_rows(winf);

} // ae::xlsx::v1::ExtractorCrickPRN::find_serum_rows

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorCrickPRN::find_two_fold_read_row()
{
    for (nrow_t row{1}; row < antigen_rows()[0]; ++row) {
        const size_t two_fold_matches = static_cast<size_t>(ranges::count_if(serum_columns(), [row, this](ncol_t col) { return sheet().matches(re_CRICK_prn_2fold, row, col); }));
        const size_t read_matches = static_cast<size_t>(ranges::count_if(serum_columns(), [row, this](ncol_t col) { return sheet().matches(re_CRICK_prn_read, row, col); }));
        if (two_fold_matches == (serum_columns().size() / 2) && two_fold_matches == read_matches) {
            two_fold_read_row_ = row;
            break;
        }
    }

    if (two_fold_read_row_.has_value()) {
        ncol_t col_no{0};
        ranges::actions::remove_if(serum_columns(), [&col_no](auto) { const auto remove = (*col_no % 2) != 0; ++col_no; return remove; });
        AD_INFO("[{} PRN]: 2-fold read row: {}", lab(), *two_fold_read_row_);
    }

} // ae::xlsx::v1::ExtractorCrickPRN::find_two_fold_read_row

// ----------------------------------------------------------------------

std::string ae::xlsx::v1::ExtractorCrickPRN::titer_comment() const
{
    if (two_fold_read_row_.has_value())
        return "<hi-like-titer> / <PRN read titer>";
    else
        return {};

} // ae::xlsx::v1::ExtractorCrickPRN::titer_comment

// ----------------------------------------------------------------------

std::string ae::xlsx::v1::ExtractorCrickPRN::titer(size_t ag_no, size_t sr_no) const
{
    using namespace std::string_view_literals;
    if (two_fold_read_row_.has_value()) {
        const auto left_col = serum_columns().at(sr_no);
        const auto two_fold_col = sheet().matches(re_CRICK_prn_2fold, *two_fold_read_row_, left_col) ? left_col : ncol_t{left_col + ncol_t{1}};
        const auto read_col = two_fold_col == left_col ? ncol_t{left_col + ncol_t{1}} : left_col;

        // interpretaion of < in the Crick PRN tables is not quite
        // clear, we just put < into togr and then converting it to
        // <10, <20, <40 when converting torg to ace

        const auto extract = [](const auto& cell) {
            return std::visit(
                [&cell]<typename Content>(const Content& cont) {
                    if constexpr (std::is_same_v<Content, std::string>)
                        return cont;
                    else if constexpr (std::is_same_v<Content, long>)
                        return fmt::format("{}", cont);
                    else if constexpr (std::is_same_v<Content, double>)
                        return fmt::format("{}", std::lround(cont));
                    else
                        return fmt::format("{}", cell);
                },
                cell);
        };

        return fmt::format("{}/{}", extract(sheet().cell(antigen_rows().at(ag_no), two_fold_col)), extract(sheet().cell(antigen_rows().at(ag_no), read_col)));
    }
    else
        return ExtractorCrick::titer(ag_no, sr_no);

} // ae::xlsx::v1::ExtractorCrickPRN::titer

// ----------------------------------------------------------------------

ae::xlsx::v1::ExtractorNIID::ExtractorNIID(std::shared_ptr<Sheet> a_sheet)
    : ExtractorWithSerumRowsAbove(a_sheet)
{
    lab("NIID");

} // ae::xlsx::v1::ExtractorNIID::ExtractorNIID

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorNIID::find_antigen_lab_id_column(warn_if_not_found winf)
{
    if (const auto matches = sheet().grep(re_NIID_lab_id_label, {nrow_t{0}, ncol_t{0}}, {nrow_t{10}, ncol_t{2}}); matches.size() == 1) {
        antigen_lab_id_column_ = matches[0].col;
    }

    if (antigen_lab_id_column_.has_value())
        AD_INFO("[{}]: Antigen lab_id column: {}", lab(), *antigen_lab_id_column_);
    else
        AD_WARNING(winf == warn_if_not_found::yes, "[{}]: Antigen lab_id column not found", lab());


} // ae::xlsx::v1::ExtractorNIID::find_antigen_lab_id_column

// ----------------------------------------------------------------------

ae::xlsx::v1::serum_fields_t ae::xlsx::v1::ExtractorNIID::serum(size_t sr_no) const
{
    if (serum_name_row().has_value()) {
        const auto serum_designation = fmt::format("{}", sheet().cell(*serum_name_row(), serum_columns().at(sr_no)));
        if (std::smatch match; std::regex_search(serum_designation, match, re_NIID_serum_name)) {
            auto name = ae::string::replace(ae::string::uppercase(match.str(1)), '\n', ' ');
            name = std::regex_replace(name, re_NIID_serum_name_fix, "$1");
            if (name.size() > 2 && ((name[0] != 'A' && name[0] != 'B') || name[1] != '/'))
                name = fmt::format("{}/{}", subtype_without_lineage(), name);
            // AD_DEBUG("serum fields \"{}\" \"{}\"", name, match.str(2));
            std::string passage;
            if (std::smatch match_passage; std::regex_search(name, match_passage, re_NIID_serum_passage)) {
                passage = ae::string::uppercase(match_passage.str(1));
                name = match_passage.prefix();
            }
            // AD_DEBUG("serum fields2 \"{}\" \"{}\" \"{}\"", name, passage, match.str(2));
            return serum_fields_t{
                .name = name,                                                                     //
                .serum_id = ae::string::uppercase(fmt::format("{} No.{}", passage, match.str(2))), //
                .passage = passage                                                                //
            };
        }
    }
    return serum_fields_t{};

} // ae::xlsx::v1::ExtractorNIID::serum

// ----------------------------------------------------------------------

std::string ae::xlsx::v1::ExtractorNIID::titer(size_t ag_no, size_t sr_no) const
{
    auto titer = ExtractorWithSerumRowsAbove::titer(ag_no, sr_no);
    titer = ae::string::replace(titer, "\xEF\xBC\x9C", "<"); // unicode Fullwidth Less-Than Sign &#xFF1C
    return titer;

} // ae::xlsx::v1::ExtractorNIID::titer

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorNIID::find_serum_rows(warn_if_not_found winf)
{
    serum_name_row_ = find_serum_row(re_NIID_serum_name, "name", winf);

} // ae::xlsx::v1::ExtractorNIID::find_serum_rows

// ----------------------------------------------------------------------

bool ae::xlsx::v1::ExtractorNIID::is_control_serum_cell(const cell_t& cell) const
{
    if (ExtractorWithSerumRowsAbove::is_control_serum_cell(cell))
        return true;

    if (is_string(cell)) {
        const auto text = fmt::format("{}", cell);
        if (std::regex_search(text, re_NIID_serum_name_row_non_serum_label))
            return true;
    }
    return false;

} // ae::xlsx::v1::ExtractorNIID::is_control_serum_cell

// ----------------------------------------------------------------------

std::string ae::xlsx::v1::ExtractorNIID::report_serum_anchors() const
{
    return fmt::format("  Serum rows:\n    Name:    {}\nSerum columns:   {}\n  Number of sera: {}\n", serum_name_row_, format(make_ranges(serum_columns_)), serum_columns_.size());

} // ae::xlsx::v1::ExtractorNIID::report_serum_anchors

// ----------------------------------------------------------------------

ae::xlsx::v1::ExtractorVIDRL::ExtractorVIDRL(std::shared_ptr<Sheet> a_sheet)
    : ExtractorWithSerumRowsAbove(a_sheet)
{
    lab("VIDRL");

} // ae::xlsx::v1::ExtractorVIDRL::ExtractorVIDRL

// ----------------------------------------------------------------------

std::string ae::xlsx::v1::ExtractorVIDRL::make_date(const std::string& src) const
{
    if (std::regex_search(src, re_VIDRL_antigen_date_column_title))
        return {};              // column title in the antigen's row
    return ExtractorWithSerumRowsAbove::make_date(src);

} // ae::xlsx::v1::ExtractorVIDRL::make_date

// ----------------------------------------------------------------------

std::string ae::xlsx::v1::ExtractorVIDRL::make_lab_id(const std::string& src) const
{
    if (std::regex_search(src, re_VIDRL_antigen_lab_id_column_title))
        return {};              // column title in the antigen's row
    return ExtractorWithSerumRowsAbove::make_lab_id(src);

} // ae::xlsx::v1::ExtractorVIDRL::make_lab_id

// ----------------------------------------------------------------------

ae::xlsx::v1::serum_fields_t ae::xlsx::v1::ExtractorVIDRL::serum(size_t sr_no) const
{
    auto serum = ExtractorWithSerumRowsAbove::serum(sr_no);
    if (serum_name_row_) {
        serum.name = fmt::format("{}", sheet().cell(*serum_name_row_, serum_columns().at(sr_no)));

        // TAS503 -> A(H3N2)/TASMANIA/503/2020
        if (std::smatch match; std::regex_search(serum.name, match, re_VIDRL_serum_name)) {
            for (const auto ag_no : range_from_0_to(number_of_antigens())) {
                const auto antigen_name = antigen(ag_no).name;
                const auto antigen_name_fields = ae::string::split(antigen_name, "/");
                if (antigen_name_fields.size() == 4 && ae::string::startswith_ignore_case(antigen_name_fields[1], match.str(1)) && antigen_name_fields[2] == match.str(2)) {
                    serum.name = antigen_name;
                    break;
                }
            }
        }
    }
    else
        serum.name = "*no serum_name_row_*";
    return serum;

} // ae::xlsx::v1::ExtractorVIDRL::serum

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorVIDRL::adjust_titer_range(nrow_t row, column_range& cr)
{
    if (cr.valid()) {
        // VIDRL mutant table may have serum_id looking like a titer. If
        // the row has serum ids to the left or to the right of titer
        // range, invalidate the range
        // AD_DEBUG("vidrl adjust_titer_range {} {} {}", row, cr, sheet().grep(re_VIDRL_serum_id_with_days, {row, ncol_t{1}}, {row + nrow_t{1}, cr.first - ncol_t{1}}));
        if (sheet().grep(re_VIDRL_serum_id_with_days, {row, ncol_t{1}}, {row + nrow_t{1}, cr.first - ncol_t{1}}).size() > 0 ||
            sheet().grep(re_VIDRL_serum_id_with_days, {row, cr.second + ncol_t{1}}, {row + nrow_t{1}, sheet().number_of_columns()}).size() > 0) {
            cr.first = ncol_t{max_row_col};
        }
    }

} // ae::xlsx::v1::ExtractorVIDRL::adjust_titer_range

// ----------------------------------------------------------------------

bool ae::xlsx::v1::ExtractorVIDRL::is_lab_id(const cell_t& cell) const
{
    return sheet().matches(re_VIDRL_antigen_lab_id, cell);

} // ae::xlsx::v1::ExtractorVIDRL::is_lab_id

// ----------------------------------------------------------------------

void ae::xlsx::v1::ExtractorVIDRL::find_serum_rows(warn_if_not_found winf)
{
    find_serum_passage_row(re_serum_passage, winf);

    // VIDRL serum name can be confused with the passage by
    // re_VIDRL_serum_name, that is why serum_passage_row_ detected
    // first and then ignored while looking for serum name row

    serum_name_row_ = find_serum_row(re_VIDRL_serum_name, "name", winf, serum_passage_row_);
    find_serum_id_row(re_VIDRL_serum_id, winf);

} // ae::xlsx::v1::ExtractorVIDRL::find_serum_rows

// ----------------------------------------------------------------------
