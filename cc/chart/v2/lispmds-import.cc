#include <set>
#include <vector>
#include <limits>
#include <regex>
#include <cmath>

#include "utils/string.hh"
#include "ad/enumerate.hh"
#include "ext/fmt.hh"
#include "chart/v2/lispmds-import.hh"
#include "chart/v2/lispmds-encode.hh"

using namespace ae::chart::v2;

// ----------------------------------------------------------------------

static std::vector<double> native_column_bases(const acmacs::lispmds::value& aData);
static std::vector<double> column_bases(const acmacs::lispmds::value& aData, size_t aProjectionNo);
static std::pair<std::shared_ptr<ae::chart::v2::ColumnBases>, ae::chart::v2::MinimumColumnBasis> forced_column_bases(const acmacs::lispmds::value& aData, size_t aProjectionNo);

// ----------------------------------------------------------------------

static inline size_t number_of_antigens(const acmacs::lispmds::value& aData)
{
    return acmacs::lispmds::size(aData, 0, 1);

} // number_of_antigens

static inline size_t number_of_sera(const acmacs::lispmds::value& aData)
{
    return acmacs::lispmds::size(aData, 0, 2);

} // number_of_sera

static inline const acmacs::lispmds::value& projection_data(const acmacs::lispmds::value& aData, size_t aProjectionNo)
{
    if (acmacs::lispmds::empty(aData, ":STARTING-COORDSS"))
        return acmacs::lispmds::get(aData, ":BATCH-RUNS", aProjectionNo);
    else if (aProjectionNo == 0)
        return acmacs::lispmds::get(aData, ":STARTING-COORDSS");
    else
        return acmacs::lispmds::get(aData, ":BATCH-RUNS", aProjectionNo - 1);

} // LispmdsProjection::data

static inline const acmacs::lispmds::value& projection_layout(const acmacs::lispmds::value& aData, size_t aProjectionNo)
{
    if (acmacs::lispmds::empty(aData, ":STARTING-COORDSS"))
        return acmacs::lispmds::get(aData, ":BATCH-RUNS", aProjectionNo, 0);
    else if (aProjectionNo == 0)
        return acmacs::lispmds::get(aData, ":STARTING-COORDSS");
    else
        return acmacs::lispmds::get(aData, ":BATCH-RUNS", aProjectionNo - 1, 0);

} // LispmdsProjection::data

// ----------------------------------------------------------------------

std::vector<double> native_column_bases(const acmacs::lispmds::value& aData)
{
    std::vector<double> cb(number_of_sera(aData), 0);
    for (const auto& row : std::get<acmacs::lispmds::list>(acmacs::lispmds::get(aData, 0, 3))) {
        for (auto [sr_no, titer_v] : acmacs::enumerate(std::get<acmacs::lispmds::list>(row))) {
            std::visit(
                [&cb, sr_no](auto&& titer) {
                    using T = std::decay_t<decltype(titer)>;
                    if constexpr (std::is_same_v<T, acmacs::lispmds::symbol>) {
                        double titer_d;
                        switch (titer[0]) {
                            case '<':
                                titer_d = std::stod(titer->substr(1));
                                if (titer_d > cb[sr_no])
                                    cb[sr_no] = titer_d;
                                break;
                            case '>':
                                titer_d = std::stod(titer->substr(1)) + 1;
                                if (titer_d > cb[sr_no])
                                    cb[sr_no] = titer_d;
                                break;
                            default:
                                break;
                        }
                    }
                    else if constexpr (std::is_same_v<T, acmacs::lispmds::number>) {
                        const double titer_d = titer;
                        if (titer_d > cb[sr_no])
                            cb[sr_no] = titer_d;
                    }
                    else
                        throw acmacs::lispmds::type_mismatch{fmt::format("Unexpected titer type: {}", typeid(T).name())};
                },
                titer_v);
        }
    }
    return cb;

} // native_column_bases

// ----------------------------------------------------------------------

std::vector<double> column_bases(const acmacs::lispmds::value& aData, size_t aProjectionNo)
{
    const auto num_antigens = number_of_antigens(aData);
    const auto num_sera = number_of_sera(aData);
    const auto number_of_points = num_antigens + num_sera;
    const auto& cb = std::get<acmacs::lispmds::list>(acmacs::lispmds::get(projection_layout(aData, aProjectionNo), number_of_points, 0, 1));
    std::vector<double> result(num_sera);
    using diff_t = decltype(cb.end() - cb.begin());
    std::transform(cb.begin() + static_cast<diff_t>(num_antigens), cb.begin() + static_cast<diff_t>(number_of_points), result.begin(), [](const auto& val) -> double { return std::get<acmacs::lispmds::number>(val); });
    return result;

} // column_bases

// ----------------------------------------------------------------------

std::pair<std::shared_ptr<ae::chart::v2::ColumnBases>, ae::chart::v2::MinimumColumnBasis> forced_column_bases(const acmacs::lispmds::value& aData, size_t aProjectionNo)
{
    try {
        const auto native_cb = native_column_bases(aData);
        const auto cb = column_bases(aData, aProjectionNo);
        if (native_cb == cb) {
            return {nullptr, ae::chart::v2::MinimumColumnBasis()};
        }
        else {
            const double min_forced = *std::min_element(cb.begin(), cb.end());
            std::decay_t<decltype(native_cb)> native_upgraded(native_cb.size());
            std::transform(native_cb.begin(), native_cb.end(), native_upgraded.begin(), [min_forced](double b) -> double { return std::max(b, min_forced); });
            // std::cerr << "INFO: native: " << native_cb << '\n';
            // std::cerr << "INFO: forced: " << cb << '\n';
            // std::cerr << "INFO: upgrad: " << native_upgraded << '\n';
            if (native_upgraded == cb)
                return {nullptr, ae::chart::v2::MinimumColumnBasis(min_forced)};
            else
                return {std::make_shared<LispmdsColumnBases>(cb), ae::chart::v2::MinimumColumnBasis()};
        }
    }
    catch (acmacs::lispmds::keyword_no_found&) {
        return {nullptr, ae::chart::v2::MinimumColumnBasis()};
    }
    catch (acmacs::lispmds::error& err) {
        fmt::print(stderr, "WARNING: broken save: {}\n", err.what());
        return {nullptr, ae::chart::v2::MinimumColumnBasis()};
    }

} // forced_column_bases

// ----------------------------------------------------------------------

ChartP ae::chart::v2::lispmds_import(std::string_view aData, Verify aVerify)
{
    try {
        auto chart = std::make_shared<LispmdsChart>(acmacs::lispmds::parse_string(aData));
        chart->verify_data(aVerify);
        return chart;
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR: lispmds_import: {}\n", err.what());
        throw;
    }

} // ae::chart::v2::lispmds_import

// ----------------------------------------------------------------------

void LispmdsChart::verify_data(Verify) const
{
    try {
        if (number_of_antigens() == 0)
            throw import_error("no antigens");
        if (number_of_sera() == 0)
            throw import_error("no sera (genetic tables are not supported)");
    }
    catch (std::exception& err) {
        throw import_error{fmt::format("[lispmds]: structure verification failed: {}", err.what())};
    }

} // LispmdsChart::verify_data

// ----------------------------------------------------------------------

InfoP LispmdsChart::info() const
{
    return std::make_shared<LispmdsInfo>(mData);

} // LispmdsChart::info

// ----------------------------------------------------------------------

AntigensP LispmdsChart::antigens() const
{
    return std::make_shared<LispmdsAntigens>(mData);

} // LispmdsChart::antigens

// ----------------------------------------------------------------------

SeraP LispmdsChart::sera() const
{
    return std::make_shared<LispmdsSera>(mData);

} // LispmdsChart::sera

// ----------------------------------------------------------------------

TitersP LispmdsChart::titers() const
{
    return std::make_shared<LispmdsTiters>(mData);

} // LispmdsChart::titers

// ----------------------------------------------------------------------

ColumnBasesP LispmdsChart::forced_column_bases(MinimumColumnBasis /*aMinimumColumnBasis*/) const
{
    return ::forced_column_bases(mData, 0).first;

} // LispmdsChart::forced_column_bases

// ----------------------------------------------------------------------

ProjectionsP LispmdsChart::projections() const
{
    if (!projections_)
        projections_ = std::make_shared<LispmdsProjections>(*this, mData);
    return projections_;

} // LispmdsChart::projections

// ----------------------------------------------------------------------

PlotSpecP LispmdsChart::plot_spec() const
{
    return std::make_shared<LispmdsPlotSpec>(mData);

} // LispmdsChart::plot_spec

// ----------------------------------------------------------------------

size_t LispmdsChart::number_of_antigens() const
{
    return ::number_of_antigens(mData);

} // LispmdsChart::number_of_antigens

// ----------------------------------------------------------------------

size_t LispmdsChart::number_of_sera() const
{
    return ::number_of_sera(mData);

} // LispmdsChart::number_of_sera

// ----------------------------------------------------------------------

std::string LispmdsInfo::name(Compute) const
{
    if (acmacs::lispmds::size(mData, 0) >= 5)
        return lispmds_decode(std::get<acmacs::lispmds::symbol>(acmacs::lispmds::get(mData, 0, 4)));
    else
        return {};

} // LispmdsInfo::name

// ----------------------------------------------------------------------

static inline std::string antigen_name(const acmacs::lispmds::value& aData, size_t aIndex)
{
    return *std::get<acmacs::lispmds::symbol>(acmacs::lispmds::get(aData, 0, 1, aIndex));
}

// ----------------------------------------------------------------------

ae::virus::Name LispmdsAntigen::name() const
{
    ae::virus::Name name;
    ae::virus::Reassortant reassortant;
    ae::virus::Passage passage;
    Annotations annotations;
    lispmds_antigen_name_decode(antigen_name(mData, mIndex), name, reassortant, passage, annotations);
    return name;

} // LispmdsAntigen::name

// ----------------------------------------------------------------------

ae::virus::Passage LispmdsAntigen::passage() const
{
    ae::virus::Name name;
    ae::virus::Reassortant reassortant;
    ae::virus::Passage passage;
    Annotations annotations;
    lispmds_antigen_name_decode(antigen_name(mData, mIndex), name, reassortant, passage, annotations);
    return passage;

} // LispmdsAntigen::passage

// ----------------------------------------------------------------------

ae::virus::Reassortant LispmdsAntigen::reassortant() const
{
    ae::virus::Name name;
    ae::virus::Reassortant reassortant;
    ae::virus::Passage passage;
    Annotations annotations;
    lispmds_antigen_name_decode(antigen_name(mData, mIndex), name, reassortant, passage, annotations);
    return reassortant;

} // LispmdsAntigen::reassortant

// ----------------------------------------------------------------------

Annotations LispmdsAntigen::annotations() const
{
    ae::virus::Name name;
    ae::virus::Reassortant reassortant;
    ae::virus::Passage passage;
    Annotations annotations;
    lispmds_antigen_name_decode(antigen_name(mData, mIndex), name, reassortant, passage, annotations);
    return annotations;

} // LispmdsAntigen::annotations

// ----------------------------------------------------------------------

bool LispmdsAntigen::reference() const
{
    try {
        const auto& val = acmacs::lispmds::get(mData, ":REFERENCE-ANTIGENS");
        if (acmacs::lispmds::empty(val))
            return false;
        const auto name = antigen_name(mData, mIndex);
        const auto& val_l = std::get<acmacs::lispmds::list>(val);
        return std::find_if(val_l.begin(), val_l.end(), [&name](const auto& ev) -> bool { return std::get<acmacs::lispmds::symbol>(ev) == acmacs::lispmds::symbol{name}; }) != val_l.end();
    }
    catch (acmacs::lispmds::keyword_no_found&) {
        return false;
    }

} // LispmdsAntigen::reference

// ----------------------------------------------------------------------

static inline std::string serum_name(const acmacs::lispmds::value& aData, size_t aIndex)
{
    return *std::get<acmacs::lispmds::symbol>(acmacs::lispmds::get(aData, 0, 2, aIndex));
}

// ----------------------------------------------------------------------

ae::virus::Name LispmdsSerum::name() const
{
    ae::virus::Name name;
    ae::virus::Reassortant reassortant;
    SerumId serum_id;
    Annotations annotations;
    lispmds_serum_name_decode(serum_name(mData, mIndex), name, reassortant, annotations, serum_id);
    return name;

} // LispmdsSerum::name

// ----------------------------------------------------------------------

ae::virus::Reassortant LispmdsSerum::reassortant() const
{
    ae::virus::Name name;
    ae::virus::Reassortant reassortant;
    SerumId serum_id;
    Annotations annotations;
    lispmds_serum_name_decode(serum_name(mData, mIndex), name, reassortant, annotations, serum_id);
    return reassortant;

} // LispmdsSerum::reassortant

// ----------------------------------------------------------------------

Annotations LispmdsSerum::annotations() const
{
    ae::virus::Name name;
    ae::virus::Reassortant reassortant;
    SerumId serum_id;
    Annotations annotations;
    lispmds_serum_name_decode(serum_name(mData, mIndex), name, reassortant, annotations, serum_id);
    return annotations;

} // LispmdsSerum::annotations

// ----------------------------------------------------------------------

SerumId LispmdsSerum::serum_id() const
{
    ae::virus::Name name;
    ae::virus::Reassortant reassortant;
    SerumId serum_id;
    Annotations annotations;
    lispmds_serum_name_decode(serum_name(mData, mIndex), name, reassortant, annotations, serum_id);
    return serum_id;

} // LispmdsSerum::serum_id

// ----------------------------------------------------------------------

size_t LispmdsAntigens::size() const
{
    return number_of_antigens(mData);

} // LispmdsAntigens::size

// ----------------------------------------------------------------------

AntigenP LispmdsAntigens::operator[](size_t aIndex) const
{
    return std::make_shared<LispmdsAntigen>(mData, aIndex);

} // LispmdsAntigens::operator[]

// ----------------------------------------------------------------------

size_t LispmdsSera::size() const
{
    return number_of_sera(mData);

} // LispmdsSera::size

// ----------------------------------------------------------------------

SerumP LispmdsSera::operator[](size_t aIndex) const
{
    return std::make_shared<LispmdsSerum>(mData, aIndex);

} // LispmdsSera::operator[]

// ----------------------------------------------------------------------

Titer LispmdsTiters::titer(size_t aAntigenNo, size_t aSerumNo) const
{
    return std::visit(
        [](auto&& titer_x) -> Titer {
            using T = std::decay_t<decltype(titer_x)>;
            if constexpr (std::is_same_v<T, acmacs::lispmds::symbol>) {
                if (titer_x[0] == '*')
                    return Titer{std::string(1, titer_x[0])};
                return Titer{fmt::format("{}{}", titer_x[0], std::lround(std::exp2(std::stod(titer_x->substr(1))) * 10))};
            }
            else if constexpr (std::is_same_v<T, acmacs::lispmds::number>) {
                return Titer{fmt::format("{}", std::lround(std::exp2(static_cast<double>(titer_x)) * 10))};
            }
            else
                throw acmacs::lispmds::type_mismatch{fmt::format("Unexpected titer type: {}", typeid(T).name())};
        },
        acmacs::lispmds::get(mData, 0, 3, aAntigenNo, aSerumNo));

} // LispmdsTiters::titer

// ----------------------------------------------------------------------

size_t LispmdsTiters::number_of_antigens() const
{
    return acmacs::lispmds::size(mData, 0, 3);

} // LispmdsTiters::number_of_antigens

// ----------------------------------------------------------------------

size_t LispmdsTiters::number_of_sera() const
{
    return acmacs::lispmds::size(mData, 0, 3, 0);

} // LispmdsTiters::number_of_sera

// ----------------------------------------------------------------------

inline bool is_dontcare(const acmacs::lispmds::value& titer)
{
    return std::visit(
        [](auto&& arg) -> bool {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, acmacs::lispmds::symbol>)
                return arg[0] == '*';
            else if constexpr (std::is_same_v<T, acmacs::lispmds::number>)
                return false;
            else
                throw acmacs::lispmds::type_mismatch{fmt::format("Unexpected titer type: {}", typeid(T).name())};
        },
        titer);
}

// ----------------------------------------------------------------------

size_t LispmdsTiters::number_of_non_dont_cares() const
{
    size_t result = 0;
    for (const auto& row : std::get<acmacs::lispmds::list>(acmacs::lispmds::get(mData, 0, 3))) {
        for (const auto& titer : std::get<acmacs::lispmds::list>(row)) {
            if (!is_dontcare(titer))
                ++result;
        }
    }
    return result;

} // LispmdsTiters::number_of_non_dont_cares

// ----------------------------------------------------------------------

size_t LispmdsTiters::titrations_for_antigen(size_t antigen_no) const
{
    size_t result = 0;
    const auto& row = std::get<acmacs::lispmds::list>(acmacs::lispmds::get(mData, 0, 3))[antigen_no];
    for (const auto& titer : std::get<acmacs::lispmds::list>(row)) {
        if (!is_dontcare(titer))
            ++result;
    }
    return result;

} // LispmdsTiters::titrations_for_antigen

// ----------------------------------------------------------------------

size_t LispmdsTiters::titrations_for_serum(size_t serum_no) const
{
    size_t result = 0;
    for (const auto& row : std::get<acmacs::lispmds::list>(acmacs::lispmds::get(mData, 0, 3))) {
        if (!is_dontcare(std::get<acmacs::lispmds::list>(row)[serum_no]))
            ++result;
    }
    return result;

} // LispmdsTiters::titrations_for_serum

// ----------------------------------------------------------------------

void LispmdsProjection::check() const
{
    try {
        if (auto nd = layout()->number_of_dimensions(); *nd > 5)
            throw import_error{fmt::format("[lispmds] projection {} has unsupported number of dimensions: {}", projection_no(), nd)};
    }
    catch (std::exception& err) {
        throw import_error{fmt::format("[lispmds] projection {} reading error: {}", projection_no(), err.what())};
    }

} // LispmdsProjection::check

// ----------------------------------------------------------------------

std::optional<double> LispmdsProjection::stored_stress() const
{
    return std::visit(
        [](auto&& arg) -> std::optional<double> {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, acmacs::lispmds::number>)
                return static_cast<double>(arg);
            else
                return {};
        },
        acmacs::lispmds::get(projection_data(mData, projection_no()), 1));

} // LispmdsProjection::stress

// ----------------------------------------------------------------------

class LispmdsLayout : public ae::chart::v2::Layout
{
 public:
    LispmdsLayout(const acmacs::lispmds::value& aData, size_t aNumberOfAntigens, size_t aNumberOfSera)
        : ae::chart::v2::Layout(aNumberOfAntigens + aNumberOfSera, number_of_dimensions_t{acmacs::lispmds::size(aData, 0)})
        {
            auto target = Vec::begin();
            for (size_t p_no = 0; p_no < number_of_points(); ++p_no) {
                const auto& point = acmacs::lispmds::get(aData, p_no);
                if (const auto ps = acmacs::lispmds::size(point); ps == *number_of_dimensions()) {
                    for (auto dim : range_from_0_to(ps))
                        *target++ = std::get<acmacs::lispmds::number>(acmacs::lispmds::get(point, dim));
                }
                else if (ps > 0)
                    throw invalid_data(fmt::format("LispmdsLayout: point has invalid number of coordinates: {}, expected 0 or {}", ps, number_of_dimensions()));
                else
                    target += static_cast<decltype(target)::difference_type>(*number_of_dimensions());
            }
        }

}; // class LispmdsLayout

// ----------------------------------------------------------------------

std::shared_ptr<ae::chart::v2::Layout> LispmdsProjection::layout() const
{
    // std::cerr << "antigens: " << mNumberOfAntigens << " sera: " << mNumberOfSera << " points: " << (mNumberOfAntigens + mNumberOfSera) << '\n';
    if (!layout_)
        layout_ = std::make_shared<LispmdsLayout>(projection_layout(mData, projection_no()), mNumberOfAntigens, mNumberOfSera);
    return layout_;

} // LispmdsProjection::layout

// ----------------------------------------------------------------------

number_of_dimensions_t LispmdsProjection::number_of_dimensions() const
{
    return number_of_dimensions_t{acmacs::lispmds::size(projection_layout(mData, projection_no()), 0)};

} // LispmdsProjection::number_of_dimensions

// ----------------------------------------------------------------------

ColumnBasesP LispmdsProjection::forced_column_bases() const
{
    return ::forced_column_bases(mData, projection_no()).first;

} // LispmdsProjection::forced_column_bases

// ----------------------------------------------------------------------

ae::chart::v2::MinimumColumnBasis LispmdsProjection::minimum_column_basis() const
{
    return ::forced_column_bases(mData, projection_no()).second;

} // LispmdsProjection::minimum_column_basis

// ----------------------------------------------------------------------

ae::draw::v1::Transformation LispmdsProjection::transformation() const
{
    ae::draw::v1::Transformation result;
    try {
        if (const auto& coord_tr = acmacs::lispmds::get(mData, ":CANVAS-COORD-TRANSFORMATIONS"); !acmacs::lispmds::empty(coord_tr)) {
            try {
                if (const auto& v0 = acmacs::lispmds::get(coord_tr, ":CANVAS-BASIS-VECTOR-0"); !acmacs::lispmds::empty(v0)) {
                    result.a() = std::get<acmacs::lispmds::number>(acmacs::lispmds::get(v0, 0));
                    result.c() = std::get<acmacs::lispmds::number>(acmacs::lispmds::get(v0, 1));
                }
            }
            catch (std::exception&) {
            }
            try {
                if (const auto& v1 = acmacs::lispmds::get(coord_tr, ":CANVAS-BASIS-VECTOR-1"); !acmacs::lispmds::empty(v1)) {
                    result.b() = std::get<acmacs::lispmds::number>(acmacs::lispmds::get(v1, 0));
                    result.d() = std::get<acmacs::lispmds::number>(acmacs::lispmds::get(v1, 1));
                }
            }
            catch (std::exception&) {
            }
            try {
                if (static_cast<double>(std::get<acmacs::lispmds::number>(acmacs::lispmds::get(coord_tr, ":CANVAS-X-COORD-SCALE"))) < 0) {
                    result.a() = - result.a();
                    result.c() = - result.c();
                }
            }
            catch (std::exception&) {
            }
            try {
                if (static_cast<double>(std::get<acmacs::lispmds::number>(acmacs::lispmds::get(coord_tr, ":CANVAS-Y-COORD-SCALE"))) < 0) {
                    result.b() = - result.b();
                    result.d() = - result.d();
                }
            }
            catch (std::exception&) {
            }
        }
    }
    catch (acmacs::lispmds::keyword_no_found&) {
    }
    return result;

} // LispmdsProjection::transformation

// ----------------------------------------------------------------------

UnmovablePoints LispmdsProjection::unmovable() const
{
      //   :UNMOVEABLE-COORDS '(87 86 85 83 82 80 81)
    try {
        const auto& val = std::get<acmacs::lispmds::list>(acmacs::lispmds::get(mData, ":UNMOVEABLE-COORDS"));
        return {val.begin(), val.end(), [](const auto& v) -> size_t { return std::get<acmacs::lispmds::number>(v); }};
    }
    catch (std::exception&) {
        return UnmovablePoints{};
    }

} // LispmdsProjection::unmovable

// ----------------------------------------------------------------------

DisconnectedPoints LispmdsProjection::disconnected() const
{
      // std::cerr << "WARNING: LispmdsProjection::disconnected not implemented\n";
    return DisconnectedPoints{};

} // LispmdsProjection::disconnected

// ----------------------------------------------------------------------

AvidityAdjusts LispmdsProjection::avidity_adjusts() const
{
    try {
        const auto num_points = layout()->number_of_points();
        const acmacs::lispmds::list& cb = std::get<acmacs::lispmds::list>(acmacs::lispmds::get(projection_layout(mData, projection_no()), num_points, 0, 1));
        AvidityAdjusts result(num_points);
        for (size_t i = 0; i < num_points; ++i)
            result[i] = std::exp2(static_cast<double>(std::get<acmacs::lispmds::number>(cb[num_points + i])));
        return result;
    }
    catch (acmacs::lispmds::error& err) {
        AD_ERROR("[lispmds avidity_adjusts] broken save: {}", err.what());
        return AvidityAdjusts{};
    }
    catch (std::exception& err) {
        AD_ERROR("[lispmds avidity_adjusts]: {}\n", err.what());
        throw;
    }

} // LispmdsProjection::avidity_adjusts

// ----------------------------------------------------------------------

bool LispmdsProjections::empty() const
{
    return acmacs::lispmds::empty(mData, ":STARTING-COORDSS") && acmacs::lispmds::empty(mData, ":BATCH-RUNS");

} // LispmdsProjections::empty

// ----------------------------------------------------------------------

size_t LispmdsProjections::size() const
{
    size_t result = 0;
    if (!acmacs::lispmds::empty(mData, ":STARTING-COORDSS"))
        ++result;
    try {
        result += acmacs::lispmds::size(mData, ":BATCH-RUNS");
    }
    catch (acmacs::lispmds::error&) {
    }
    return result;

} // LispmdsProjections::size

// ----------------------------------------------------------------------

ProjectionP LispmdsProjections::operator[](size_t aIndex) const
{
    if (!projections_[aIndex])
        projections_[aIndex] = std::make_shared<LispmdsProjection>(chart(), mData, aIndex, number_of_antigens(mData), number_of_sera(mData));
    return projections_[aIndex];

} // LispmdsProjections::operator[]

// ----------------------------------------------------------------------

bool LispmdsPlotSpec::empty() const
{
    try {
        return acmacs::lispmds::empty(mData, ":PLOT-SPEC");
    }
    catch (std::exception&) {
        return true;
    }

} // LispmdsPlotSpec::empty

// ----------------------------------------------------------------------

DrawingOrder LispmdsPlotSpec::drawing_order() const
{
      // :RAISE-POINTS 'NIL
      // :LOWER-POINTS 'NIL
      // don't know how drawing order is stored
    return DrawingOrder{};

} // LispmdsPlotSpec::drawing_order

// ----------------------------------------------------------------------

Color LispmdsPlotSpec::error_line_positive_color() const
{
    return RED;

} // LispmdsPlotSpec::error_line_positive_color

// ----------------------------------------------------------------------

Color LispmdsPlotSpec::error_line_negative_color() const
{
    return BLUE;

} // LispmdsPlotSpec::error_line_negative_color

// ----------------------------------------------------------------------

acmacs::PointStyle LispmdsPlotSpec::style(size_t aPointNo) const
{
    acmacs::PointStyle result;
    extract_style(result, aPointNo);
    return result;

} // LispmdsPlotSpec::style

// ----------------------------------------------------------------------

std::vector<acmacs::PointStyle> LispmdsPlotSpec::all_styles() const
{
    try {
        const auto number_of_points = acmacs::lispmds::size(mData, 0, 1) + acmacs::lispmds::size(mData, 0, 2);
        std::vector<acmacs::PointStyle> result(number_of_points);
        for (size_t point_no = 0; point_no < number_of_points; ++point_no) {
            extract_style(result[point_no], point_no);
        }
        return result;
    }
    catch (std::exception& err) {
        AD_WARNING("[lispmds]: cannot get point styles: {}", err.what());
    }
    return {};

} // LispmdsPlotSpec::all_styles

// ----------------------------------------------------------------------

size_t LispmdsPlotSpec::number_of_points() const
{
    try {
        return acmacs::lispmds::size(mData, 0, 1) + acmacs::lispmds::size(mData, 0, 2);
    }
    catch (std::exception& err) {
        AD_WARNING("[lispmds]: cannot get point styles: {}", err.what());
        return 0;
    }

} // LispmdsPlotSpec::number_of_points

// ----------------------------------------------------------------------

void LispmdsPlotSpec::extract_style(acmacs::PointStyle& aTarget, size_t aPointNo) const
{
    std::string name = aPointNo < number_of_antigens(mData)
                                  ? static_cast<std::string>(std::get<acmacs::lispmds::symbol>(acmacs::lispmds::get(mData, 0, 1, aPointNo))) + "-AG"
                                  : static_cast<std::string>(std::get<acmacs::lispmds::symbol>(acmacs::lispmds::get(mData, 0, 2, aPointNo - number_of_antigens(mData)))) + "-SR";
    const auto& plot_spec = std::get<acmacs::lispmds::list>(acmacs::lispmds::get(mData, ":PLOT-SPEC"));
    for (const auto& pstyle : plot_spec) {
        if (std::get<acmacs::lispmds::symbol>(acmacs::lispmds::get(pstyle, 0)) == acmacs::lispmds::symbol{name}) {
            extract_style(aTarget, std::get<acmacs::lispmds::list>(pstyle));
            break;
        }
    }

} // LispmdsPlotSpec::extract_style

// ----------------------------------------------------------------------

void LispmdsPlotSpec::extract_style(acmacs::PointStyle& aTarget, const acmacs::lispmds::list& aSource) const
{
    try {
        aTarget.size(ae::draw::v1::Pixels{static_cast<double>(std::get<acmacs::lispmds::number>(aSource[":DS"])) / acmacs::lispmds::DS_SCALE});
          // if antigen also divide size by 2 ?
    }
    catch (std::exception&) {
    }

    try {
        aTarget.label_text(*std::get<acmacs::lispmds::string>(aSource[":WN"]));
        aTarget.label().shown = !aTarget.label_text().empty();
    }
    catch (std::exception&) {
    }

    try {
        aTarget.shape(acmacs::PointShape{*std::get<acmacs::lispmds::string>(aSource[":SH"])});
    }
    catch (std::exception&) {
    }

    try {
        aTarget.label().size = ae::draw::v1::Pixels{static_cast<double>(std::get<acmacs::lispmds::number>(aSource[":NS"])) / acmacs::lispmds::NS_SCALE};
    }
    catch (std::exception&) {
    }

    try {
        if (const auto label_color = std::get<acmacs::lispmds::string>(aSource[":NC"]); label_color != acmacs::lispmds::string{"{}"})
            aTarget.label().color = acmacs::color::Modifier{Color(*label_color)};
    }
    catch (std::exception&) {
    }

    try {
        if (const auto fill_color = std::get<acmacs::lispmds::string>(aSource[":CO"]); fill_color != acmacs::lispmds::string{"{}"})
            aTarget.fill(Color(*fill_color));
        else
            aTarget.fill(TRANSPARENT);
    }
    catch (std::exception&) {
    }

    try {
        if (const auto outline_color = std::get<acmacs::lispmds::string>(aSource[":OC"]); outline_color != acmacs::lispmds::string{"{}"})
            aTarget.outline(Color(*outline_color));
        else
            aTarget.outline(TRANSPARENT);
    }
    catch (std::exception&) {
    }

    try {
        aTarget.fill(acmacs::color::Modifier{acmacs::color::Modifier::transparency_set{static_cast<double>(std::get<acmacs::lispmds::number>(aSource[":TR"]))}});
    }
    catch (std::exception&) {
    }

} // LispmdsPlotSpec::extract_style

// ----------------------------------------------------------------------
