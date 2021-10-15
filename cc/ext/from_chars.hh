#pragma once

#include <charconv>
#include <tuple>
#include <string_view>

#include "ext/fmt.hh"

// ----------------------------------------------------------------------

namespace ae
{
    template <typename Result, typename Source> std::tuple<Result, const char*, std::errc> from_chars_detailed(Source&& source)
    {
        Result result;
        const auto [end, ec] = std::from_chars(&*std::begin(source), &*std::end(source), result);
        return {result, end, ec};
    }

    template <typename Result, typename Source> Result from_chars(Source&& source)
    {
        Result result;
        const auto [end, ec] = std::from_chars(&*std::begin(source), &*std::end(source), result);
        if (ec != std::errc())
            throw std::invalid_argument{std::make_error_condition(ec).message()};
        if (end != &*std::end(source))
            throw std::invalid_argument{fmt::format("unrecognized tail: \"{}\"", std::string_view(end, &*std::end(source) - end))};
        return result;
    }

} // namespace ae

// ----------------------------------------------------------------------
