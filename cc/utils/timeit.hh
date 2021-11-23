#pragma once

#include <chrono>
#include <string_view>

#include "ext/fmt.hh"

// ======================================================================

namespace ae
{
    enum class report_time { no, yes };

    using clock_t = std::chrono::high_resolution_clock;
    using timestamp_t = decltype(clock_t::now());
    // using duration_t = std::chrono::nanoseconds;
    using duration_t = std::chrono::microseconds;

    inline duration_t elapsed(timestamp_t start) { return std::chrono::duration_cast<duration_t>(clock_t::now() - start); }

    class Timeit
    {
      public:
        Timeit(std::string_view msg, report_time report = report_time::yes)
            : message_{msg}, report_{report}, start_{clock_t::now()}
        {
        }
        ~Timeit() { report(); }

        void report() const
        {
            if (report_ == report_time::yes) {
                fmt::print(">>> {}: {:%H:%M:%S}\n", message_, elapsed(start_));
                report_ = report_time::no;
            }
        }

      private:
        std::string message_;
        mutable report_time report_;
        timestamp_t start_;
    };

} // namespace ae

// ======================================================================
