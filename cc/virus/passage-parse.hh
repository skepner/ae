#pragma once

#include <string>
#include <string_view>

// ======================================================================

namespace ae
{
    class Messages;
    struct MessageLocation;
}

namespace ae::virus::passage
{
    class parse_settings
    {
      public:
        enum class tracing { no, yes };

        parse_settings(tracing a_trace = tracing::no) : tracing_{a_trace} {}

        constexpr bool trace() const { return tracing_ == tracing::yes; }

      private:
        tracing tracing_{tracing::no};
    };

    std::string parse(std::string_view source, parse_settings& settings, Messages& messages, const MessageLocation& location);

} // namespace ae::virus::passage

// ======================================================================
