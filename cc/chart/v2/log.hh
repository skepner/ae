#pragma once

#include "acmacs-virus/log.hh"

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif

namespace acmacs::log::inline v1
{
    const log_key_t relax{"relax"};
    const log_key_t report_stresses{"report-stresses"};
    const log_key_t common{"common"};
    const log_key_t distinct{"distinct"};

} // namespace acmacs::log::inline v1

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------
