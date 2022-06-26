#include "utils/log.hh"
#include "chart/v3/projections.hh"
#include "chart/v3/randomizer.hh"
#include "chart/v3/stress.hh"

// ----------------------------------------------------------------------

double ae::chart::v3::Projection::stress(const Chart& chart, recalculate_stress recalculate) const
{
    switch (recalculate) {
      case recalculate_stress::yes:
          return stress_recalculate(chart);
      case recalculate_stress::if_necessary:
          if (stress_.has_value())
              return *stress_;
          else
              return stress_recalculate(chart);
      // case recalculate_stress::no:
      //     if (stress_.has_value())
      //         return *stress_;
      //     else
      //         return InvalidStress;
    }
    throw std::runtime_error{"Projection::stress: internal"};

} // ae::chart::v3::Projection::stress

// ----------------------------------------------------------------------

double ae::chart::v3::Projection::stress_recalculate(const Chart& chart) const
{
    stress_ = stress_factory(chart, *this, optimization_options{}.mult).value(layout());
    AD_DEBUG("stress_recalculate {}", *stress_);
    return *stress_;

} // ae::chart::v3::Projection::stress_recalculate

// ----------------------------------------------------------------------

void ae::chart::v3::Projection::randomize_layout(LayoutRandomizer& randomizer)
{
    for (const auto point_no : layout().number_of_points())
        layout().update(point_no, randomizer.get(layout().number_of_dimensions()));

} // ae::chart::v3::Projection::randomize_layout

// ----------------------------------------------------------------------

void ae::chart::v3::Projection::randomize_layout(const point_indexes& to_randomize, LayoutRandomizer& randomizer)
{
    for (const auto point_no : to_randomize)
        layout().update(point_no, randomizer.get(layout().number_of_dimensions()));

} // ae::chart::v3::Projection::randomize_layout

// ----------------------------------------------------------------------

ae::point_indexes ae::chart::v3::Projection::non_nan_points() const
{
    const auto& layt = layout();
    point_indexes non_nan;
    for (const auto point_no : layt.number_of_points()) {
        if (layt.point_has_coordinates(point_no))
            non_nan.insert(point_no);
    }
    return non_nan;

} // ae::chart::v3::Projection::non_nan_points

// ----------------------------------------------------------------------

ae::chart::v3::optimization_status ae::chart::v3::Projection::relax(const Chart& chart, const optimization_options& options)
{
    const auto status = optimize(chart, *this, options);
    stress_ = status.final_stress;
    if (transformation_.number_of_dimensions != layout_.number_of_dimensions())
        transformation_.reset(layout_.number_of_dimensions());
    return status;

} // ae::chart::v3::Projection::relax

// ----------------------------------------------------------------------

void ae::chart::v3::Projection::remove_points(const point_indexes& points, antigen_index number_of_antigens)
{
    const auto indexes_descending = to_vector_base_t_descending(points);
    layout_.remove_points(indexes_descending);
    stress_ = std::nullopt;
    if (!forced_column_bases_.empty()) {
        for (const auto no : indexes_descending) {
            if (no > number_of_antigens.get())
                forced_column_bases_.remove(serum_index{no - number_of_antigens.get()});
        }
    }
    remove(disconnected_, points);
    remove(unmovable_, points);
    remove(unmovable_in_the_last_dimension_, points);
    if (!avidity_adjusts_->empty()) {
        for (const auto no : indexes_descending) {
            if (no < number_of_antigens.get())
                avidity_adjusts_.get().erase(std::next(avidity_adjusts_->begin(), no));
        }
    }

} // ae::chart::v3::Projection::remove_points

// ----------------------------------------------------------------------

void ae::chart::v3::Projections::sort(const Chart& chart)
{
    // projections with NaN stress are at the end
    std::sort(data_.begin(), data_.end(), [&chart](const auto& p1, const auto& p2) {
        const auto s1 = p1.stress(chart), s2 = p2.stress(chart);
        if (std::isnan(s1))
            return false;
        else if (std::isnan(s2))
            return true;
        else
            return s1 < s2;
    });

} // ae::chart::v3::Projections::sort

// ----------------------------------------------------------------------
