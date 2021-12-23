#pragma once

#pragma GCC diagnostic push

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wreserved-identifier"
#pragma GCC diagnostic ignored "-Wpadded"
#pragma GCC diagnostic ignored "-Wmissing-variable-declarations"
#pragma GCC diagnostic ignored "-Wredundant-parens"
// #pragma GCC diagnostic ignored "-Wshadow-uncaptured-local"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wctad-maybe-unsupported"
#pragma GCC diagnostic ignored "-Wformat-nonliteral"

// https://github.com/foonathan/lexy/issues/15
#define __cpp_nontype_template_parameter_class __cpp_nontype_template_args

#endif

#ifdef __GNUG__
#pragma GCC diagnostic ignored "-Weffc++"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wparentheses"
#endif

#ifndef __cpp_consteval
#  define __cpp_consteval 0
#endif

#ifndef NDEBUG
#  define NDEBUG 0
#endif

#include <lexy/action/parse.hpp>
#include <lexy/action/trace.hpp>
#include <lexy/callback.hpp>     // value callbacks
#include <lexy/dsl.hpp>          // lexy::dsl::*
// #include <lexy/input/buffer.hpp>
#include <lexy/input/string_input.hpp>
#include <lexy_ext/report_error.hpp> // lexy_ext::report_error

#pragma GCC diagnostic pop
