#pragma once

#include <vector>
#include <optional>

#include "chart/v3/layout.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Chart;

    enum class RecalculateStress { no, if_necessary, yes };
    constexpr const double InvalidStress{-1.0};

    class Projection
    {
      public:
        Projection() = default;
        Projection(const Projection&) = default;
        Projection(Projection&&) = default;
        Projection& operator=(const Projection&) = default;
        Projection& operator=(Projection&&) = default;

        // std::string make_info() const;

        // std::optional<double> stored_stress() const = 0;
        double stress(RecalculateStress recalculate = RecalculateStress::if_necessary) const;
        // double stress_with_moved_point(size_t point_no, const PointCoordinates& move_to) const;
        // std::string comment() const = 0;
        // number_of_dimensions_t number_of_dimensions() const = 0;
        // size_t number_of_points() const = 0;
        // std::shared_ptr<Layout> layout() const = 0;
        // std::shared_ptr<Layout> transformed_layout() const { return layout()->transform(transformation()); }
        // MinimumColumnBasis minimum_column_basis() const = 0;
        // std::shared_ptr<ColumnBases> forced_column_bases() const = 0; // returns nullptr if not forced
        // draw::v1::Transformation transformation() const = 0;
        // enum dodgy_titer_is_regular dodgy_titer_is_regular() const = 0;
        // double stress_diff_to_stop() const = 0;
        // UnmovablePoints unmovable() const = 0;
        // DisconnectedPoints disconnected() const = 0;
        // UnmovableInTheLastDimensionPoints unmovable_in_the_last_dimension() const = 0;
        // AvidityAdjusts avidity_adjusts() const = 0; // antigens_sera_titers_multipliers, double for each point
        //                                             // antigens_sera_gradient_multipliers, double for each point

        // double calculate_stress(const Stress& stress) const { return stress.value(*layout()); }
        // std::vector<double> calculate_gradient(const Stress& stress) const { return stress.gradient(*layout()); }

        // double calculate_stress(multiply_antigen_titer_until_column_adjust mult = multiply_antigen_titer_until_column_adjust::yes) const;
        // std::vector<double> calculate_gradient(multiply_antigen_titer_until_column_adjust mult = multiply_antigen_titer_until_column_adjust::yes) const;

        // Blobs blobs(double stress_diff, size_t number_of_drections = 36, double stress_diff_precision = 1e-5) const;
        // Blobs blobs(double stress_diff, const PointIndexList& points, size_t number_of_drections = 36, double stress_diff_precision = 1e-5) const;

        // void set_projection_no(size_t projection_no) { projection_no_ = projection_no; }

        // ErrorLines error_lines() const { return ae::chart::v2::error_lines(*this); }

      protected:
        // double recalculate_stress() const { return calculate_stress(); }

      private:
        Layout layout_{};
        // ae::draw::v1::Transformation transformation_{};
        // mutable std::optional<Layout> transformed_layout_{};
        // mutable std::optional<double> stress_{};
        // ColumnBases forced_column_bases_{};
        // std::string comment_{};
        // DisconnectedPoints disconnected_{};
        // UnmovablePoints unmovable_{};
        // UnmovableInTheLastDimensionPoints unmovable_in_the_last_dimension_{};
    };

    // ----------------------------------------------------------------------

    class Projections
    {
      public:
        Projections() = default;
        Projections(const Projections&) = default;
        Projections(Projections&&) = default;
        Projections& operator=(const Projections&) = default;
        Projections& operator=(Projections&&) = default;

        bool empty() const { return data_.empty(); }
        size_t size() const { return data_.size(); }
        const Projection& operator[](projection_index aIndex) const { return data_[*aIndex]; }
        Projection& operator[](projection_index aIndex) { return data_[*aIndex]; }
        const Projection& best() const { return operator[](projection_index{0}); }
        auto begin() const { return data_.begin(); }
        auto end() const { return data_.end(); }
        Projection& add() { return data_.emplace_back(); }
        // std::string make_info(size_t max_number_of_projections_to_show = 20) const;

        void sort()
        {
            std::sort(data_.begin(), data_.end(), [](const auto& p1, const auto& p2) { return p1.stress() < p2.stress(); });
            // set_projection_no();
        }

        void keep_just(projection_index number_of_projections_to_keep)
        {
            if (data_.size() > *number_of_projections_to_keep)
                data_.erase(data_.begin() + static_cast<decltype(data_)::difference_type>(*number_of_projections_to_keep), data_.end());
        }

        void remove_all() { data_.clear(); }
        void remove(projection_index projection_no) { data_.erase(data_.begin() + *projection_no); }
        // void remove_all_except(size_t projection_no);
        // void remove_except(size_t number_of_initial_projections_to_keep, ProjectionP projection_to_keep = {nullptr});

        // void remove_antigens(const ReverseSortedIndexes& indexes)
        // {
        //     for_each(data_.begin(), data_.end(), [&](auto& projection) { projection->remove_antigens(indexes); });
        // }
        // void remove_sera(const ReverseSortedIndexes& indexes, size_t number_of_antigens)
        // {
        //     for_each(data_.begin(), data_.end(), [&indexes, number_of_antigens](auto& projection) { projection->remove_sera(indexes, number_of_antigens); });
        // }
        // void insert_antigen(size_t before)
        // {
        //     for_each(data_.begin(), data_.end(), [=](auto& projection) { projection->insert_antigen(before); });
        // }
        // void insert_serum(size_t before, size_t number_of_antigens)
        // {
        //     for_each(data_.begin(), data_.end(), [=](auto& projection) { projection->insert_serum(before, number_of_antigens); });
        // }

      private:
        std::vector<Projection> data_;
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v3::Projection> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::chart::v3::Projection& projection, FormatCtx& ctx) const
        {
            format_to(ctx.out(), "PROJECTION");
            return ctx.out();
        }
};

// ----------------------------------------------------------------------
