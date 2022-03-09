#include "utils/log.hh"
#include "chart/v3/projections.hh"

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
