#pragma once

#include "ext/pybind11.hh"

// ======================================================================

namespace ae::py
{
    void sequences(pybind11::module_& mdl);
    void tree(pybind11::module_& mdl);
    void virus(pybind11::module_& mdl);
    void whocc(pybind11::module_& mdl);
    void utils(pybind11::module_& mdl);

} // namespace acmacs_py

// ======================================================================
