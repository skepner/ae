#include <optional>

#include "utils/named-type.hh"
#include "chart/v3/chart.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    using serum_circle_fold = named_double_t<struct serum_circle_fold_tag>;
    enum class serum_circle_status { not_calculated, good, non_regular_homologous_titer, titer_too_low, serum_disconnected, antigen_disconnected };

    constexpr const double serum_circle_min_radius{2.0}; // Derek 2020-09-16 13:16 (Influenza B report and sig pages)

    struct serum_circle_antigen_t
    {
        antigen_index antigen_no;
        Titer titer;
        std::optional<double> theoretical{};
        std::optional<double> empirical{};
        serum_circle_status status{serum_circle_status::not_calculated};

        bool valid_theoretical() const { return theoretical.has_value(); }
        bool valid_empirical() const { return empirical.has_value(); }
    };

    struct serum_circles_for_serum_t
    {
        serum_index serum_no;
        double column_basis;
        serum_circle_fold fold;
        std::vector<serum_circle_antigen_t> antigens{};

        serum_circles_for_serum_t(serum_index sr_no, double cb, serum_circle_fold a_fold) : serum_no{sr_no}, column_basis{cb}, fold{a_fold} {}
        bool valid() const { return !antigens.empty(); }
        std::optional<double> theoretical() const;
        std::optional<double> empirical() const;
    };

    std::vector<serum_circles_for_serum_t> serum_circles(const Chart& chart, const Projection& projection, serum_circle_fold fold);

    struct serum_circle_for_multiple_sera_t
    {
        serum_indexes serum_no{};
        serum_circle_fold fold{2.0};
        std::optional<double> theoretical{};
        std::optional<double> empirical{};

        bool valid_theoretical() const { return theoretical.has_value(); }
        bool valid_empirical() const { return empirical.has_value(); }
    };

    serum_circle_for_multiple_sera_t serum_circle_for_multiple_sera(const Chart& chart, const Projection& projection, const serum_indexes& sera, serum_circle_fold fold);

    // ----------------------------------------------------------------------

    struct serum_coverage_serum_t
    {
        antigen_indexes within{}; // indexes of antigens which are within 4fold (fold=2.0) distance from homologous titer
        antigen_indexes outside{}; // indexes of antigens which are outside 4fold (fold=2.0) distance from homologous titer
        std::optional<antigen_index> homologous_antigen{};
    };

    serum_coverage_serum_t serum_coverage(const Titers& titers, const Titer& homologous_titer, serum_index serum_no, serum_circle_fold fold);

    inline serum_coverage_serum_t serum_coverage(const Titers& titers, antigen_index antigen_no, serum_index serum_no, serum_circle_fold fold)
    {
        auto cov = serum_coverage(titers, titers.titer(antigen_no, serum_no), serum_no, fold);
        cov.homologous_antigen = antigen_no;
        return cov;
    }

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
