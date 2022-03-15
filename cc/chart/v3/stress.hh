#pragma once

#include "chart/v3/optimize-options.hh"
#include "chart/v3/table-distances.hh"
#include "chart/v3/index.hh"
#include "chart/v3/avidity-adjusts.hh"
#include "chart/v3/column-bases.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Layout;
    class Chart;
    class Projection;

    struct StressParameters
    {
        StressParameters(point_index a_number_of_points, const unmovable_points& a_unmovable, const disconnected_points& a_disconnected,
                         const unmovable_in_the_last_dimension_points& a_unmovable_in_the_last_dimension, multiply_antigen_titer_until_column_adjust a_mult, const avidity_adjusts& a_avidity_adjusts,
                         dodgy_titer_is_regular_e a_dodgy_titer_is_regular)
            : number_of_points{a_number_of_points}, unmovable{a_unmovable}, disconnected{a_disconnected}, unmovable_in_the_last_dimension{a_unmovable_in_the_last_dimension}, mult{a_mult},
              m_avidity_adjusts{a_avidity_adjusts}, dodgy_titer_is_regular{a_dodgy_titer_is_regular}
        {
        }
        StressParameters(point_index a_number_of_points, multiply_antigen_titer_until_column_adjust a_mult, dodgy_titer_is_regular_e a_dodgy_titer_is_regular)
            : number_of_points(a_number_of_points), mult(a_mult), dodgy_titer_is_regular(a_dodgy_titer_is_regular)
        {
        }
        StressParameters(point_index a_number_of_points) : number_of_points{a_number_of_points} {}

        point_index number_of_points{0};
        unmovable_points unmovable{};
        disconnected_points disconnected{};
        unmovable_in_the_last_dimension_points unmovable_in_the_last_dimension{};
        multiply_antigen_titer_until_column_adjust mult{multiply_antigen_titer_until_column_adjust::yes};
        avidity_adjusts m_avidity_adjusts{};
        dodgy_titer_is_regular_e dodgy_titer_is_regular{dodgy_titer_is_regular_e::no};

    }; // struct StressParameters

    class Stress
    {
      public:
        using TableDistancesForPoint = typename TableDistances::EntriesForPoint;

        Stress(const Projection& projection, multiply_antigen_titer_until_column_adjust mult);
        Stress(number_of_dimensions_t number_of_dimensions, point_index number_of_points, multiply_antigen_titer_until_column_adjust mult, dodgy_titer_is_regular_e a_dodgy_titer_is_regular);
        Stress(number_of_dimensions_t number_of_dimensions, point_index number_of_points);

        double value(std::span<const double> args) const;
        double value(const Layout& aLayout) const;
        double contribution(point_index point_no, std::span<const double> args) const;
        double contribution(point_index point_no, const Layout& aLayout) const;
        double contribution(point_index point_no, const TableDistancesForPoint& table_distances_for_point, std::span<const double> args) const;
        double contribution(point_index point_no, const TableDistancesForPoint& table_distances_for_point, const Layout& aLayout) const;
        std::vector<double> gradient(std::span<const double> args) const;
        void gradient(std::span<const double> args, double* gradient_first) const;
        double value_gradient(std::span<const double> args, double* gradient_first) const;
        std::vector<double> gradient(const Layout& aLayout) const;
        auto number_of_dimensions() const { return number_of_dimensions_; }
        void change_number_of_dimensions(number_of_dimensions_t num_dim) { number_of_dimensions_ = num_dim; }

        const TableDistances& table_distances() const { return table_distances_; }
        TableDistances& table_distances() { return table_distances_; }
        TableDistancesForPoint table_distances_for(point_index point_no) const { return TableDistancesForPoint(point_no, table_distances_); }
        const StressParameters& parameters() const { return parameters_; }
        StressParameters& parameters() { return parameters_; }
        void set_disconnected(const disconnected_points& to_disconnect) { parameters_.disconnected = to_disconnect; }
        void extend_disconnected(const point_indexes& to_disconnect) { parameters_.disconnected.insert_if_not_present(to_disconnect); }
        size_t number_of_disconnected() const { return parameters_.disconnected.size(); }
        void set_unmovable(const unmovable_points& unmovable) { parameters_.unmovable = unmovable; }
        void set_unmovable_in_the_last_dimension(const unmovable_in_the_last_dimension_points& unmovable_in_the_last_dimension)
        {
            parameters_.unmovable_in_the_last_dimension = unmovable_in_the_last_dimension;
        }

        void set_coordinates_of_disconnected(std::span<double> args, double value, number_of_dimensions_t number_of_dimensions) const;

      private:
        number_of_dimensions_t number_of_dimensions_{0};
        TableDistances table_distances_{};
        StressParameters parameters_;

        void gradient_plain(std::span<const double> args, double* gradient_first) const;
        void gradient_with_unmovable(std::span<const double> args, double* gradient_first) const;

    }; // class Stress

    Stress stress_factory(const Chart& chart, const Projection& projection, multiply_antigen_titer_until_column_adjust mult);
    Stress stress_factory(const Chart& chart, number_of_dimensions_t number_of_dimensions, minimum_column_basis mcb, const disconnected_points& disconnected,
                          disconnect_few_numeric_titers disconnect_too_few_numeric_titers, multiply_antigen_titer_until_column_adjust mult,
                          dodgy_titer_is_regular_e a_dodgy_titer_is_regular = dodgy_titer_is_regular_e::no);

    // avidity test support
    Stress stress_factory(const Chart& chart, const Projection& projection, antigen_index antigen_no, double logged_avidity_adjust, multiply_antigen_titer_until_column_adjust mult);

    TableDistances table_distances(const Chart& chart, minimum_column_basis mcb, dodgy_titer_is_regular_e a_dodgy_titer_is_regular = dodgy_titer_is_regular_e::no);

    constexpr inline double SigmoidMutiplier() { return 10.0; }

} // namespace ae::chart::v3

// ----------------------------------------------------------------------
