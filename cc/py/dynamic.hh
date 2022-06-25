#pragma once

#include "ext/pybind11.hh"
#include "utils/log.hh"
#include "utils/overload.hh"
#include "utils/collection.hh"

// ======================================================================

namespace ae::py
{
    inline ae::dynamic::value to_dynamic_value(pybind11::handle handle)
    {
        if (pybind11::isinstance<pybind11::bool_>(handle))
            return ae::dynamic::value{handle.cast<bool>()};
        else if (pybind11::isinstance<pybind11::str>(handle))
            return ae::dynamic::value{handle.cast<std::string_view>()};
        else if (pybind11::isinstance<pybind11::int_>(handle))
            return ae::dynamic::value{handle.cast<long>()};
        else if (pybind11::isinstance<pybind11::float_>(handle))
            return ae::dynamic::value{handle.cast<double>()};
        else if (pybind11::isinstance<pybind11::list>(handle)) {
            ae::dynamic::array arr;
            for (auto elt : handle)
                arr.add(to_dynamic_value(elt));
            return ae::dynamic::value{std::move(arr)};
        }
        else if (pybind11::isinstance<pybind11::object>(handle)) {
            ae::dynamic::object obj;
            for (auto key : handle)
                obj[key.cast<std::string_view>()] = to_dynamic_value(handle[key]);
            return ae::dynamic::value{std::move(obj)};
        }
        else
            throw std::runtime_error{fmt::format("ae::py::to_dynamic_value: unsupported value: {}", pybind11::repr(handle).cast<std::string_view>())};
    }

    // ----------------------------------------------------------------------

    inline pybind11::object to_py_object(const ae::dynamic::value& value)
    {
        return std::visit(overload{
                              [](long num) -> pybind11::object { return pybind11::int_(num); },                                                                                   //
                              [](double num) -> pybind11::object { return pybind11::float_(num); },                                                                               //
                              [](bool flag) -> pybind11::object { return pybind11::bool_(flag); },                                                                                //
                              [](const ae::dynamic::string& str) -> pybind11::object { return pybind11::str(static_cast<std::string_view>(str)); },                               //
                              [](const ae::dynamic::null&) -> pybind11::object { return pybind11::none(); },                                                                      //
                              [](const ae::dynamic::object& /*obj*/) -> pybind11::object { throw std::runtime_error{"ae::py::to_py_object is not implmented for dynamic::object"}; }, //
                              [](const ae::dynamic::array& /*arr*/) -> pybind11::object { throw std::runtime_error{"ae::py::to_py_object is not implmented for dynamic::array"}; }    //
                          },
                          value.data());
    }

} // namespace ae::py

// ======================================================================
