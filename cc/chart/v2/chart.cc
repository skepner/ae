#include "utils/string.hh"
#include "ad/enumerate.hh"
#include "ext/range-v3.hh"
#include "ad/counter.hh"
// #include "acmacs-virus/virus-name-v1.hh"
// #include "acmacs-whocc-data/labs.hh"
#include "chart/v2/chart.hh"
#include "chart/v2/serum-circle.hh"

// ----------------------------------------------------------------------

std::string ae::chart::v2::Chart::make_info(size_t max_number_of_projections_to_show, unsigned inf) const
{
    fmt::memory_buffer text;
    fmt::format_to_mb(text, "{}\nAntigens: {}   Sera: {}\n", info()->make_info(), number_of_antigens(), number_of_sera());
    if (const auto layers = titers()->number_of_layers(); layers)
        fmt::format_to_mb(text, "Number of layers: {}\n", layers);
    if (const auto having_too_few_numeric_titers = titers()->having_too_few_numeric_titers(); !having_too_few_numeric_titers->empty())
        fmt::format_to_mb(text, "Points having too few numeric titers:{} {}\n", having_too_few_numeric_titers->size(), having_too_few_numeric_titers);

    if (inf & info_data::column_bases) {
        auto cb = computed_column_bases(MinimumColumnBasis{});
        fmt::format_to_mb(text, fmt::runtime("computed column bases:                 {:5.2f}\n"), *cb);
        for (auto projection_no : range_from_0_to(std::min(number_of_projections(), max_number_of_projections_to_show))) {
            if (auto fcb = projection(projection_no)->forced_column_bases(); fcb) {
                fmt::format_to_mb(text, fmt::runtime("forced column bases for projection {:2d}: {:5.2f}\n"), projection_no, *fcb);
                fmt::format_to_mb(text, "                                 diff: [");
                for (const auto sr_no : range_from_0_to(cb->size())) {
                    if (float_equal(cb->column_basis(sr_no), fcb->column_basis(sr_no)))
                        fmt::format_to_mb(text, "  .   ");
                    else
                        fmt::format_to_mb(text, "{:5.2f} ", cb->column_basis(sr_no) - fcb->column_basis(sr_no));
                }
                fmt::format_to_mb(text, "]\n");
            }
        }
    }

    fmt::format_to_mb(text, "{}\n", projections()->make_info(max_number_of_projections_to_show));

    if (inf & info_data::tables && info()->number_of_sources() > 0) {
        fmt::format_to_mb(text, "\nTables:\n");
        for (const auto src_no : range_from_0_to(info()->number_of_sources()))
            fmt::format_to_mb(text, "{:3d} {}\n", src_no, info()->source(src_no)->make_name());
    }

    if (inf & info_data::tables_for_sera && info()->number_of_sources() > 0) {
        auto titers = this->titers();
        auto sera = this->sera();
        for (auto [sr_no, serum] : acmacs::enumerate(*sera)) {
            fmt::format_to_mb(text, "SR {:3d} {}\n", sr_no, serum->format("{name_full_passage}"));
            for (const auto layer_no : titers->layers_with_serum(sr_no))
                fmt::format_to_mb(text, "    {:3d} {}\n", layer_no, info()->source(layer_no)->make_name());
        }
    }

    if (inf & info_data::dates) {
        acmacs::Counter<std::string> dates;
        auto antigens = this->antigens();
        for (const auto antigen : *antigens) {
            if (const auto date = antigen->date(); !date.empty())
                dates.count(date->substr(0, 7));
            else
                dates.count("*empty*");
        }
        fmt::format_to_mb(text, "Antigen dates ({})\n", dates.size());
        for (const auto& [date, count] : dates.counter())
            fmt::format_to_mb(text, "    {:7s} {:4d}\n", date, count);
    }

    return fmt::to_string(text);

} // ae::chart::v2::Chart::make_info

// ----------------------------------------------------------------------

std::string ae::chart::v2::Chart::make_name(std::optional<size_t> aProjectionNo) const
{
    fmt::memory_buffer name;
    fmt::format_to_mb(name, "{}", info()->make_name());
    if (auto prjs = projections(); !prjs->empty() && (!aProjectionNo || *aProjectionNo < prjs->size())) {
        auto prj = (*prjs)[aProjectionNo ? *aProjectionNo : 0];
        fmt::format_to_mb(name, " {}", prj->minimum_column_basis().format(">={}", MinimumColumnBasis::use_none::no));
        if (const auto stress = prj->stress(); !std::isnan(stress))
            fmt::format_to_mb(name, " {:.4f}", stress);
    }
    return fmt::to_string(name);

} // ae::chart::v2::Chart::make_name

// ----------------------------------------------------------------------

std::string ae::chart::v2::Chart::description() const
{
    fmt::memory_buffer desc;
    fmt::format_to_mb(desc, "{}", info()->make_name());
    if (auto prjs = projections(); !prjs->empty()) {
        auto prj = (*prjs)[0];
        fmt::format_to_mb(desc, "{}", prj->minimum_column_basis().format(">={}", MinimumColumnBasis::use_none::yes));
        if (const auto stress = prj->stress(); !std::isnan(stress))
            fmt::format_to_mb(desc, " {:.4f}", stress);
    }
    if (info()->virus_type() == acmacs::virus::type_subtype_t{"B"})
        fmt::format_to_mb(desc, " {}", lineage());
    fmt::format_to_mb(desc, " AG:{} Sr:{}", number_of_antigens(), number_of_sera());
    if (const auto layers = titers()->number_of_layers(); layers > 1)
        fmt::format_to_mb(desc, " ({} source tables)", layers);
    return fmt::to_string(desc);

} // ae::chart::v2::Chart::description

// ----------------------------------------------------------------------

std::shared_ptr<ae::chart::v2::ColumnBases> ae::chart::v2::Chart::computed_column_bases(MinimumColumnBasis aMinimumColumnBasis, use_cache a_use_cache) const
{
    if (a_use_cache == use_cache::yes) {
        if (auto found = computed_column_bases_.find(aMinimumColumnBasis); found != computed_column_bases_.end())
            return found->second;
    }
    return computed_column_bases_[aMinimumColumnBasis] = titers()->computed_column_bases(aMinimumColumnBasis);

} // ae::chart::v2::Chart::computed_column_bases

// ----------------------------------------------------------------------

std::shared_ptr<ae::chart::v2::ColumnBases> ae::chart::v2::Chart::column_bases(MinimumColumnBasis aMinimumColumnBasis) const
{
    if (auto cb = forced_column_bases(aMinimumColumnBasis); cb)
        return cb;
    return computed_column_bases(aMinimumColumnBasis);

} // ae::chart::v2::Chart::column_bases

// ----------------------------------------------------------------------

double ae::chart::v2::Chart::column_basis(size_t serum_no, size_t projection_no) const
{
    if (number_of_projections()) {
        auto prj = projection(projection_no);
        if (auto forced = prj->forced_column_bases(); forced)
            return forced->column_basis(serum_no);
        else
            return computed_column_bases(prj->minimum_column_basis(), use_cache::yes)->column_basis(serum_no);
    }
    else {
        if (auto forced = forced_column_bases({}); forced)
            return forced->column_basis(serum_no);
        else
            return computed_column_bases({}, use_cache::yes)->column_basis(serum_no);
    }

} // ae::chart::v2::Chart::column_basis

// ----------------------------------------------------------------------

acmacs::virus::lineage_t ae::chart::v2::Chart::lineage() const
{
    std::map<BLineage, size_t> lineages;
    auto ags = antigens();
    for (auto antigen: *ags) {
        if (const auto lineage = antigen->lineage(); lineage != BLineage::Unknown)
            ++lineages[lineage];
    }
    switch (lineages.size()) {
      case 0:
          return {};
      case 1:
          return lineages.begin()->first;
      default:
          return std::max_element(lineages.begin(), lineages.end(), [](const auto& a, const auto& b) -> bool { return a.second < b.second; })->first;
    }
    // return {};

} // ae::chart::v2::Chart::lineage

// ----------------------------------------------------------------------

void ae::chart::v2::Chart::set_homologous(find_homologous options, SeraP aSera, acmacs::debug dbg) const
{
    if (!aSera)
        aSera = sera();
    aSera->set_homologous(options, *antigens(), dbg);

} // ae::chart::v2::Chart::set_homologous

// ----------------------------------------------------------------------

acmacs::PointStyle ae::chart::v2::Chart::default_style(PointType aPointType) const
{
    acmacs::PointStyle style;
    style.outline(BLACK);
    switch (aPointType) {
      case PointType::TestAntigen:
          style.shape(acmacs::PointShape::Circle);
          style.size(Pixels{5.0});
          style.fill(GREEN);
          break;
      case PointType::ReferenceAntigen:
          style.shape(acmacs::PointShape::Circle);
          style.size(Pixels{8.0});
          style.fill(TRANSPARENT);
          break;
      case PointType::Serum:
          style.shape(acmacs::PointShape::Box);
          style.size(Pixels{6.5});
          style.fill(TRANSPARENT);
          break;
    }
    return style;

} // ae::chart::v2::Chart::default_style

// ----------------------------------------------------------------------

std::vector<acmacs::PointStyle> ae::chart::v2::Chart::default_all_styles() const
{
    auto ags = antigens();
    auto srs = sera();
    std::vector<acmacs::PointStyle> result(ags->size() + srs->size());
    for (size_t ag_no = 0; ag_no < ags->size(); ++ag_no)
        result[ag_no] = default_style((*ags)[ag_no]->reference() ? PointType::ReferenceAntigen : PointType::TestAntigen);
    for (auto ps = result.begin() + static_cast<typename decltype(result.begin())::difference_type>(ags->size()); ps != result.end(); ++ps)
        *ps = default_style(PointType::Serum);
    return result;

} // ae::chart::v2::Chart::default_all_styles

// ----------------------------------------------------------------------

std::string ae::chart::v2::Chart::show_table(std::optional<size_t> layer_no) const
{
    fmt::memory_buffer output;

    // auto sr_label = [](size_t sr_no) -> char { return static_cast<char>('A' + sr_no); };
    auto sr_label = [](size_t sr_no) -> size_t { return sr_no + 1; };

    auto ags = antigens();
    auto srs = sera();
    auto tt = titers();
    PointIndexList antigen_indexes, serum_indexes;
    if (layer_no) {
        std::tie(antigen_indexes, serum_indexes) = tt->antigens_sera_of_layer(*layer_no);
    }
    else {
        antigen_indexes = PointIndexList{filled_with_indexes(ags->size())};
        serum_indexes = PointIndexList{filled_with_indexes(srs->size())};
    }

    const auto max_ag_name = static_cast<int>(max_full_name(*ags));

    fmt::format_to_mb(output, "{:>{}s}Serum full names are under the table\n{:>{}s}", "", max_ag_name + 6, "", max_ag_name);
    for (auto sr_ind : range_from_0_to(serum_indexes->size()))
        fmt::format_to_mb(output, "{:>7d}", sr_label(sr_ind));
    fmt::format_to_mb(output, "\n");

    fmt::format_to_mb(output, "{:{}s}", "", max_ag_name + 2);
    for (auto sr_no : serum_indexes)
        fmt::format_to_mb(output, "{:>7s}", srs->at(sr_no)->format("{location_abbreviated}/{year2}"));
    fmt::format_to_mb(output, "\n");

    for (auto ag_no : antigen_indexes) {
        fmt::format_to_mb(output, "{:<{}s}", ags->at(ag_no)->name_full(), max_ag_name + 2);
        for (auto sr_no : serum_indexes)
            fmt::format_to_mb(output, "{:>7s}", *tt->titer(ag_no, sr_no));
        fmt::format_to_mb(output, "\n");
    }
    fmt::format_to_mb(output, "\n");

    for (auto [sr_ind, sr_no] : acmacs::enumerate(serum_indexes))
        fmt::format_to_mb(output, "{:3d} {} {}\n", sr_label(sr_ind), srs->at(sr_no)->format("{location_abbreviated}/{year2}"), srs->at(sr_no)->name_full());

    return fmt::to_string(output);

} // ae::chart::v2::Chart::show_table

// ----------------------------------------------------------------------

bool ae::chart::v2::same_tables(const Chart& c1, const Chart& c2, bool verbose)
{
    if (!equal(*c1.antigens(), *c2.antigens(), verbose)) {
        if (verbose)
            fmt::print(stderr, "WARNING: antigen sets are different\n");
        return false;
    }

    if (!equal(*c1.sera(), *c2.sera(), verbose)) {
        if (verbose)
            fmt::print(stderr, "WARNING: serum sets are different\n");
        return false;
    }

    if (!equal(*c1.titers(), *c2.titers(), verbose)) {
        if (verbose)
            fmt::print(stderr, "WARNING: titers are different\n");
        return false;
    }

    return true;

} // ae::chart::v2::same_tables

// ----------------------------------------------------------------------

ae::chart::v2::BLineage::Lineage ae::chart::v2::BLineage::from(char aSource)
{
    switch (aSource) {
        case 'Y':
        case 'y':
            return Yamagata;
        case 'V':
        case 'v':
            return Victoria;
    }
    return Unknown;

} // ae::chart::v2::BLineage::from

// ----------------------------------------------------------------------

ae::chart::v2::BLineage::Lineage ae::chart::v2::BLineage::from(std::string_view aSource)
{
    return aSource.empty() ? Unknown : from(aSource[0]);

} // ae::chart::v2::BLineage::from

// ----------------------------------------------------------------------

std::string ae::chart::v2::Info::make_info() const
{
    const auto n_sources = number_of_sources();
    return acmacs::string::join(acmacs::string::join_space, name(), *virus(Compute::Yes), lab(Compute::Yes), virus_type(Compute::Yes), subset(Compute::Yes), assay(Compute::Yes), rbc_species(Compute::Yes),
                                date(Compute::Yes), n_sources ? ("(" + std::to_string(n_sources) + " tables)") : std::string{});

} // ae::chart::v2::Info::make_info

// ----------------------------------------------------------------------

std::string ae::chart::v2::Info::make_name() const
{
    std::string n = name(Compute::No);
    if (n.empty()) {
        const auto vt = virus_type(Compute::Yes);
        n = acmacs::string::join(acmacs::string::join_space, lab(Compute::Yes), *virus_not_influenza(Compute::Yes), vt, subset(Compute::Yes), assay(Compute::Yes).HI_or_Neut(Assay::no_hi::yes),
                                 rbc_species(Compute::Yes), date(Compute::Yes));
    }
    return n;

} // ae::chart::v2::Info::make_name

// ----------------------------------------------------------------------

size_t ae::chart::v2::Info::max_source_name() const
{
    if (number_of_sources() < 2)
        return 0;
    size_t msn = 0;
    for (auto s_no : range_from_0_to(number_of_sources()))
        msn = std::max(msn, source(s_no)->name().size());
    return msn;

} // ae::chart::v2::Info::max_source_name

// ----------------------------------------------------------------------

acmacs::Lab ae::chart::v2::Info::fix_lab_name(Lab source, FixLab fix) const
{
    switch (fix) {
        case FixLab::no:
            break;
        case FixLab::yes:
            source = acmacs::whocc::lab_name_normalize(source);
            break;
        case FixLab::reverse:
            source = acmacs::whocc::lab_name_old(source);
            break;
    }
    return source;

} // ae::chart::v2::Info::fix_lab_name

// ----------------------------------------------------------------------

std::string ae::chart::v2::Projection::make_info() const
{
    fmt::memory_buffer result;
    auto lt = layout();
    fmt::format_to_mb(result, "{:.14f} {}d", stress(), lt->number_of_dimensions());
    if (auto cmt = comment(); !cmt.empty())
        fmt::format_to_mb(result, " <{}>", cmt);
    if (auto fcb = forced_column_bases(); fcb)
        fmt::format_to_mb(result, " forced-column-bases"); // fcb
    else
        fmt::format_to_mb(result, " >={}", minimum_column_basis());
    return fmt::to_string(result);

} // ae::chart::v2::Projection::make_info

// ----------------------------------------------------------------------

double ae::chart::v2::Projection::stress(RecalculateStress recalculate) const
{
    switch (recalculate) {
      case RecalculateStress::yes:
          return recalculate_stress();
      case RecalculateStress::if_necessary:
          if (const auto s = stored_stress(); s)
              return *s;
          else
              return recalculate_stress();
      case RecalculateStress::no:
          if (const auto s = stored_stress(); s)
              return *s;
          else
              return InvalidStress;
    }
    throw invalid_data("Projection::stress: internal");

} // ae::chart::v2::Projection::stress

// ----------------------------------------------------------------------

double ae::chart::v2::Projection::stress_with_moved_point(size_t point_no, const PointCoordinates& move_to) const
{
    acmacs::Layout new_layout(*layout());
    new_layout.update(point_no, move_to);
    return stress_factory(*this, multiply_antigen_titer_until_column_adjust::yes).value(new_layout);

} // ae::chart::v2::Projection::stress_with_moved_point

// ----------------------------------------------------------------------

ae::chart::v2::Blobs ae::chart::v2::Projection::blobs(double stress_diff, size_t number_of_drections, double stress_diff_precision) const
{
    Blobs blobs(stress_diff, number_of_drections, stress_diff_precision);
    blobs.calculate(*layout(), stress_factory(*this, multiply_antigen_titer_until_column_adjust::yes));
    return blobs;

} // ae::chart::v2::Projection::blobs

// ----------------------------------------------------------------------

ae::chart::v2::Blobs ae::chart::v2::Projection::blobs(double stress_diff, const PointIndexList& points, size_t number_of_drections, double stress_diff_precision) const
{
    Blobs blobs(stress_diff, number_of_drections, stress_diff_precision);
    blobs.calculate(*layout(), points, stress_factory(*this, multiply_antigen_titer_until_column_adjust::yes));
    return blobs;

} // ae::chart::v2::Projection::blobs

// ----------------------------------------------------------------------

std::string ae::chart::v2::Projections::make_info(size_t max_number_of_projections_to_show) const
{
    fmt::memory_buffer text;
    fmt::format_to_mb(text, "Projections: {}", size());
    for (auto projection_no: range_from_0_to(std::min(max_number_of_projections_to_show, size())))
        fmt::format_to_mb(text, "\n{:3d} {}", projection_no, operator[](projection_no)->make_info());
    return fmt::to_string(text);

} // ae::chart::v2::Projections::make_info

// ----------------------------------------------------------------------

// size_t ae::chart::v2::Projections::projection_no(const Projection* projection) const
// {
//     std::cerr << "projection_no " << projection << '\n';
//     for (size_t index = 0; index < size(); ++index) {
//         std::cerr << "p " << index << ' ' << operator[](index).get() << '\n';
//         if (operator[](index).get() == projection)
//             return index;
//     }
//     throw invalid_data{AD_FORMAT("cannot find projection_no, total projections: {}", size())};

// } // ae::chart::v2::Projections::projection_no

// ----------------------------------------------------------------------

std::vector<ae::chart::v2::Date> ae::chart::v2::Antigens::all_dates(include_reference inc_ref) const
{
    std::vector<Date> dates;
    for (auto antigen : *this) {
        if (inc_ref == include_reference::yes || !antigen->reference()) {
            if (auto date{antigen->date()}; !date.empty())
                dates.push_back(std::move(date));
        }
    }
    std::sort(std::begin(dates), std::end(dates));
    dates.erase(std::unique(std::begin(dates), std::end(dates)), std::end(dates));
    return dates;

} // ae::chart::v2::Antigens::all_dates

// ----------------------------------------------------------------------

#include "acmacs-base/global-constructors-push.hh"
static const std::regex sAnntotationToIgnore{"(CONC|RDE@|BOOST|BLEED|LAIV|^CDC$)"};
#include "acmacs-base/diagnostics-pop.hh"

bool ae::chart::v2::Annotations::match_antigen_serum(const Annotations& antigen, const Annotations& serum)
{
    std::vector<std::string_view> antigen_fixed(antigen->size());
    auto antigen_fixed_end = antigen_fixed.begin();
    for (const auto& anno : antigen) {
        *antigen_fixed_end++ = anno;
    }
    antigen_fixed.erase(antigen_fixed_end, antigen_fixed.end());
    std::sort(antigen_fixed.begin(), antigen_fixed.end());

    std::vector<std::string_view> serum_fixed(serum->size());
    auto serum_fixed_end = serum_fixed.begin();
    for (const auto& anno : serum) {
        const std::string_view annos = static_cast<std::string_view>(anno);
        if (!std::regex_search(std::begin(annos), std::end(annos), sAnntotationToIgnore))
            *serum_fixed_end++ = anno;
    }
    serum_fixed.erase(serum_fixed_end, serum_fixed.end());
    std::sort(serum_fixed.begin(), serum_fixed.end());

    return antigen_fixed == serum_fixed;

} // ae::chart::v2::Annotations::match_antigen_serum

// ----------------------------------------------------------------------

ae::chart::v2::Sera::homologous_canditates_t ae::chart::v2::Sera::find_homologous_canditates(const Antigens& aAntigens, acmacs::debug dbg) const
{
    const auto match_passage = [](acmacs::virus::Passage antigen_passage, acmacs::virus::Passage serum_passage, const Serum& serum) -> bool {
        if (serum_passage.empty()) // NIID has passage type data in serum_id
            return antigen_passage.is_egg() == (serum.serum_id().find("EGG") != std::string::npos);
        else
            return antigen_passage.is_egg() == serum_passage.is_egg();
    };

    std::map<std::string, std::vector<size_t>, std::less<>> antigen_name_index;
    for (auto [ag_no, antigen] : acmacs::enumerate(aAntigens))
        antigen_name_index.emplace(antigen->name(), std::vector<size_t>{}).first->second.push_back(ag_no);

    Sera::homologous_canditates_t result(size());
    for (auto [sr_no, serum] : acmacs::enumerate(*this)) {
        if (auto ags = antigen_name_index.find(*serum->name()); ags != antigen_name_index.end()) {
            for (auto ag_no : ags->second) {
                auto antigen = aAntigens[ag_no];
                if (dbg == debug::yes)
                    fmt::print(stderr, "DEBUG: SR {} {} R:{} A:{} P:{} -- AG {} {} R:{} A:{} P:{} -- A_match:{} R_match: {} P_match:{}\n",
                               sr_no, *serum->name(), serum->annotations(), *serum->reassortant(), *serum->passage(),
                               ag_no, *antigen->name(), antigen->annotations(), *antigen->reassortant(), *antigen->passage(),
                               Annotations::match_antigen_serum(antigen->annotations(), serum->annotations()), antigen->reassortant() == serum->reassortant(),
                               match_passage(antigen->passage(), serum->passage(), *serum));
                if (Annotations::match_antigen_serum(antigen->annotations(), serum->annotations()) && antigen->reassortant() == serum->reassortant() &&
                    match_passage(antigen->passage(), serum->passage(), *serum)) {
                    result[sr_no].insert(ag_no);
                }
            }
        }
    }

    return result;

} // ae::chart::v2::Sera::find_homologous_canditates

// ----------------------------------------------------------------------

void ae::chart::v2::Sera::set_homologous(find_homologous options, const Antigens& aAntigens, acmacs::debug dbg)
{
    const auto match_passage_strict = [](acmacs::virus::Passage antigen_passage, acmacs::virus::Passage serum_passage, const Serum& serum) -> bool {
        if (serum_passage.empty()) // NIID has passage type data in serum_id
            return antigen_passage.is_egg() == (serum.serum_id().find("EGG") != std::string::npos);
        else
            return antigen_passage == serum_passage;
    };

    const auto match_passage_relaxed = [](acmacs::virus::Passage antigen_passage, acmacs::virus::Passage serum_passage, const Serum& serum) -> bool {
        if (serum_passage.empty()) // NIID has passage type data in serum_id
            return antigen_passage.is_egg() == (serum.serum_id().find("EGG") != std::string::npos);
        else
            return antigen_passage.is_egg() == serum_passage.is_egg();
    };

    const auto homologous_canditates = find_homologous_canditates(aAntigens, dbg);

    if (options == find_homologous::all) {
        for (auto [sr_no, serum] : acmacs::enumerate(*this))
            serum->set_homologous(*homologous_canditates[sr_no], dbg);
    }
    else {
        std::vector<std::optional<size_t>> homologous(size()); // for each serum
        for (auto [sr_no, serum] : acmacs::enumerate(*this)) {
            const auto& canditates = homologous_canditates[sr_no];
            for (auto canditate : canditates) {
                if (match_passage_strict(aAntigens[canditate]->passage(), serum->passage(), *serum)) {
                    homologous[sr_no] = canditate;
                    break;
                }
            }
        }

        if (options != find_homologous::strict) {
            for (auto [sr_no, serum] : acmacs::enumerate(*this)) {
                if (!homologous[sr_no]) {
                    const auto& canditates = homologous_canditates[sr_no];
                    for (auto canditate : canditates) {
                        const auto occupied = std::any_of(homologous.begin(), homologous.end(), [canditate](std::optional<size_t> ag_no) -> bool { return ag_no && *ag_no == canditate; });
                        if (!occupied && match_passage_relaxed(aAntigens[canditate]->passage(), serum->passage(), *serum)) {
                            homologous[sr_no] = canditate;
                            break;
                        }
                    }
                }
            }

            if (options != find_homologous::relaxed_strict) {
                for (auto [sr_no, serum] : acmacs::enumerate(*this)) {
                    if (!homologous[sr_no]) {
                        const auto& canditates = homologous_canditates[sr_no];
                        for (auto canditate : canditates) {
                            if (match_passage_relaxed(aAntigens[canditate]->passage(), serum->passage(), *serum)) {
                                homologous[sr_no] = canditate;
                                break;
                            }
                        }
                    }
                }
            }
        }

        for (auto [sr_no, serum] : acmacs::enumerate(*this))
            if (const auto homol = homologous[sr_no]; homol)
                serum->set_homologous({*homol}, dbg);
    }

} // ae::chart::v2::Sera::set_homologous

// ----------------------------------------------------------------------

acmacs::PointStylesCompacted ae::chart::v2::PlotSpec::compacted() const
{
    acmacs::PointStylesCompacted result;
    for (const auto& style: all_styles()) {
        if (auto found = std::find(result.styles.begin(), result.styles.end(), style); found == result.styles.end()) {
            result.styles.push_back(style);
            result.index.push_back(result.styles.size() - 1);
        }
        else {
            result.index.push_back(static_cast<size_t>(found - result.styles.begin()));
        }
    }
    return result;

} // ae::chart::v2::PlotSpec::compacted

// ======================================================================

ae::chart::v2::TableDate ae::chart::v2::table_date_from_sources(std::vector<std::string>&& sources)
{
    std::sort(std::begin(sources), std::end(sources));
    std::string front{sources.front()}, back{sources.back()};
    const std::string_view tables_separator{"+"};
    if (const auto fparts = acmacs::string::split(front, tables_separator, acmacs::string::Split::RemoveEmpty); fparts.size() > 1)
        front = fparts.front();
    if (const auto bparts = acmacs::string::split(back, tables_separator, acmacs::string::Split::RemoveEmpty); bparts.size() > 1)
        back = bparts.back();
    return TableDate{fmt::format("{}-{}", front, back)};

} // ae::chart::v2::table_date_from_sources

// ----------------------------------------------------------------------
