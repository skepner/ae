#pragma once

// ----------------------------------------------------------------------
#ifdef _OPENMP

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wreserved-id-macro"
#endif

#include <omp.h>

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------
#else

constexpr inline int omp_get_thread_num() { return 0; }
constexpr inline int omp_get_num_threads() { return 1; }
constexpr inline int omp_get_max_threads() { return 1; }

#endif

// ----------------------------------------------------------------------
