#pragma once

#include <concepts>
#include <charconv>
#include <tuple>
#include <string_view>

#include "ext/fmt.hh"

// ----------------------------------------------------------------------

namespace ae
{
    template <typename Result, typename Source> std::tuple<Result, const char*, std::errc> from_chars_detailed(Source&& source, int base = 10)
    {
        Result result;
        const auto [end, ec] = std::from_chars(&*std::begin(source), &*std::end(source), result, base);
        return {result, end, ec};
    }

    template <std::integral Result, typename Source> Result from_chars(Source&& source, int base = 10)
    {
        Result result;
        const auto [end, ec] = std::from_chars(&*std::begin(source), &*std::end(source), result, base);
        if (ec != std::errc())
            throw std::invalid_argument{std::make_error_condition(ec).message()};
        if (end != &*std::end(source))
            throw std::invalid_argument{fmt::format("unrecognized tail: \"{}\"", std::string_view(end, static_cast<size_t>(&*std::end(source) - end)))};
        return result;
    }

    template <std::integral Result, typename Source> Result from_chars(Source&& source, size_t& processed)
    {
        Result result;
        const auto [end, ec] = std::from_chars(&*std::begin(source), &*std::end(source), result);
        if (ec != std::errc())
            throw std::invalid_argument{std::make_error_condition(ec).message()};
        if (end != &*std::end(source))
            throw std::invalid_argument{fmt::format("unrecognized tail: \"{}\"", std::string_view(end, static_cast<size_t>(&*std::end(source) - end)))};
        processed = static_cast<size_t>(end - &*source.begin());
        return result;
    }

    template <std::floating_point Result, typename Source> Result from_chars(Source&& source)
    {
        if constexpr (std::is_same_v<std::decay_t<Source>, std::string>)
            return std::stod(source);
        else
            return std::stod(std::string{source});
    }

    template <std::floating_point Result, typename Source> Result from_chars(Source&& source, size_t& processed)
    {
        if constexpr (std::is_same_v<std::decay_t<Source>, std::string>)
            return std::stod(source, &processed);
        else
            return std::stod(std::string{source}, &processed);
    }

    template <typename Result, typename Iter> Result from_chars(Iter begin, Iter end)
    {
        return from_chars<Result>(std::string{begin, end});
    }

} // namespace ae

// ----------------------------------------------------------------------
