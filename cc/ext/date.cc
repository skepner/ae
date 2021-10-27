// #include <sstream>
#include <vector>
#include <ctime>
#include <string_view>

#include "ext/date.hh"

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
    std::chrono::year_month_day date{floor<std::chrono::days>(std::chrono::system_clock::from_time_t(std::mktime(&tm)))};
    if (date.year() < std::chrono::year{30})
        date += std::chrono::years(2000);
    else if (date.year() < std::chrono::year{100})
        date += std::chrono::years(1900);
    return date;

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

    using fmt_order_t = std::vector<std::string_view>;
    const auto fmt_order = [mf]() -> fmt_order_t {
        const auto month_first_no = [] { return fmt_order_t{"%Y-%m-%d", "%Y%m%d", "%d/%m/%Y", "%m/%d/%Y", "%Y/%m/%d", "%B%n %d%n %Y", "%B %d,%n %Y", "%b%n %d%n %Y", "%b %d,%n %Y"}; };
        const auto month_first_yes = [] { return fmt_order_t{"%Y-%m-%d", "%Y%m%d", "%m/%d/%Y", "%d/%m/%Y", "%Y/%m/%d", "%B%n %d%n %Y", "%B %d,%n %Y", "%b%n %d%n %Y", "%b %d,%n %Y"}; };
        switch (mf) {
            case month_first::no:
                return month_first_no();
            case month_first::yes:
                return month_first_yes();
        }
        return month_first_no(); // hey g++-10
    };

    for (const auto fmt : fmt_order()) {
        // if (const auto result = from_string(std::forward<S>(source), fmt); result.ok())
        if (const auto result = from_string(source, fmt, toe); result.ok() /* && year_ok(result) */)
            return result;
    }
    if (allow == allow_incomplete::yes) {
        for (const char* fmt : {"%Y-00-00", "%Y-%m-00", "%Y-%m", "%Y%m", "%Y"}) {
            // date lib cannot parse incomplete date
            constexpr int invalid = 99999;
            struct tm tm;
            tm.tm_mon = tm.tm_mday = invalid;
            if (strptime(source.data(), fmt, &tm) == &*source.end()) {
                if (tm.tm_mon == invalid)
                    return std::chrono::year{tm.tm_year + 1900} / 0 / 0;
                else
                    return std::chrono::year{tm.tm_year + 1900} / std::chrono::month{static_cast<unsigned>(tm.tm_mon) + 1} / 0;
            }
        }
    }
    if (toe == throw_on_error::yes)
        throw parse_error(fmt::format("cannot parse date from \"{}\" (allow_incomplete: {})", source, allow == allow_incomplete::yes));
    return invalid_date;
}

// ----------------------------------------------------------------------
