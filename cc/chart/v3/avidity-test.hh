#include <array>

#include "chart/v3/index.hh"
#include "chart/v3/point-coordinates.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Chart;
    class Projection;
    class Layout;

    namespace avidity_test
    {
        struct settings_t
        {
            double adjust_step{1.0};
            double min_adjust{-6.0};
            double max_adjust{6.0};
            bool rough{false};
            int threads{0};
        };

        // ----------------------------------------------------------------------

        constexpr const size_t number_of_most_moved_antigens{5};

        struct most_moved_t
        {
            antigen_index antigen_no{invalid_index};
            double distance{-1.0};
        };


        struct per_adjust_t
        {
            double logged_adjust;
            double distance_test_antigen;
            double angle_test_antigen;
            double average_procrustes_distances_except_test_antigen;
            point_coordinates final_coordinates;
            double stress_diff;
            std::array<most_moved_t, number_of_most_moved_antigens> most_moved{};
        };

        struct result_t
        {
            const per_adjust_t* best_adjust() const;
            void post_process();

            antigen_index antigen_no;
            double best_logged_adjust;
            point_coordinates_ref_const original;
            std::vector<per_adjust_t> adjusts;
        };

        class results_t
        {
          public:
            results_t(double original_stress) : original_stress_{original_stress} {}

            const result_t& get(antigen_index antigen_no) const
            {
                if (const auto found = std::find_if(std::begin(data_), std::end(data_), [antigen_no](const auto& en) { return en.antigen_no == antigen_no; }); found != std::end(data_))
                    return *found;
                else
                    throw std::runtime_error{AD_FORMAT("avidity: no result entry for AG {} (internal error)", antigen_no)};
            }

            const auto& results() const { return data_; }

          private:
            std::vector<result_t> data_;
            double original_stress_;

            void post_process();

            friend results_t test(const Chart&, const Projection&, const settings_t&);
        };

        // ----------------------------------------------------------------------

        results_t test(const Chart& chart, const Projection& projection, const settings_t& settings = {});

    } // namespace avidity_test
} // namespace ae::chart::v3

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v3::avidity_test::per_adjust_t> : fmt::formatter<ae::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const ae::chart::v3::avidity_test::per_adjust_t& per_adjust, FormatCtx& ctx) const
    {
        return format_to(ctx.out(), "{:4.1f}  diff:{:8.4f} dist:{:7.4f} angle:{:7.4f} aver_pc_dist:{:7.4f}", per_adjust.logged_adjust, per_adjust.stress_diff, per_adjust.distance_test_antigen,
                         per_adjust.angle_test_antigen, per_adjust.average_procrustes_distances_except_test_antigen);
    }
};

template <> struct fmt::formatter<ae::chart::v3::avidity_test::result_t> : fmt::formatter<ae::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const ae::chart::v3::avidity_test::result_t& result, FormatCtx& ctx) const
    {
        format_to(ctx.out(), "AG {}\n", result.antigen_no);
        if (!float_zero(result.best_logged_adjust))
            format_to(ctx.out(), "    {}\n", result.best_adjust());
        else
            format_to(ctx.out(), "    no adjust\n");
        for (const auto& en : result.adjusts)
            format_to(ctx.out(), "        {}\n", en);
        return ctx.out();
    }
};

template <> struct fmt::formatter<ae::chart::v3::avidity_test::results_t> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::chart::v3::avidity_test::results_t& results, FormatCtx& ctx) const
    {
        for (const auto& result : results.results())
            format_to(ctx.out(), "{}\n", result);
        return ctx.out();
    }
};

// ----------------------------------------------------------------------
