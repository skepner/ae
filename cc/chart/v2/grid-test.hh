#pragma once

#include "chart/v2/chart-modify.hh"

// ======================================================================

namespace ae::chart::v2
{
    class GridTest
    {
      public:
        GridTest(ChartModify& chart, size_t projection_no, double grid_step)
            : chart_(chart), projection_(chart.projection_modify(projection_no)), grid_step_(grid_step), original_layout_(*projection_->layout()), stress_(chart.make_stress(projection_no))
        {
        }
        void reset(ae::chart::v2::ProjectionModifyP projection)
        {
            projection_ = projection;
            original_layout_ = *projection_->layout();
            stress_ = chart_.make_stress(projection_->projection_no());
        }

        struct Result
        {
            enum diagnosis_t { excluded, not_tested, normal, trapped, hemisphering };

            Result(size_t a_point_no, number_of_dimensions_t number_of_dimensions) : point_no(a_point_no), diagnosis(not_tested), pos(number_of_dimensions) {}
            Result(size_t a_point_no, diagnosis_t a_diagnosis, const PointCoordinates& a_pos, double a_distance, double diff)
                : point_no(a_point_no), diagnosis(a_diagnosis), pos(a_pos), distance(a_distance), contribution_diff(diff)
            {
            }
            explicit operator bool() const { return diagnosis == trapped || diagnosis == hemisphering; }
            std::string diagnosis_str(bool brief = false) const;

            std::string report(const ChartModify& chart, std::string_view pattern = "{ag_sr} {no0:{num_digits}d} {name_full_passage:<60s}") const
            {
                return fmt::format("{} {} diff:{:7.4f} dist:{:7.4f}", diagnosis_str(true), format_point(pattern, chart, point_no, collapse_spaces_t::yes), contribution_diff, distance);
            }

            size_t point_no;
            diagnosis_t diagnosis;
            PointCoordinates pos;
            double distance;
            double contribution_diff;
        };

        class Results : public std::vector<Result>
        {
          public:
            Results() = default;
            Results(const Projection& projection);
            Results(const std::vector<size_t>& points, const Projection& projection);

            std::string report() const;                                                                                                            // brief
            std::string report(const ChartModify& chart, std::string_view pattern = "{ag_sr} {no0:{num_digits}d} {name_full_passage:<60s}") const; // detailed
            std::string export_to_json(const ChartModify& chart, size_t number_of_relaxations = 0) const;
            std::string export_to_layout_csv(const ChartModify& chart, const Projection& projection) const;
            auto count_trapped_hemisphering() const
            {
                return std::count_if(begin(), end(), [](const auto& r) { return r.diagnosis == Result::trapped || r.diagnosis == Result::hemisphering; });
            }
            number_of_dimensions_t num_dimensions() const { return front().pos.number_of_dimensions(); }

          private:
            void exclude_disconnected(const Projection& projection);
            Result* find(size_t point_no)
            {
                if (const auto found = std::find_if(begin(), end(), [point_no](const auto& en) { return en.point_no == point_no; }); found != end())
                    return &*found;
                else
                    return nullptr;
            }
            const Result* find(size_t point_no) const
            {
                if (const auto found = std::find_if(begin(), end(), [point_no](const auto& en) { return en.point_no == point_no; }); found != end())
                    return &*found;
                else
                    return nullptr;
            }
        };

        std::string point_name(size_t point_no) const;
        Result test(size_t point_no);
        Results test(const std::vector<size_t>& points, int threads = 0);
        Results test_all(int threads = 0);
        ProjectionModifyP make_new_projection_and_relax(const Results& results, verbose verb);

      private:
        ChartModify& chart_;
        ProjectionModifyP projection_;
        const double grid_step_;                             // acmacs-c2: 0.01
        const double hemisphering_distance_threshold_ = 1.0; // from acmacs-c2 hemi-local test: 1.0
        const double hemisphering_stress_threshold_ = 0.25;  // stress diff within threshold -> hemisphering, from acmacs-c2 hemi-local test: 0.25
        Layout original_layout_;
        Stress stress_;
        static constexpr auto optimization_method_ = optimization_method::alglib_cg_pca;

        void test(Result& result);
        bool antigen(size_t point_no) const { return point_no < chart_.number_of_antigens(); }
        size_t antigen_serum_no(size_t point_no) const { return antigen(point_no) ? point_no : (point_no - chart_.number_of_antigens()); }
        // acmacs::Area area_for(size_t point_no) const;
        Area area_for(const Stress::TableDistancesForPoint& table_distances_for_point) const;

    }; // class GridTest::chart

    // if relax_attempts > 1, move trapped points and relax, test again, repeat while there are trapped points
    // if export_filename is not empty, exports in the json format
    // returns last grid test result and the number of grid test projections
    std::pair<GridTest::Results, size_t> grid_test(ChartModify& chart, size_t projection_no, double grid_step, int threads, size_t relax_attempts, std::string_view export_filename,
                                                   verbose verb = verbose::yes);

} // namespace ae::chart::v2

// ======================================================================
