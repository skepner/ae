#include <tuple>

#include "chart/v3/serum-circles.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    static void set_theoretical(serum_circles_for_serum_t& serum_data);
    static void set_empirical(serum_circles_for_serum_t& serum_data, const Layout& layout, const Titers& titers);

    class titer_distance_t
    {
      public:
        titer_distance_t(const Titer& aTiter, double aColumnBase, double aDistance)
            : titer{aTiter}, similarity{aTiter.is_dont_care() ? 0.0 : aTiter.logged_for_column_bases()}, final_similarity{std::min(aColumnBase, similarity)}, distance{aDistance}
        {
        }
        titer_distance_t() = default;
        explicit operator bool() const { return !titer.is_dont_care() && !std::isnan(distance); }

        Titer titer{};
        double similarity{0.0};
        double final_similarity{0.0};
        double distance{std::numeric_limits<double>::quiet_NaN()};
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

std::vector<ae::chart::v3::serum_circles_for_serum_t> ae::chart::v3::serum_circles(const Chart& chart, const Projection& projection, serum_circle_fold fold)
{
    const auto column_bases = chart.column_bases(projection.minimum_column_basis());
    std::vector<serum_circles_for_serum_t> circles;

    for (const auto sr_no : chart.sera().size()) {
        // AD_DEBUG("homologous SR {:4d} \"{}\": {}", sr_no, chart.sera()[sr_no].designation(), chart.antigens().homologous(chart.sera()[sr_no]));
        auto& serum_data = circles.emplace_back(sr_no, column_bases[sr_no], fold);
        for (const auto ag_no : chart.antigens().homologous(chart.sera()[sr_no])) {
            serum_data.antigens.push_back({.antigen_no = ag_no, .titer = chart.titers().titer(ag_no, sr_no)});
            set_theoretical(serum_data);
            set_empirical(serum_data, projection.layout(), chart.titers());
        }
    }

    return circles;

} // ae::chart::v3::serum_circles

// ----------------------------------------------------------------------

// If there are many sera made from the same or similar viruses, we
// can try to draw just one circle for them.
//
// Center of the circle is an average point of the sera.
//
// We split antigens into two groups (protected by the sera and not
// protected) and then find circle radius that keeps most of the
// proteced antigens within circle and most of the not proteced
// antigens outside circle, i.e. figure circle radius approximately
// the same way as we do for a single serum circle.
//
// First built a table with cells for each antigen and serum stating
// if that antigen is protected by that serum. Then if conservative is
// true, we exclude from consideration antigens that are protected by
// few sera and not protected by other sera, and then we have two
// groups of antigens but not all antigens are in those groups. If
// conservative is false, antigens with mixed protection status are
// assigned by majority of their statuses (i.e. if most of the sera
// protects that antigen, antigen is considered protected), if numbers
// of sera with protected and not protected status are equal, antigen
// is excluded.
//
// The the radius is determined in the same way as in set_empirical() below.

ae::chart::v3::serum_circle_for_multiple_sera_t ae::chart::v3::serum_circle_for_multiple_sera(const Chart& chart, const Projection& projection, const serum_indexes& sera, serum_circle_fold fold, bool conservative)
{
    const auto column_bases = chart.column_bases(projection.minimum_column_basis());
    const auto& layout = projection.layout();
    const auto& titers = chart.titers();

    struct Protected
    {
        size_t yes{0};
        size_t no{0};
        bool perfect() const { return yes == 0 || no == 0; }
        bool dominates() const { return yes != no; }
        bool prot() const { return yes > no; }
    };

    std::vector<Protected> ag_protected(*chart.antigens().size());
    serum_circle_for_multiple_sera_t data{.serum_no = sera, .fold = fold};
    bool first_serum{true};
    size_t num_connected_sera{0};
    for (const auto serum_no : sera) {
        if (const auto serum_coord = layout[titers.number_of_antigens() + serum_no]; serum_coord.exists()) {
            for (const auto homol_ag_no : chart.antigens().homologous(chart.sera()[serum_no])) {
                const auto homol_titer = chart.titers().titer(homol_ag_no, serum_no);
                if (layout.point_has_coordinates(homol_ag_no) && !homol_titer.is_dont_care()) {
                    if (const double protection_boundary_titer = std::min(column_bases[serum_no], homol_titer.logged_for_column_bases()) - *fold; protection_boundary_titer >= 1.0) {
                        for (const auto ag_no : titers.number_of_antigens()) {
                                const auto titer = chart.titers().titer(ag_no, serum_no);
                                const auto final_similarity = std::min(titer.is_dont_care() ? 0.0 : titer.logged_for_column_bases(), column_bases[serum_no]);
                                if (titer.is_regular() ? final_similarity >= protection_boundary_titer : final_similarity > protection_boundary_titer)
                                    ++ag_protected[*ag_no].yes;
                                else
                                    ++ag_protected[*ag_no].no;
                        }
                        break; // consider just first suitable homologous antigen
                    }
                }
            }

            ++num_connected_sera;
            if (first_serum) {
                data.center = serum_coord;
                first_serum = false;
            }
            else
                data.center += serum_coord;
        }
    }
    data.center /= static_cast<double>(num_connected_sera);

    std::vector<std::tuple<antigen_index, bool, double>> antigen_distances; // if protected, distances to data.center
    size_t num_protected{0};
    for (const auto ag_no : titers.number_of_antigens()) {
        if (const auto& ag_data = ag_protected[*ag_no]; ag_data.dominates() && (!conservative || ag_data.perfect())) {
            antigen_distances.emplace_back(ag_no, ag_data.prot(), ae::chart::v3::distance(layout[ag_no], data.center));
            if (ag_data.prot())
                ++num_protected;
        }
    }
    if (num_protected > 0 && num_protected < antigen_distances.size()) {
        std::sort(std::begin(antigen_distances), std::end(antigen_distances), [](const auto& e1, const auto& e2) { return std::get<double>(e1) < std::get<double>(e2); });

        constexpr const size_t None = static_cast<size_t>(-1);
        size_t best_sum = None;
        double previous_dist = -1.0;
        double sum_radii = 0;
        size_t num_radii = 0;
        for (const auto& en : antigen_distances) {
            const auto dist0 = std::get<double>(en);
            const double radius = previous_dist < 0.0 ? dist0 : (dist0 + previous_dist) / 2.0;
            size_t protected_outside{0}, not_protected_inside{0};
            for (const auto& [ag_no, prot, dist] : antigen_distances) {
                if (const bool inside = dist <= radius; prot && !inside)
                    ++protected_outside;
                else if (!prot && inside)
                    ++not_protected_inside;
            }
            if (const size_t summa = protected_outside + not_protected_inside; best_sum == None || best_sum >= summa) { // if sums are the same, choose the smaller radius (found earlier)
                if (best_sum == summa) {
                    sum_radii += radius;
                    ++num_radii;
                }
                else {
                    sum_radii = radius;
                    num_radii = 1;
                    best_sum = summa;
                }
            }
            previous_dist = dist0;
        }
        data.empirical = sum_radii / static_cast<double>(num_radii);
    }
    else
        AD_WARNING("serum_circle_for_multiple_sera {}: protects everything or nothing", sera);

    return data;

} // ae::chart::v3::serum_circle_for_multiple_sera

// ----------------------------------------------------------------------

inline bool less(std::optional<double> v1, std::optional<double> v2)
{
    if (v1) {
        if (v2)
            return *v1 < *v2;
        else
            return true;
    }
    else
        return false;
}

std::optional<double> ae::chart::v3::serum_circles_for_serum_t::theoretical() const
{
    if (antigens.empty()) {
        AD_WARNING("no theoretical serum circle for SR {}: no homologous antigens", serum_no);
        return std::nullopt;
    }
    const auto val = std::min_element(antigens.begin(), antigens.end(), [](const auto& a1, const auto& a2) { return less(a1.theoretical, a2.theoretical); })->theoretical;
    if (val)
        return std::max(*val, serum_circle_min_radius);
    else
        return val;

} // ae::chart::v3::serum_circle_serum_t::theoretical

// ----------------------------------------------------------------------

std::optional<double> ae::chart::v3::serum_circles_for_serum_t::empirical() const
{
    if (antigens.empty()) {
        AD_WARNING("no empirical serum circle for SR {}: no homologous antigens", serum_no);
        return std::nullopt;
    }
    const auto val = std::min_element(antigens.begin(), antigens.end(), [](const auto& a1, const auto& a2) { return less(a1.empirical, a2.empirical); })->empirical;
    if (val)
        return std::max(*val, serum_circle_min_radius);
    else
        return val;

} // ae::chart::v3::serum_circle_serum_t::empirical

// ----------------------------------------------------------------------

// Low reactors are defined as >4-fold from the homologous titer,
// hence the theoretical radius is 2 units plus the number of 2-folds
// between max titer and the homologous titer for a serum. Saying the
// same thing mathematically the theoretical radius for a serum circle
// is fold + log2(max titer for serum S against any antigen A) - log2(homologous titer for serum S)
// where fold is 2 by default (4-fold).

void ae::chart::v3::set_theoretical(serum_circles_for_serum_t& serum_data)
{
    for (auto& antigen_data : serum_data.antigens) {
        if (antigen_data.titer.is_regular()) {
            antigen_data.theoretical = *serum_data.fold + serum_data.column_basis - antigen_data.titer.logged_for_column_bases();
        }
        else
            antigen_data.status = serum_circle_status::non_regular_homologous_titer;
    }

} // ae::chart::v3::set_theoretical

// ----------------------------------------------------------------------

// Description of empirical radius calculation found in my message to Derek 2015-09-21 12:03 Subject: Serum protection radius
//
// Program "draws" some circle around a serum with some radius. Then for
// each antigen having titer with that serum program calculates:
// 1. Theoretical protection, i.e. if titer for antigen and serum is more
// or equal than (homologous-titer - fold) (fold = 2 by default)
// 2. Empirical protection, i.e. if antigen is inside the drawn circle,
// i.e. if distance between antigen and serum is less or equal than the
// circle radius.
//
// As the result for a circle we have four numbers:
// 1. Number of antigens both theoretically and empirically protected;
// 2. Number of antigens just theoretically protected;
// 3. Number of antigens just empirically protected;
// 4. Number of antigens not protected at all.
//
// Then the program optimizes the circle radius to minimize 2 and 3,
// i.e. the sum of number of antigens protected only theoretically and
// only empirically.
//
// Practically program first calculates stress for the radius equal to
// the distance of the closest antigen. Then it takes the radius as
// average between closest antigen distance and the second closest
// antigen distance and gets stress. Then it takes the radius as
// closest antigen and gets stress. And so on, the stress increases
// with each antigen included into the circle.
//
// If there are multiple optima with equal sums of 2 and 3, then the
// radius is a mean of optimal radii.

void ae::chart::v3::set_empirical(serum_circles_for_serum_t& serum_data, const Layout& layout, const Titers& titers)
{
    for (auto& antigen_data : serum_data.antigens) {
        if (!layout.point_has_coordinates(titers.number_of_antigens() + serum_data.serum_no)) {
            antigen_data.status = serum_circle_status::serum_disconnected;
        }
        else if (!layout.point_has_coordinates(antigen_data.antigen_no)) {
            antigen_data.status = serum_circle_status::antigen_disconnected;
        }
        else if (antigen_data.titer.is_dont_care()) {
            antigen_data.status = serum_circle_status::non_regular_homologous_titer;
        }
        else {
            std::vector<titer_distance_t> titers_and_distances(*titers.number_of_antigens());
            // antigen_index max_titer_for_serum_ag_no{0};
            for (const auto ag_no : titers.number_of_antigens()) {
                const auto titer = titers.titer(ag_no, serum_data.serum_no);
                if (!titer.is_dont_care()) {
                    // TODO: antigensSeraTitersMultipliers (acmacs/plot/serum_circle.py:113)
                    titers_and_distances[*ag_no] = titer_distance_t{titer, serum_data.column_basis, layout.distance(to_point_index(ag_no), titers.number_of_antigens() + serum_data.serum_no)};
                    // if (max_titer_for_serum_ag_no != ag_no && titers_and_distances[*max_titer_for_serum_ag_no].final_similarity < titers_and_distances[*ag_no].final_similarity)
                    //     max_titer_for_serum_ag_no = ag_no;
                }
                // else if (ag_no == antigen_data.antigen_no)
                //     throw serum_circle_radius_calculation_error("no homologous titer");
            }
            // const double protection_boundary_titer = titers_and_distances[antigen_data.antigen_no].final_similarity - fold;
            const double protection_boundary_titer = std::min(serum_data.column_basis, antigen_data.titer.logged_for_column_bases()) - *serum_data.fold; // fixed to support forced homologous titer
            if (protection_boundary_titer < 1.0) {
                antigen_data.status = serum_circle_status::titer_too_low;
            }
            else {
                // sort antigen indices by antigen distance from serum, closest first
                const auto antigens_by_distances_sorting = [&titers_and_distances](antigen_index a, antigen_index b) -> bool {
                    const auto& aa = titers_and_distances[*a];
                    if (aa) {
                        const auto& bb = titers_and_distances[*b];
                        return bb ? aa.distance < bb.distance : true;
                    }
                    else
                        return false;
                };
                antigen_indexes antigens_by_distances(*titers.number_of_antigens());
                std::iota(antigens_by_distances.begin(), antigens_by_distances.end(), antigen_index{0});
                // antigen_indexes antigens_by_distances(titers.number_of_antigens().begin(), titers.number_of_antigens().end());
                std::sort(antigens_by_distances.begin(), antigens_by_distances.end(), antigens_by_distances_sorting);

                constexpr const size_t None = static_cast<size_t>(-1);
                size_t best_sum = None;
                antigen_index previous{invalid_index};
                double sum_radii = 0;
                size_t num_radii = 0;
                for (const auto ag_no : antigens_by_distances) {
                    if (!titers_and_distances[*ag_no])
                        break;
                    const double radius = previous == antigen_index{invalid_index} ? titers_and_distances[*ag_no].distance : (titers_and_distances[*ag_no].distance + titers_and_distances[*previous].distance) / 2.0;
                    size_t protected_outside = 0, not_protected_inside = 0; // , protected_inside = 0, not_protected_outside = 0;
                    for (const auto& protection_data : titers_and_distances) {
                        if (protection_data) {
                            const bool inside = protection_data.distance <= radius;
                            const bool protectd =
                                protection_data.titer.is_regular() ? protection_data.final_similarity >= protection_boundary_titer : protection_data.final_similarity > protection_boundary_titer;
                            if (protectd && !inside)
                                ++protected_outside;
                            else if (!protectd && inside)
                                ++not_protected_inside;
                        }
                    }
                    const size_t summa = protected_outside + not_protected_inside;
                    if (best_sum == None || best_sum >= summa) { // if sums are the same, choose the smaller radius (found earlier)
                        if (best_sum == summa) {
                            sum_radii += radius;
                            ++num_radii;
                        }
                        else {
                            sum_radii = radius;
                            num_radii = 1;
                            best_sum = summa;
                        }
                    }
                    previous = ag_no;
                }
                antigen_data.empirical = sum_radii / static_cast<double>(num_radii);
                antigen_data.status = serum_circle_status::good;
            }
        }
    }

} // ae::chart::v3::set_empirical

// ----------------------------------------------------------------------

ae::chart::v3::serum_coverage_serum_t ae::chart::v3::serum_coverage(const Titers& titers, const Titer& homologous_titer, serum_index serum_no, serum_circle_fold fold)
{
    if (!homologous_titer.is_regular())
        throw Error(fmt::format("cannot handle non-regular homologous titer: {}", homologous_titer));
    const double titer_threshold = homologous_titer.logged() - *fold;
    if (titer_threshold <= 0)
        throw Error(fmt::format("homologous titer is too low: {}", homologous_titer));
    serum_coverage_serum_t coverage;
    for (const auto  ag_no : titers.number_of_antigens()) {
        const auto& titer = titers.titer(ag_no, serum_no);
        const double value = titer.is_dont_care() ? -1 : titer.logged_for_column_bases();
        if (value >= titer_threshold)
            coverage.within.insert(ag_no);
        else if (value >= 0 && value < titer_threshold)
            coverage.outside.insert(ag_no);
    }
    if (coverage.within.empty())
        AD_WARNING("no antigens within fold from homologous titer (for serum coverage)");
    return coverage;

} // ae::chart::v3::serum_coverage

// ----------------------------------------------------------------------
