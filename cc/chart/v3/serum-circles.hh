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

    struct serum_circle_serum_t
    {
        serum_index serum_no;
        double column_basis;
        serum_circle_fold fold;
        std::vector<serum_circle_antigen_t> antigens{};

        serum_circle_serum_t(serum_index sr_no, double cb, serum_circle_fold a_fold) : serum_no{sr_no}, column_basis{cb}, fold{a_fold} {}
        bool valid() const { return !antigens.empty(); }
        std::optional<double> theoretical() const;
        std::optional<double> empirical() const;
    };

    struct serum_circles_t
    {
        std::vector<serum_circle_serum_t> sera{};
    };

    serum_circles_t serum_circles(const Chart& chart, const Projection& projection, serum_circle_fold fold);

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
