#include "chart/v3/index.hh"
#include "chart/v3/point-coordinates.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    class Chart;
    class Projection;

    namespace grid_test
    {
        struct settings_t
        {
            double step{0.1};
            size_t threads{1};
        };

        struct result_t
        {
            enum diagnosis_t { excluded, not_tested, normal, trapped, hemisphering };

            result_t(point_index a_point_no, number_of_dimensions_t number_of_dimensions) : point_no(a_point_no), diagnosis(not_tested), pos(number_of_dimensions) {}

            point_index point_no;
            diagnosis_t diagnosis{not_tested};
            point_coordinates pos;
            double distance{0.0};
            double contribution_diff{0.0};
        };

        class results_t
        {
          public:
            results_t(const Projection& projection);

            size_t size() const { return data_.size(); }
            result_t& operator[](size_t index) { return data_[index]; }

          private:
            std::vector<result_t> data_;

            void exclude_disconnected(const Projection& projection);
            result_t* find(point_index point_no);

        };

        results_t test(const Chart& chart, projection_index projection_no, const settings_t& settings = {});

    } // namespace grid_test
} // namespace ae::chart::v3

// ----------------------------------------------------------------------
