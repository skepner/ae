// #include <sstream>
// #include <vector>
#include <array>
#include <ctime>
#include <string_view>

#include "ext/date.hh"

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
// #pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

static const auto current_year = ae::date::today_year();

static const std::array<std::string_view, 15> month_not_first_complete{"%Y-%m-%d", "%Y%m%d", "%d/%m/%Y", "%m/%d/%Y", "%Y/%m/%d", "%B%n %d%n %Y", "%B %d,%n %Y", "%b%n %d%n %Y", "%b %d,%n %Y"};
static const std::array<std::string_view, 15> month_not_first_incomplete{"%Y-%m-%d", "%Y%m%d", "%d/%m/%Y", "%m/%d/%Y", "%Y/%m/%d", "%B%n %d%n %Y", "%B %d,%n %Y", "%b%n %d%n %Y", "%b %d,%n %Y", "%Y-00-00", "%Y-%m-00", "%Y-%m", "%Y%m", "%Y/%m", "%Y"};
static const std::array<std::string_view, 15> month_first_complete{"%Y-%m-%d", "%Y%m%d", "%m/%d/%Y", "%d/%m/%Y", "%Y/%m/%d", "%B%n %d%n %Y", "%B %d,%n %Y", "%b%n %d%n %Y", "%b %d,%n %Y"};
static const std::array<std::string_view, 15> month_first_incomplete{"%Y-%m-%d", "%Y%m%d", "%m/%d/%Y", "%d/%m/%Y", "%Y/%m/%d", "%B%n %d%n %Y", "%B %d,%n %Y", "%b%n %d%n %Y", "%b %d,%n %Y", "%Y-00-00", "%Y-%m-00", "%Y-%m", "%Y%m", "%Y/%m", "%Y"};

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

std::chrono::year_month_day ae::date::from_string(std::string_view source, std::string_view fmt, throw_on_error toe)
{
    std::tm tm{};
    if (const auto* end = strptime(source.data(), fmt.data(), &tm); !end || static_cast<size_t>(end - source.data()) < source.size()) {
        if (toe == throw_on_error::yes)
            throw parse_error(fmt::format("cannot parse date from \"{}\" (read fewer symbols than available)", source));
        else
            return invalid_date;
    }

    const auto make_year = [](int tm_year) {
        if (const auto year = 1900 + tm_year; year < (static_cast<int>(current_year) - 2000))
            return year + 2000;
        else if (year < 100)
            return year + 1900;
        else
            return year;
    };

    const auto make_day = [](int tm_mday) { return tm_mday ? tm_mday : (tm_mday + 1); };
    // fmt::print(">>>> strptime \"{}\" -> {}-{}-{}\n", source, tm.tm_year, tm.tm_mon, tm.tm_mday);

    return std::chrono::year{make_year(tm.tm_year)} / (tm.tm_mon + 1) / make_day(tm.tm_mday);

    // std::chrono::from_stream is not yet implemented in clang13

    // std::chrono::year_month_day result{invalid_date};
    // std::istringstream in(std::string{source});
    // if (std::chrono::from_stream(in, fmt, result)) {
    //     if (static_cast<size_t>(in.tellg()) < source.size()) {
    //         if (toe == throw_on_error::yes)
    //             throw parse_error(fmt::format("cannot parse date from \"{}\" (read fewer symbols than available)", source));
    //         else
    //             return invalid_date;
    //     }
    //     if (result.year() < std::chrono::year{30})
    //         result += std::chrono::years(2000);
    //     else if (result.year() < std::chrono::year{100})
    //         result += std::chrono::years(1900);
    // }
    // return result;
}

// ----------------------------------------------------------------------

std::chrono::year_month_day ae::date::from_string(std::string_view source, allow_incomplete allow, throw_on_error toe, month_first mf)
{
    if (source.empty()) {
        if (toe == throw_on_error::yes)
            throw parse_error(fmt::format("cannot parse date from \"{}\"", source));
        return invalid_date;
    }

    const auto fmt_order = [allow, mf]() {
        if (allow == allow_incomplete::yes) {
            if (mf == month_first::no)
                return month_not_first_incomplete;
            else
                return month_first_incomplete;
        }
        else {
            if (mf == month_first::no)
                return month_not_first_complete;
            else
                return month_first_complete;
        }
    };

    for (const auto fmt : fmt_order()) {
        if (fmt.empty())
            break;
        if (const auto result = from_string(source, fmt, throw_on_error::no); result.ok() /* && year_ok(result) */) {
            // fmt::print(">>> \"{}\" ({}) -> {}\n", source, fmt, result);
            return result;
        }
        // else
        //     fmt::print(">> \"{}\" ({}) -> {}\n", source, fmt, result);
    }
    if (toe == throw_on_error::yes)
        throw parse_error(fmt::format("cannot parse date from \"{}\" (allow_incomplete: {})", source, allow == allow_incomplete::yes));
    return invalid_date;
}

    // ----------------------------------------------------------------------
