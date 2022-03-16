#include <numeric>
#include <unordered_map>
#include <unordered_set>

#include "ext/from_chars.hh"
// #include "ext/range-v3.hh"
#include "utils/statistics.hh"
#include "chart/v3/titers.hh"

// ----------------------------------------------------------------------

std::string_view ae::chart::v3::Titer::validate(std::string_view titer)
{
    if (titer.empty())
        throw invalid_titer(titer);

    const auto just_digits = [titer](auto&& data) {
        if (!std::all_of(std::begin(data), std::end(data), [](auto val) { return std::isdigit(val); }))
            throw invalid_titer(titer);
    };

    switch (titer.front()) {
      case '*':
          if (titer.size() != 1)
              throw invalid_titer(titer);
          break;
      case '<':
      case '>':
      case '~':
          just_digits(titer.substr(1));
          break;
      default:
          just_digits(titer);
          break;
    }
    return titer;

} // ae::chart::v3::Titer::validate

// ----------------------------------------------------------------------

double ae::chart::v3::Titer::logged_with_thresholded() const
{
    switch (type()) {
      case Invalid:
      case Regular:
      case DontCare:
      case Dodgy:
          return logged();
      case LessThan:
          return logged() - 1;
      case MoreThan:
          return logged() + 1;
    }
    throw invalid_titer(*this); // for gcc 7.2

} // ae::chart::v3::Titer::logged_with_thresholded

// ----------------------------------------------------------------------

std::string ae::chart::v3::Titer::logged_as_string() const
{
    switch (type()) {
      case Invalid:
          throw invalid_titer(*this);
      case Regular:
          return fmt::format("{}", logged());
      case DontCare:
          return std::string{get()};
      case LessThan:
      case MoreThan:
      case Dodgy:
          return fmt::format("{}{}", get().front(), logged());
    }
    throw invalid_titer(*this); // for gcc 7.2

} // ae::chart::v3::Titer::logged_as_string

// ----------------------------------------------------------------------

double ae::chart::v3::Titer::logged_for_column_bases() const
{
    switch (type()) {
      case Invalid:
          throw invalid_titer(*this);
      case Regular:
      case LessThan:
          return logged();
      case MoreThan:
          return logged() + 1;
      case DontCare:
      case Dodgy:
          return -1;
    }
    throw invalid_titer(*this); // for gcc 7.2

} // ae::chart::v3::Titer::logged_for_column_bases

// ----------------------------------------------------------------------

size_t ae::chart::v3::Titer::value_for_sorting() const
{
    switch (type()) {
      case Invalid:
      case DontCare:
          return 0;
      case Regular:
          return from_chars<size_t>(get());
      case LessThan:
          return from_chars<size_t>(get().substr(1)) - 1;
      case MoreThan:
          return from_chars<size_t>(get().substr(1)) + 1;
      case Dodgy:
          return from_chars<size_t>(get().substr(1));
    }
    return 0;

} // ae::chart::v3::Titer::value_for_sorting

// ----------------------------------------------------------------------

size_t ae::chart::v3::Titer::value() const
{
    switch (type()) {
      case Invalid:
      case DontCare:
          return 0;
      case Regular:
          return from_chars<size_t>(get());
      case LessThan:
      case MoreThan:
      case Dodgy:
          return from_chars<size_t>(get().substr(1));
    }
    return 0;

} // ae::chart::v3::Titer::value

// ----------------------------------------------------------------------

size_t ae::chart::v3::Titer::value_with_thresholded() const
{
    switch (type()) {
      case Invalid:
      case DontCare:
          return 0;
      case Regular:
          return from_chars<size_t>(get());
      case LessThan:
          return from_chars<size_t>(get().substr(1)) / 2;
      case MoreThan:
          return from_chars<size_t>(get().substr(1)) * 2;
      case Dodgy:
          return from_chars<size_t>(get().substr(1));
    }
    return 0;

} // ae::chart::v3::Titer::value_with_thresholded

// ----------------------------------------------------------------------

ae::chart::v3::Titer ae::chart::v3::Titer::multiplied_by(double value) const // multiplied_by(2) returns 80 for 40 and <80 for <40, * for *
{
    switch (type()) {
      case Invalid:
      case DontCare:
          return *this;
      case Regular:
          return Titer{fmt::format("{}", std::lround(static_cast<double>(std::stoul(get())) * value))};
      case LessThan:
      case MoreThan:
      case Dodgy:
          return Titer{fmt::format("{}{}", get().front(), std::lround(static_cast<double>(std::stoul(get().substr(1))) * value))};
    }
    return Titer{};

} // ae::chart::v3::Titer::multiplied_by

// ----------------------------------------------------------------------

ae::chart::v3::Titer ae::chart::v3::Titers::titer(antigen_index aAntigenNo, serum_index aSerumNo) const
{
    auto get = [this,aAntigenNo,aSerumNo](const auto& titers) -> Titer {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>)
            return titers[aAntigenNo.get() * this->number_of_sera_.get() + aSerumNo.get()];
        else
            return titer_in_sparse_t(titers, aAntigenNo, aSerumNo);
    };
    return std::visit(get, titers_);

} // ae::chart::v3::Titers::titer

// ----------------------------------------------------------------------

std::vector<ae::chart::v3::Titer> ae::chart::v3::Titers::titers_for_layers(antigen_index aAntigenNo, serum_index aSerumNo, include_dotcare inc) const // returns list of non-dont-care titers in layers, may throw data_not_available
{
    check_layers();
    std::vector<Titer> result;
    for (const auto& layer: layers_) {
        if (const auto titer = find_titer_for_serum(layer[aAntigenNo.get()], aSerumNo); !titer.is_dont_care())
            result.push_back(titer);
        else if (inc == include_dotcare::yes)
            result.push_back({});
    }
    return result;

} // ae::chart::v3::Titers::titers_for_layers

// ----------------------------------------------------------------------

std::vector<ae::layer_index> ae::chart::v3::Titers::layers_with_antigen(antigen_index aAntigenNo) const // returns list of layer indexes that have non-dont-care titers for the antigen, may throw data_not_available
{
    check_layers();
    std::vector<layer_index> result;
    for (const auto no : number_of_layers()) {
        const auto& layer = this->layer(no);
        for (const auto serum_no : number_of_sera()) {
            if (const auto titer = find_titer_for_serum(layer[aAntigenNo.get()], serum_no); !titer.is_dont_care()) {
                result.push_back(no);
                break;
            }
        }
    }
    return result;

} // ae::chart::v3::Titers::layers_with_antigen

// ----------------------------------------------------------------------

std::vector<ae::layer_index> ae::chart::v3::Titers::layers_with_serum(serum_index aSerumNo) const // returns list of layer indexes that have non-dont-care titers for the serum, may throw data_not_available
{
    check_layers();
    std::vector<layer_index> result;
    for (const auto no : number_of_layers()) {
        const auto& layer = this->layer(no);
        for (auto antigen_no : number_of_antigens()) {
            if (const auto titer = find_titer_for_serum(layer[antigen_no.get()], aSerumNo); !titer.is_dont_care()) {
                result.push_back(no);
                break;
            }
        }
    }
    return result;

} // ae::chart::v3::Titers::layers_with_serum

// ----------------------------------------------------------------------

ae::antigen_index ae::chart::v3::Titers::number_of_antigens() const
{
    auto num_ags = [this]<typename TT>(const TT& titers) -> antigen_index {
        if constexpr (std::is_same_v<TT, dense_t>)
            return antigen_index{titers.size() / this->number_of_sera_.get()};
        else
            return antigen_index{titers.size()};
    };
    return std::visit(num_ags, titers_);

} // ae::chart::v3::Titers::number_of_antigens

// ----------------------------------------------------------------------

size_t ae::chart::v3::Titers::number_of_non_dont_cares() const
{
    auto num_non_dont_cares = [](const auto& titers) -> size_t {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>)
            return std::accumulate(titers.begin(), titers.end(), size_t{0}, [](size_t a, const auto& titer) -> size_t { return a + (titer.is_dont_care() ? size_t{0} : size_t{1}); });
        else
            return std::accumulate(titers.begin(), titers.end(), size_t{0}, [](size_t a, const auto& row) -> size_t { return a + row.size(); });
    };
    return std::visit(num_non_dont_cares, titers_);

} // ae::chart::v3::Titers::number_of_non_dont_cares

// ----------------------------------------------------------------------

size_t ae::chart::v3::Titers::titrations_for_antigen(antigen_index antigen_no) const
{
    auto num_non_dont_cares = [antigen_no, this](const auto& titers) -> size_t {
        using T = std::decay_t<decltype(titers)>;
        if constexpr (std::is_same_v<T, dense_t>)
            return static_cast<size_t>(std::count_if(&titers[antigen_no.get() * this->number_of_sera_.get()], &titers[(antigen_no.get() + 1) * this->number_of_sera_.get()],
                                                     [](const Titer& titer) { return !titer.is_dont_care(); }));
        else
            return titers[antigen_no.get()].size();
    };
    return std::visit(num_non_dont_cares, titers_);

} // ae::chart::v3::Titers::titrations_for_antigen

// ----------------------------------------------------------------------

size_t ae::chart::v3::Titers::titrations_for_serum(serum_index serum_no) const
{
    auto num_non_dont_cares = [serum_no, this](const auto& titers) -> size_t {
        using T = std::decay_t<decltype(titers)>;
        size_t result{0};
        if constexpr (std::is_same_v<T, dense_t>) {
            for (auto antigen_no = 0ul; antigen_no < (titers.size() / this->number_of_sera_.get()); ++antigen_no) {
                if (!titers[antigen_no * this->number_of_sera_.get() + serum_no.get()].is_dont_care())
                    ++result;
            }
        }
        else {
            for (auto& row : titers) {
                if (auto found = std::lower_bound(row.begin(), row.end(), serum_no, [](const auto& e1, auto sr_no) { return e1.first < sr_no; });
                    found != row.end() && found->first == serum_no) {
                    if (!found->second.is_dont_care())
                        ++result;
                }
            }
        }
        return result;
    };
    return std::visit(num_non_dont_cares, titers_);

} // ae::chart::v3::Titers::titrations_for_serum

// ----------------------------------------------------------------------

std::pair<ae::antigen_indexes, ae::serum_indexes> ae::chart::v3::Titers::antigens_sera_of_layer(layer_index aLayerNo) const
{
    ae::antigen_indexes antigens;
    ae::serum_indexes sera;

    for (const auto& titer_ref : titers_existing_from_layer(aLayerNo)) {
        antigens.insert_if_not_present(titer_ref.antigen);
        sera.insert_if_not_present(titer_ref.serum);
    }
    return {antigens, sera};

} // ae::chart::v3::Titers::antigens_sera_of_layer

// ----------------------------------------------------------------------

std::pair<ae::antigen_indexes, ae::serum_indexes> ae::chart::v3::Titers::antigens_sera_in_multiple_layers() const
{
    using layer_set = std::unordered_set<layer_index, index_hash_for_unordered_map, std::equal_to<>>;
    std::unordered_map<antigen_index, layer_set, index_hash_for_unordered_map, std::equal_to<>> antigen_to_layers;
    std::unordered_map<serum_index, layer_set, index_hash_for_unordered_map, std::equal_to<>> serum_to_layers;

    for (const auto layer_no :number_of_layers()) {
        for (const auto& titer_ref : titers_existing_from_layer(layer_no)) {
            antigen_to_layers[titer_ref.antigen].insert(layer_no);
            serum_to_layers[titer_ref.serum].insert(layer_no);
        }
    }
    ae::antigen_indexes antigens;
    ae::serum_indexes sera;
    for (const auto& ag : antigen_to_layers) {
        if (ag.second.size() > 1)
            antigens.insert_if_not_present(ag.first);
    }
    for (const auto& sr : serum_to_layers) {
        if (sr.second.size() > 1)
            sera.insert_if_not_present(sr.first);
    }
    return {antigens, sera};

} // ae::chart::v3::Titers::antigens_sera_in_multiple_layers

// ----------------------------------------------------------------------

bool ae::chart::v3::Titers::has_morethan_in_layers() const
{
    for (const auto  layer_no : number_of_layers()) {
        for (const auto& titer_ref : titers_existing_from_layer(layer_no)) {
            if (titer_ref.titer.is_more_than())
                return true;
        }
    }
    return false;

} // ae::chart::v3::Titers::has_morethan_in_layers

// ----------------------------------------------------------------------

ae::point_indexes ae::chart::v3::Titers::having_titers_with(point_index point_no, bool return_point_no) const
{
    point_indexes result;
    if (point_no.get() < number_of_antigens().get()) {
        const auto base = return_point_no ? number_of_antigens().get() : 0ul;
        for (const auto& titer_ref : titers_existing()) {
            if (titer_ref.antigen.get() == point_no.get())
                result.push_back(point_index{titer_ref.serum.get() + base});
        }
    }
    else {
        const auto serum_no = point_no.get() - number_of_antigens().get();
        for (const auto& titer_ref : titers_existing()) {
            if (titer_ref.serum.get() == serum_no)
                result.push_back(point_index{titer_ref.antigen});
        }
    }
    return result;

} // ae::chart::v3::Titers::having_titers_with

// ----------------------------------------------------------------------

ae::point_indexes ae::chart::v3::Titers::having_too_few_numeric_titers(size_t threshold) const
{
    std::vector<size_t> number_of_numeric_titers(*(number_of_antigens() + number_of_sera()), 0);
    for (const auto& titer_ref : titers_existing()) {
        AD_DEBUG(titer_ref.serum == serum_index{16}, "{:3d} {:3d} {} {}", titer_ref.antigen, titer_ref.serum, titer_ref.titer, titer_ref.titer.is_regular());
        if (titer_ref.titer.is_regular()) {
            ++number_of_numeric_titers[*titer_ref.antigen];
            ++number_of_numeric_titers[*(number_of_antigens() + titer_ref.serum)];
        }
    }
    point_indexes result;
    for (auto ind = number_of_numeric_titers.begin(); ind != number_of_numeric_titers.end(); ++ind) {
        // AD_DEBUG("having_too_few_numeric_titers {:4d}  {:3d}", ind - number_of_numeric_titers.begin(), *ind);
        if (*ind < threshold)
            result.push_back(point_index{ind - number_of_numeric_titers.begin()});
    }
    return result;

} // ae::chart::v3::Titers::having_too_few_numeric_titers

// ----------------------------------------------------------------------

ae::chart::v3::Titer ae::chart::v3::Titers::find_titer_for_serum(const sparse_row_t& aRow, serum_index aSerumNo)
{
    if (aRow.empty())
        return {};
    if (const auto found = std::lower_bound(aRow.begin(), aRow.end(), aSerumNo, [](const auto& e1, serum_index sr_no) { return e1.first < sr_no; }); found != aRow.end() && found->first == aSerumNo)
        return found->second;
    return {};

} // ae::chart::v3::Titers::find_titer_for_serum

// ----------------------------------------------------------------------

void ae::chart::v3::Titers::set_titer(sparse_t& titers, antigen_index aAntigenNo, serum_index aSerumNo, const Titer& aTiter)
{
    auto& row = titers[aAntigenNo.get()];
    if (row.empty()) {
        row.emplace_back(aSerumNo, aTiter);
    }
    else {
        if (auto found = std::lower_bound(row.begin(), row.end(), aSerumNo, [](const auto& e1, serum_index sr_no) { return e1.first < sr_no; }); found != row.end() && found->first == aSerumNo)
            found->second = aTiter;
        else
            row.emplace(found, aSerumNo, aTiter);
    }

} // ae::chart::v3::Titers::set_titer

// ----------------------------------------------------------------------

// if there are more-than thresholded titers and more_than_thresholded
// is 'dont-care', ignore them, if more_than_thresholded is
// 'adjust-to-next', those titers are converted to the next value,
// e.g. >5120 to 10240.
std::unique_ptr<ae::chart::v3::Titers::titer_merge_report> ae::chart::v3::Titers::set_titers_from_layers(more_than_thresholded mtt)
{
    // core/antigenic_table.py:266
    // backend/antigenic-table.hh:892

    constexpr double standard_deviation_threshold = 1.0; // lispmds: average-multiples-unless-sd-gt-1-ignore-thresholded-unless-only-entries-then-min-threshold
    const antigen_index number_of_antigens{layers_[0].size()};
    auto titers = std::make_unique<titer_merge_report>();
    for (const auto ag_no : number_of_antigens) {
        for (const auto sr_no : number_of_sera_) {
            auto [titer, report] = titer_from_layers(ag_no, sr_no, mtt, standard_deviation_threshold);
            titers->emplace_back(std::move(titer), ag_no, sr_no, report);
        }
    }

    if (titers->size() < (number_of_antigens.get() * number_of_sera_.get() / 2))
        titers_ = sparse_t(number_of_antigens.get());
    else
        titers_ = dense_t(number_of_antigens.get() * number_of_sera_.get());
    for (const auto& data : *titers) {
        if (!data.titer.is_dont_care())
            std::visit([&data, this](auto& target) { this->set_titer(target, data.antigen, data.serum, data.titer); }, titers_);
    }

    return titers;

} // ae::chart::v3::Titers::set_titers_from_layers

// ----------------------------------------------------------------------

std::pair<ae::chart::v3::Titer, ae::chart::v3::Titers::titer_merge> ae::chart::v3::Titers::titer_from_layers(antigen_index aAntigenNo, serum_index aSerumNo, more_than_thresholded mtt, double standard_deviation_threshold)
{
    std::vector<Titer> titers;
    for (const auto layer_no : number_of_layers()) {
        if (auto titer = titer_in_sparse_t(layers_[layer_no.get()], aAntigenNo, aSerumNo); !titer.is_dont_care()) {
            titers.emplace_back(std::move(titer));
        }
    }

    return merge_titers(titers, mtt, standard_deviation_threshold);

} // ae::chart::v3::Titers::titer_from_layers

// ----------------------------------------------------------------------

// lispmds algorithm 2014-12-06 (from mds/src/mds/mds/hi-table.lisp)
// lispmds does not support > at all, it is added to acmacs
// 1. If there are > and < titers, result is *
// 2. If there are just *, result is *
// 3. If there are just thresholded titers, result is min (<) or max (>) of them
// 4. Convert > and < titers to their next values, i.e. <40 to 20, >10240 to 20480, etc.
// 5. Compute SD, if SD > 1, result is *
// 6. If there are no < nor >, result is mean of log titers.
// 7. if max(<) of thresholded is more than max on non-thresholded (e.g. <40 20), then find minimum of thresholded which is more than max on non-thresholded, it is the result with <
// 8. if min(>) of thresholded is less than min on non-thresholded (e.g. >1280 2560), then find maximum of thresholded which is less than min on non-thresholded, it is the result with >
// 9. otherwise result is next of of max/min non-thresholded with </> (e.g. <20 40 --> <80, <20 80 --> <160) "min-more-than >= min-regular", "max-less-than <= max-regular"

// backend/antigenic-table.hh:1087

std::pair<ae::chart::v3::Titer, ae::chart::v3::Titers::titer_merge> ae::chart::v3::Titers::merge_titers(const std::vector<Titer>& titers, more_than_thresholded mtt, double standard_deviation_threshold)
{
    constexpr auto max_limit = std::numeric_limits<decltype(std::declval<Titer>().value())>::max();
    size_t min_less_than = max_limit, min_more_than = max_limit, min_regular = max_limit;
    size_t max_less_than = 0, max_more_than = 0, max_regular = 0;
    for (const auto& titer : titers) {
        const auto val = titer.value();
        switch (titer.type()) {
            case Titer::Regular:
                min_regular = std::min(min_regular, val);
                max_regular = std::max(max_regular, val);
                break;
            case Titer::LessThan:
                min_less_than = std::min(min_less_than, val);
                max_less_than = std::max(max_less_than, val);
                break;
            case Titer::MoreThan:
                min_more_than = std::min(min_more_than, val);
                max_more_than = std::max(max_more_than, val);
                break;
            case Titer::Dodgy:
            case Titer::Invalid:
            case Titer::DontCare:
                throw std::invalid_argument{"TitersModify::merge_titers cannot handle dont-care, dodgy, invalid titers"};
        }
    }

    if (titers.empty()) // 2. just dontcare
        return {{}, titer_merge::all_dontcare};
    if (max_less_than != 0 && min_more_than != max_limit) // 1. both thresholded
        return {{}, titer_merge::less_and_more_than};
    if (min_regular == max_limit) { // 3. no regular, just thresholded
        if (min_less_than != max_limit)
            return {Titer('<', min_less_than), titer_merge::less_than_only};
        if (mtt == more_than_thresholded::adjust_to_next)
            return {Titer('>', max_more_than), titer_merge::more_than_only_adjust_to_next};
        else
            return {{}, titer_merge::more_than_only_to_dontcare};
    }

    // compute SD
    std::vector<double> adjusted_log(titers.size());
    std::transform(titers.begin(), titers.end(), adjusted_log.begin(), [](const auto& titer) -> double { return titer.logged_with_thresholded(); }); // 4.
    const auto sd_mean = ae::statistics::standard_deviation(adjusted_log.begin(), adjusted_log.end());
    if (sd_mean.population_sd() > standard_deviation_threshold)
        return {Titer{}, titer_merge::sd_too_big};        // 5. if SD > 1, result is *
    if (max_less_than == 0 && min_more_than == max_limit) // 6. just regular
        return {Titer::from_logged(sd_mean.mean()), titer_merge::regular_only};
    if (max_less_than) { // 7.
        if (max_less_than > max_regular) {
            auto result = max_less_than;
            for (const auto& titer : titers) {
                if (titer.is_less_than())
                    if (const auto tval = titer.value(); tval < result && tval > max_regular)
                        result = tval;
            }
            return {Titer('<', result), titer_merge::max_less_than_bigger_than_max_regular};
        }
        else
            return {Titer('<', max_regular * 2), titer_merge::less_than_and_regular};
    }
    if (min_more_than < min_regular) { // 8.
        auto result = min_more_than;
        for (const auto& titer : titers) {
            if (titer.is_more_than())
                if (const auto tval = titer.value(); tval > result && tval < min_regular)
                    result = tval;
        }
        return {Titer('>', result), titer_merge::min_more_than_less_than_min_regular};
    }
    else
        return {Titer('>', min_regular / 2), titer_merge::more_than_and_regular};

} // ae::chart::v3::Titers::merge_titers

// ----------------------------------------------------------------------

// raw value, not adjusted by minimum column basis
double ae::chart::v3::Titers::column_basis(serum_index sr_no) const
{
    double cb{0.0};
    for (const auto& titer_ref : titers_existing()) {
        if (titer_ref.serum == sr_no)
            cb = std::max(cb, titer_ref.titer.logged_for_column_bases());
    }
    return cb;

} // ae::chart::v3::Titers::column_basis

// ----------------------------------------------------------------------

double ae::chart::v3::Titers::max_distance(const column_bases& cb) const
{
    double max_distance{0.0};
    if (number_of_sera() > serum_index{0}) {
        for (const auto& titer_ref : titers_existing()) {
            max_distance = std::max(max_distance, cb.column_basis(titer_ref.serum) - titer_ref.titer.logged_with_thresholded());
            if (std::isnan(max_distance) || std::isinf(max_distance))
                throw std::runtime_error{fmt::format("Titers::max_distance invalid: {} after titer [{}] column_bases:{} @@ {}:{}: {}", max_distance, titer_ref, cb, __builtin_FILE(), __builtin_LINE(),
                                                     __builtin_FUNCTION())};
        }
    }
    else {
        throw std::runtime_error(AD_FORMAT("genetic table support not implemented"));
    }

    return max_distance;

} // ae::chart::v3::Titers::max_distance

// ----------------------------------------------------------------------
