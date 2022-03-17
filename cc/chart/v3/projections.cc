#include "utils/log.hh"
#include "chart/v3/projections.hh"
#include "chart/v3/randomizer.hh"

// ----------------------------------------------------------------------

double ae::chart::v3::Projection::stress(recalculate_stress recalculate) const
{
    switch (recalculate) {
      case recalculate_stress::yes:
          return stress_recalculate();
      case recalculate_stress::if_necessary:
          if (stress_.has_value())
              return *stress_;
          else
              return stress_recalculate();
      // case recalculate_stress::no:
      //     if (stress_.has_value())
      //         return *stress_;
      //     else
      //         return InvalidStress;
    }
    throw std::runtime_error{"Projection::stress: internal"};

} // ae::chart::v3::Projection::stress

// ----------------------------------------------------------------------

double ae::chart::v3::Projection::stress_recalculate() const
{
    AD_WARNING("ae::chart::v3::Projection::stress_recalculate: not imeplemented");
    return InvalidStress;

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

ae::chart::v3::optimization_status ae::chart::v3::Projection::relax(Chart& chart, const optimization_options& options)
{
    const auto status = optimize(chart, *this, options);
    stress_ = status.final_stress;
    if (transformation_.number_of_dimensions != layout_.number_of_dimensions())
        transformation_.reset(layout_.number_of_dimensions());
    return status;

} // ae::chart::v3::Projection::relax

// ----------------------------------------------------------------------
