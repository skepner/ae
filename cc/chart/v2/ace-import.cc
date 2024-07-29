#include <set>
#include <vector>
#include <limits>

#include "utils/log.hh"
#include "virus/name.hh"
#include "utils/string.hh"
#include "chart/v2/ace-import.hh"
#include "chart/v2/ace.hh"

using namespace std::string_literals;
using namespace ae::chart::v2;

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif

const ae::chart::v2::RjsonTiters::Keys ae::chart::v2::AceTiters::s_keys_{"l", "d", "L"};
const ae::chart::v2::RjsonProjection::Keys ae::chart::v2::AceProjection::s_keys_{"s", "l", "c"};

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

bool ae::chart::v2::is_ace(std::string_view aData)
{
    return aData.size() > 35 && aData.front() == '{' && aData.find("\"acmacs-ace-v1\"") != std::string_view::npos;

} // ae::chart::v2::is_ace

// ----------------------------------------------------------------------

ChartP ae::chart::v2::ace_import(std::string_view aData, Verify aVerify)
{
    auto chart = std::make_shared<AceChart>(rjson::parse_string(aData));
    chart->verify_data(aVerify);
    return chart;

} // ae::chart::v2::ace_import

// ----------------------------------------------------------------------

void AceChart::verify_data(Verify aVerify) const
{
    try {
        const auto& antigens = data_.get("c", "a");
        if (antigens.empty())
            throw import_error("no antigens");
        const auto& sera = data_.get("c", "s");
        if (sera.empty())
            throw import_error("no sera");
        const auto& titers = data_.get("c", "t");
        if (titers.empty())
            throw import_error("no titers");
        if (const auto& ll = titers["l"]; !ll.is_null()) {
            if (ll.size() != antigens.size())
                throw import_error(fmt::format("number of the titer rows ({}) does not correspond to the number of antigens ({})", ll.size(), antigens.size()));
        }
        else if (const auto& dd = titers["d"]; !dd.is_null()) {
            if (dd.size() != antigens.size())
                throw import_error(fmt::format("number of the titer rows ({}) does not correspond to the number of antigens ({})", dd.size(), antigens.size()));
        }
        else
            throw import_error("no titers (neither \"l\" nor \"d\" present)");
        if (aVerify != Verify::None) {
            std::cerr << "WARNING: AceChart::verify_data not implemented\n";
        }
    }
    catch (std::exception& err) {
        throw import_error("[ace]: structure verification failed: "s + err.what());
    }

} // AceChart::verify_data

// ----------------------------------------------------------------------

InfoP AceChart::info() const
{
    return std::make_shared<AceInfo>(data_.get("c", "i"));

} // AceChart::info

// ----------------------------------------------------------------------

AntigensP AceChart::antigens() const
{
    return std::make_shared<AceAntigens>(data_.get("c", "a"), mAntigenNameIndex);

} // AceChart::antigens

// ----------------------------------------------------------------------

SeraP AceChart::sera() const
{
    return std::make_shared<AceSera>(data_.get("c", "s"));

} // AceChart::sera

// ----------------------------------------------------------------------

const rjson::value& AceChart::extension_field(std::string_view field_name) const
{
    return data_.get("c", "x", field_name);

} // AceChart::extension_field

// ----------------------------------------------------------------------

const rjson::value& AceChart::extension_fields() const
{
    return data_.get("c", "x");

} // AceChart::extension_fields

// ----------------------------------------------------------------------

TitersP AceChart::titers() const
{
    return std::make_shared<AceTiters>(data_.get("c", "t"), data_.get("c", "a").size(), data_.get("c", "s").size());

} // AceChart::titers

// ----------------------------------------------------------------------

ColumnBasesP AceChart::forced_column_bases(MinimumColumnBasis aMinimumColumnBasis) const
{
    // Racmacs may store "C": [null, null, ...], !cb[0].is_null() below handles it
    if (const auto& cb = data_.get("c", "C"); !cb.empty() && !cb[0].is_null())
        return std::make_shared<AceColumnBases>(cb, aMinimumColumnBasis);
    return nullptr;

} // AceChart::forced_column_bases

// ----------------------------------------------------------------------

ProjectionsP AceChart::projections() const
{
    if (!projections_)
        projections_ = std::make_shared<AceProjections>(*this, data_.get("c", "P"));
    return projections_;

} // AceChart::projections

// ----------------------------------------------------------------------

PlotSpecP AceChart::plot_spec() const
{
    return std::make_shared<AcePlotSpec>(data_.get("c", "p"), *this);

} // AceChart::plot_spec

// ----------------------------------------------------------------------

bool AceChart::is_merge() const
{
    return !data_.get("c", "t", "L").empty();

} // AceChart::is_merge

// ----------------------------------------------------------------------

bool AceChart::has_sequences() const
{
    return !rjson::find_if(data_.get("c", "a"), [](const auto& antigen) { return !antigen["A"].empty() || !antigen["B"].empty(); }).is_null();

} // AceChart::has_sequences

// ----------------------------------------------------------------------

std::string AceInfo::name(Compute aCompute) const
{
    std::string result{data_["N"].get_or_default("")};
    if (result.empty()) {
        if (const auto& sources = data_["S"]; !sources.empty()) {
            std::vector<std::string> composition(sources.size());
            rjson::transform(sources, composition.begin(), [](const rjson::value& sinfo) { return sinfo["N"].get_or_default(""); });
            composition.erase(std::remove_if(composition.begin(), composition.end(), [](const auto& s) { return s.empty(); }), composition.end());
            if (composition.size() > (sources.size() / 2))
                result = ae::string::join(" + ", composition);
        }
    }
    if (result.empty() && aCompute == Compute::Yes) {
        result = ae::string::join(" ", *virus_not_influenza(aCompute), virus_type(aCompute), subset(aCompute), *assay(aCompute), *lab(aCompute), *rbc_species(aCompute), *date(aCompute));
    }
    return result;

} // AceInfo::name

// ----------------------------------------------------------------------

std::string AceInfo::make_field(const char* aField, std::string_view aSeparator, Compute aCompute) const
{
    std::string result{data_[aField].get_or_default("")};
    if (result.empty() && aCompute == Compute::Yes) {
        if (const auto& sources = data_["S"]; !sources.empty()) {
            std::set<std::string> composition;
            rjson::transform(sources, std::inserter(composition, composition.begin()), [aField](const rjson::value& sinfo) { return sinfo[aField].get_or_default(""); });
            result = string::join(aSeparator, composition);
        }
    }
    return result;

} // AceInfo::make_field

// ----------------------------------------------------------------------

TableDate AceInfo::date(Compute aCompute) const
{
    std::string result{data_["D"].get_or_default("")};
    if (result.empty() && aCompute == Compute::Yes) {
        if (const auto& sources = data_["S"]; !sources.empty()) {
            std::vector<std::string> composition{sources.size()};
            rjson::transform(sources, composition.begin(), [](const rjson::value& sinfo) { return sinfo["D"].get_or_default(""); });
            return table_date_from_sources(std::move(composition));
        }
    }
    return TableDate{result};

} // AceInfo::date

// ----------------------------------------------------------------------

BLineage AceAntigen::lineage() const
{
    return BLineage{data_["L"].get_or_default("")};

} // AceAntigen::lineage

BLineage AceSerum::lineage() const
{
    return BLineage{data_["L"].get_or_default("")};

} // AceSerum::lineage

// ----------------------------------------------------------------------

std::optional<size_t> AceAntigens::find_by_full_name(std::string_view aFullName) const
{
    if (mAntigenNameIndex.empty())
        make_name_index();
    const auto name = aFullName; // ::virus_name::name(aFullName);
    if (const auto found = mAntigenNameIndex.find(name); found != mAntigenNameIndex.end()) {
        for (auto index: found->second) {
            if (AceAntigen(data_[index]).format("{name_full}") == aFullName)
                return index;
        }
    }
    return {};

} // AceAntigens::find_by_full_name

// std::optional<size_t> AceAntigens::find_by_full_name(std::string_view aFullName) const
// {
//     const std::string_view name{virus_name::name(aFullName)};
//     for (auto iter = data_.begin(); iter != data_.end(); ++iter) {
//         if ((*iter)["N"] == name && AceAntigen(*iter).format("{name_full}") == aFullName) {
//             return static_cast<size_t>(iter - data_.begin());
//         }
//     }
//     return {};

// } // AceAntigens::find_by_full_name

// ----------------------------------------------------------------------

void AceAntigens::make_name_index() const
{
    rjson::for_each(data_, [this](const rjson::value& val, size_t index) { mAntigenNameIndex[val["N"].to<std::string_view>()].push_back(index); });

} // AceAntigens::make_name_index

// ----------------------------------------------------------------------

// std::shared_ptr<Layout> AceProjection::layout() const
// {
//     if (!layout_)
//         layout_ = std::make_shared<rjson_import::Layout>(data_.get_or_empty_array("l"));
//     return layout_;

// } // AceProjection::layout

// ----------------------------------------------------------------------

// size_t AceProjection::number_of_dimensions() const
// {
//     return rjson_import::number_of_dimensions(data_["l"]);

// } // AceProjection::number_of_dimensions

// ----------------------------------------------------------------------

ColumnBasesP AceProjection::forced_column_bases() const
{
    // Racmacs stores "C": [null, null, ...], !cb[0].is_null() below handles it
    if (const auto& cb = data()["C"]; !cb.empty() && !cb[0].is_null()) {
        try {
            return std::make_shared<AceColumnBases>(cb);
        }
        catch (std::exception& err) {
            AD_ERROR("cannot read column bases: {}\ndata: {}", err.what(), cb);
            throw;
        }
    }
    return nullptr;

} // AceProjection::forced_column_bases

// ----------------------------------------------------------------------

ae::draw::v1::Transformation AceProjection::transformation() const
{
    ae::draw::v1::Transformation result(number_of_dimensions());
    if (const auto& array = data()["t"]; !array.empty()) {
        if (array.size() != (number_of_dimensions().get() * number_of_dimensions().get()))
            fmt::print(stderr, "WARNING: transformation stored in ace file ({}) does not correspond to the number of dimensions in the projection ({})\n", array, number_of_dimensions());
        std::vector<double> tr;
        rjson::copy(array, tr);
        result.set(tr.begin(), tr.size());
    }
    return result;

} // AceProjection::transformation

// ----------------------------------------------------------------------

Color AcePlotSpec::error_line_positive_color() const
{
    if (const auto& color = data_.get("E", "c"); !color.is_null())
        return Color(color.to<std::string_view>());
    else
        return RED;

} // AcePlotSpec::error_line_positive_color

// ----------------------------------------------------------------------

Color AcePlotSpec::error_line_negative_color() const
{
    if (const auto& color = data_.get("e", "c"); !color.is_null())
        return Color(color.to<std::string_view>());
    else
        return BLUE;

} // AcePlotSpec::error_line_negative_color

// ----------------------------------------------------------------------

acmacs::PointStyle AcePlotSpec::style(size_t aPointNo) const
{
    const auto& indices = data_["p"];
    try {
        const size_t style_no{indices[aPointNo].to<size_t>()};
        // std::cerr << "style " << aPointNo << ' ' << style_no << ' ' << data_["P"][style_no].to_json() << '\n';
        return extract(data_["P"][style_no], aPointNo, style_no);
    }
    catch (std::exception& /*err*/) {
          // std::cerr << "WARNING: [ace]: cannot get style for point " << aPointNo << ": " << err.what() << '\n';
    }
    return mChart.default_style(aPointNo);

} // AcePlotSpec::style

// ----------------------------------------------------------------------

std::vector<acmacs::PointStyle> AcePlotSpec::all_styles() const
{
    if (const auto& indices = data_["p"]; !indices.empty()) {
        std::vector<acmacs::PointStyle> result(indices.size());
        for (auto [point_no, target]: acmacs::enumerate(result)) {
            try {
                const size_t style_no{indices[point_no].to<size_t>()};
                target = extract(data_["P"][style_no], point_no, style_no);
            }
            catch (std::exception& err) {
                std::cerr << "WARNING: [ace]: cannot get point " << point_no << " style: " << err.what() << '\n';
                target = mChart.default_style(point_no);
            }
        }
        return result;
    }
    else {
          // std::cerr << "WARNING: [ace]: no point styles stored, default is used\n";
        return mChart.default_all_styles();
    }

} // AcePlotSpec::all_styles

// ----------------------------------------------------------------------

size_t AcePlotSpec::number_of_points() const
{
    if (const auto& indices = data_["p"]; !indices.empty())
        return indices.size();
    else
        return mChart.number_of_points();

} // AcePlotSpec::number_of_points

// ----------------------------------------------------------------------

acmacs::PointStyle AcePlotSpec::extract(const rjson::value& aSrc, size_t aPointNo, size_t aStyleNo) const
{
    acmacs::PointStyle result;
    rjson::for_each(aSrc, [&result,aPointNo,aStyleNo,this](std::string_view field_name, const rjson::value& field_value) {
        if (!field_name.empty()) {
            try {
                switch (field_name[0]) {
                  case '+':
                      result.shown(field_value.to<bool>());
                      break;
                  case 'F':
                      result.fill(Color(field_value.to<std::string_view>()));
                      break;
                  case 'O':
                      result.outline(Color(field_value.to<std::string_view>()));
                      break;
                  case 'o':
                      result.outline_width(ae::draw::v1::Pixels{field_value.to<double>()});
                      break;
                  case 's':
                      result.size(ae::draw::v1::Pixels{field_value.to<double>() * ae::chart::v2::ace::PointScale});
                      break;
                  case 'r':
                      result.rotation(ae::draw::v1::Rotation{field_value.to<double>()});
                      break;
                  case 'a':
                      result.aspect(ae::draw::v1::Aspect{field_value.to<double>()});
                      break;
                  case 'S':
                      result.shape(field_value.to<std::string_view>());
                      break;
                  case 'l':
                      this->label_style(result, field_value);
                      break;
                }
            }
            catch (std::exception& err) {
                AD_WARNING("[ace]: point {} style {} field \"{}\" value is wrong: {} value {}\n", aPointNo, aStyleNo, field_name, err.what(), field_value);
            }
        }
    });
    return result;

} // AcePlotSpec::extract

// ----------------------------------------------------------------------

void AcePlotSpec::label_style(acmacs::PointStyle& aStyle, const rjson::value& aData) const
{
    rjson::for_each(aData, [&aStyle](std::string_view field_name, const rjson::value& field_value) {
        if (!field_name.empty()) {
            try {
                auto& label_style = aStyle.label();
                switch (field_name[0]) {
                  case '+':
                      label_style.shown = field_value.to<bool>();
                      break;
                  case 'p':
                      label_style.offset = ae::draw::v1::Offset{field_value[0].to<double>(), field_value[1].to<double>()};
                      break;
                  case 's':
                      label_style.size = ae::draw::v1::Pixels{field_value.to<double>() * ae::chart::v2::ace::LabelScale};
                      break;
                  case 'c':
                      label_style.color.add(acmacs::color::Modifier{Color{field_value.to<std::string_view>()}});
                      break;
                  case 'r':
                      label_style.rotation = ae::draw::v1::Rotation{field_value.to<double>()};
                      break;
                  case 'i':
                      label_style.interline = field_value.to<double>();
                      break;
                  case 'f':
                      label_style.style.font_family = field_value.to<std::string_view>();
                      break;
                  case 'S':
                      label_style.style.slant = field_value.to<std::string_view>();
                      break;
                  case 'W':
                      label_style.style.weight = field_value.to<std::string_view>();
                      break;
                  case 't':
                      aStyle.label_text(field_value.to<std::string_view>());
                      break;
                }
            }
            catch (std::exception& err) {
                AD_WARNING("[ace]: label style field \"{}\" value is wrong: {} value {}\n", field_name, err.what(), field_value);
            }
        }
    });

} // AcePlotSpec::label_style

// ----------------------------------------------------------------------
