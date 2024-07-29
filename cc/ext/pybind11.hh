#pragma once

#include "ext/fmt.hh"

// ----------------------------------------------------------------------

#pragma GCC diagnostic push

#if defined(__clang__)

// clang++ 13
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#pragma GCC diagnostic ignored "-Wdocumentation"
#pragma GCC diagnostic ignored "-Wdocumentation-unknown-command"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wshadow-field"
#pragma GCC diagnostic ignored "-Wshadow-field-in-constructor"
#pragma GCC diagnostic ignored "-Wshadow-uncaptured-local"
#pragma GCC diagnostic ignored "-Wundefined-reinterpret-cast"

// python 3.9
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wreserved-identifier"
#pragma GCC diagnostic ignored "-Wreserved-macro-identifier"

#elif defined(__GNUG__)

#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wnonnull" // subprojects/pybind11-2.9.0/include/pybind11/pybind11.h:210:21: warning: ‘this’ pointer is null

#endif

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
#include <pybind11/embed.h>
#include <pybind11/functional.h>
#include <pybind11/stl/filesystem.h>

#pragma GCC diagnostic pop

namespace py = pybind11;

// ----------------------------------------------------------------------

// template <> struct fmt::formatter<py::object> : fmt::formatter<ae::fmt_helper::default_formatter> {
//     auto format(const py::object& value, format_context& ctx) const
//     {
//         return fmt::format_to(ctx.out(), "{}", py::repr(value).cast<std::string>());
//     }
// };

template <> struct fmt::formatter<py::object> : fmt::formatter<std::string> {
    auto format(const py::object& value, format_context& ctx) const
    {
        return fmt::formatter<std::string>::format(py::repr(value).cast<std::string>(), ctx);
    }
};

// ----------------------------------------------------------------------
