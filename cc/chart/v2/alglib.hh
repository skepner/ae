#pragma once

#include "chart/v2/number-of-dimensions.hh"
#include "chart/v2/optimization-precision.hh"

// ----------------------------------------------------------------------

namespace ae::chart::v2
{
    struct optimization_status;
    struct OptimiserCallbackData;
}

// ----------------------------------------------------------------------

namespace ae::alglib
{
    void lbfgs_optimize(ae::chart::v2::optimization_status& status, ae::chart::v2::OptimiserCallbackData& callback_data, double* arg_first, double* arg_last, ae::chart::v2::optimization_precision precision);
    void cg_optimize(ae::chart::v2::optimization_status& status, ae::chart::v2::OptimiserCallbackData& callback_data, double* arg_first, double* arg_last, ae::chart::v2::optimization_precision precision);
    void pca(ae::chart::v2::OptimiserCallbackData& callback_data, ae::chart::v2::number_of_dimensions_t source_number_of_dimensions, ae::chart::v2::number_of_dimensions_t target_number_of_dimensions, double* arg_first, double* arg_last);
    void pca_full(ae::chart::v2::OptimiserCallbackData& callback_data, ae::chart::v2::number_of_dimensions_t number_of_dimensions, double* arg_first, double* arg_last);

} // namespace alglib

// ----------------------------------------------------------------------
