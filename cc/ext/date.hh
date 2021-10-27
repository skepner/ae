#pragma once

#include <chrono>
#include <stdexcept>

#include "ext/fmt.hh"

// ----------------------------------------------------------------------

namespace ae::date
{
    constexpr auto invalid_date{std::chrono::year{0} / 0 / 0};
    // std::chrono::year_month_day invalid_date() { return std::chrono::year{0} / 0 / 0; }

    class parse_error : public std::runtime_error
    {
      public:
        using std::runtime_error::runtime_error;
    };

    // using years = std::chrono::duration<int, detail::ratio_multiply<std::ratio<146097, 400>, std::chrono::days::period>>;

    // ----------------------------------------------------------------------

    inline auto today() { return floor<std::chrono::days>(std::chrono::system_clock::now()); }
    inline auto today_year() { return static_cast<size_t>(static_cast<int>(std::chrono::year_month_day{today()}.year())); }

   // ----------------------------------------------------------------------

    enum class throw_on_error { no, yes };
    enum class allow_incomplete { no, yes };
    enum class month_first { no, yes }; // European vs. US  if parts are separated by slashes

    std::chrono::year_month_day from_string(std::string_view source, std::string_view fmt, throw_on_error toe = throw_on_error::yes);
    std::chrono::year_month_day from_string(std::string_view source, allow_incomplete allow = allow_incomplete::no, throw_on_error toe = throw_on_error::yes, month_first mf = month_first::no);

    inline std::string parse_and_format(std::string_view source, allow_incomplete allow = allow_incomplete::no, throw_on_error toe = throw_on_error::yes, month_first mf = month_first::no)
    {
        const auto date = from_string(source, allow, toe, mf);
        return fmt::format("{:04d}-{:02d}-{:02d}", static_cast<int>(date.year()), static_cast<unsigned>(date.month()), static_cast<unsigned>(date.day()));
    }
}

// ----------------------------------------------------------------------
