#pragma once

#include "chart/v3/index.hh"
#include "chart/v3/optimization-precision.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v3
{
    struct optimization_status;
    struct OptimiserCallbackData;
}

// ----------------------------------------------------------------------

namespace ae::alglib
{
    void lbfgs_optimize(ae::chart::v3::optimization_status& status, ae::chart::v3::OptimiserCallbackData& callback_data, std::span<double> args, ae::chart::v3::optimization_precision precision);
    void cg_optimize(ae::chart::v3::optimization_status& status, ae::chart::v3::OptimiserCallbackData& callback_data, std::span<double> args, ae::chart::v3::optimization_precision precision);
    void pca(ae::chart::v3::OptimiserCallbackData& callback_data, ae::number_of_dimensions_t source_number_of_dimensions, ae::number_of_dimensions_t target_number_of_dimensions, std::span<double> args);
    void pca_full(ae::chart::v3::OptimiserCallbackData& callback_data, ae::number_of_dimensions_t number_of_dimensions, std::span<double> args);

} // namespace alglib

// ----------------------------------------------------------------------
