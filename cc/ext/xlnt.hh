#pragma once

#pragma GCC diagnostic push

#if defined(__clang__)

#pragma GCC diagnostic ignored "-Wdocumentation-unknown-command"
#pragma GCC diagnostic ignored "-Wsuggest-destructor-override"
#pragma GCC diagnostic ignored "-Wpadded"
// #pragma GCC diagnostic ignored ""

#elif defined(__GNUG__)

#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wunknown-pragmas"

#endif

#include "xlnt/xlnt.hpp"

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------
