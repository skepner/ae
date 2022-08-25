#pragma once

#include <chrono>
#include <string_view>

#include "ext/fmt.hh"
#include "utils/log.hh"

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
        Timeit(std::string_view msg, report_time report = report_time::yes) : message_{msg}, report_{report} {}
        template <typename Threshold> Timeit(std::string_view msg, Threshold threshold) : message_{msg}, threshold_{threshold} {}
        ~Timeit() { report(); }

        void report() const
        {
            if (report_ == report_time::yes) {
                if (const auto elap = elapsed(start_); elap >= threshold_)
                    AD_PRINT(">>> {}: {:%H:%M:%S}", message_, elap);
                report_ = report_time::no;
            }
        }

      private:
        std::string message_;
        mutable report_time report_{report_time::yes};
        timestamp_t start_{clock_t::now()};
        duration_t threshold_{0};
    };

} // namespace ae

// ======================================================================
