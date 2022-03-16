#pragma once

#include <optional>

#include "chart/v3/layout.hh"
#include "chart/v3/transformation.hh"
#include "chart/v3/column-bases.hh"
#include "chart/v3/avidity-adjusts.hh"
#include "chart/v3/optimize-options.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Chart;
    class LayoutRandomizer;

    enum class recalculate_stress { /* no, */ if_necessary, yes };

    constexpr const double InvalidStress{-1.0};

    class Projection
    {
      public:
        Projection() = default;
        Projection(const Projection&) = default;
        Projection(Projection&&) = default;
        Projection(point_index num_points, number_of_dimensions_t num_dim, minimum_column_basis mcb) : layout_{num_points, num_dim}, minimum_column_basis_{mcb} {}
        Projection& operator=(const Projection&) = default;
        Projection& operator=(Projection&&) = default;

        // std::string make_info() const;

        point_index number_of_points() const noexcept { return layout_.number_of_points(); }
        number_of_dimensions_t number_of_dimensions() const noexcept { return layout_.number_of_dimensions(); }

        const Layout& layout() const { return layout_; }
        Layout& layout() { return layout_; }

        // std::optional<double> stored_stress() const { return stress_; }
        void stress(double str) { stress_ = str; }
        // void no_stress() { stress_ = std::nullopt; }
        double stress(recalculate_stress recalculate = recalculate_stress::if_necessary) const;
        // double stress_with_moved_point(size_t point_no, const PointCoordinates& move_to) const;
        Transformation& transformation() { return transformation_; }
        const Transformation& transformation() const { return transformation_; }
        std::string_view comment() const { return comment_; }
        void comment(std::string_view comm) { comment_ = comm; }

        class minimum_column_basis minimum_column_basis() const { return minimum_column_basis_; }
        void minimum_column_basis(std::string_view mcb) { minimum_column_basis_ = mcb; }
        column_bases& forced_column_bases() { return forced_column_bases_; }
        const column_bases& forced_column_bases() const { return forced_column_bases_; }

        avidity_adjusts& avidity_adjusts_access() { return avidity_adjusts_; }
        const avidity_adjusts& avidity_adjusts_access() const { return avidity_adjusts_; }
        unmovable_points& unmovable() { return unmovable_; }
        const unmovable_points& unmovable() const { return unmovable_; }
        disconnected_points& disconnected() { return disconnected_; }
        const disconnected_points& disconnected() const { return disconnected_; }
        unmovable_in_the_last_dimension_points& unmovable_in_the_last_dimension() { return unmovable_in_the_last_dimension_; }
        const unmovable_in_the_last_dimension_points& unmovable_in_the_last_dimension() const { return unmovable_in_the_last_dimension_; }
        dodgy_titer_is_regular_e dodgy_titer_is_regular() const { return dodgy_titer_is_regular_; }
        void dodgy_titer_is_regular(dodgy_titer_is_regular_e dtir) { dodgy_titer_is_regular_ = dtir; }

        void randomize_layout(LayoutRandomizer& randomizer);
        void randomize_layout(const point_indexes& to_randomize, LayoutRandomizer& randomizer); // randomize just some point coordinates

        void transformation_reset()
        {
            transformation_.reset(number_of_dimensions());
            // transformed_layout_.reset();
        }

        point_indexes non_nan_points() const; // for relax_incremental and enum unmovable_non_nan_points

        // std::shared_ptr<Layout> transformed_layout() const { return layout()->transform(transformation()); }
        // double stress_diff_to_stop() const = 0;

        // double calculate_stress(const Stress& stress) const { return stress.value(*layout()); }
        // std::vector<double> calculate_gradient(const Stress& stress) const { return stress.gradient(*layout()); }

        // double calculate_stress(multiply_antigen_titer_until_column_adjust mult = multiply_antigen_titer_until_column_adjust::yes) const;
        // std::vector<double> calculate_gradient(multiply_antigen_titer_until_column_adjust mult = multiply_antigen_titer_until_column_adjust::yes) const;

        // Blobs blobs(double stress_diff, size_t number_of_drections = 36, double stress_diff_precision = 1e-5) const;
        // Blobs blobs(double stress_diff, const PointIndexList& points, size_t number_of_drections = 36, double stress_diff_precision = 1e-5) const;

        // void set_projection_no(size_t projection_no) { projection_no_ = projection_no; }

        // ErrorLines error_lines() const { return ae::chart::v2::error_lines(*this); }

      private:
        Layout layout_{};
        Transformation transformation_{};
        // mutable std::optional<Layout> transformed_layout_{};
        mutable std::optional<double> stress_{};
        column_bases forced_column_bases_{};
        class minimum_column_basis minimum_column_basis_{};
        std::string comment_{};
        disconnected_points disconnected_{};
        unmovable_points unmovable_{};
        unmovable_in_the_last_dimension_points unmovable_in_the_last_dimension_{};
        avidity_adjusts avidity_adjusts_{};
        dodgy_titer_is_regular_e dodgy_titer_is_regular_{dodgy_titer_is_regular_e::no};

        double stress_recalculate() const;
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
        projection_index size() const { return projection_index{data_.size()}; }
        const Projection& operator[](projection_index aIndex) const { return data_[*aIndex]; }
        Projection& operator[](projection_index aIndex) { return data_[*aIndex]; }
        const Projection& best() const { return operator[](projection_index{0}); }
        auto begin() const { return data_.begin(); }
        auto end() const { return data_.end(); }
        template <typename ... Args> Projection& add(Args&& ... args) { return data_.emplace_back(std::forward<Args>(args) ...); }
        // std::string make_info(size_t max_number_of_projections_to_show = 20) const;

        void sort()
        {
            std::sort(data_.begin(), data_.end(), [](const auto& p1, const auto& p2) { return p1.stress() < p2.stress(); });
            // set_projection_no();
        }

        void keep(projection_index number_of_projections_to_keep)
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
        std::vector<Projection> data_{};
    };

} // namespace ae::chart::v3

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::chart::v3::Projection> : fmt::formatter<ae::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::chart::v3::Projection& projection, FormatCtx& ctx) const
        {
            return format_to(ctx.out(), "{:.4f}", projection.stress());
        }
};

// ----------------------------------------------------------------------
