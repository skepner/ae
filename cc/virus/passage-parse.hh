#pragma once

#include <string_view>

#include "utils/messages.hh"
#include "virus/passage.hh"

// ======================================================================

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

    passage_deconstructed_t parse(std::string_view source, const parse_settings& settings, Messages& messages, const MessageLocation& location);

    inline passage_deconstructed_t parse(std::string_view source)
    {
        Messages messages;
        return parse(source, parse_settings{}, messages, MessageLocation{});
    }

} // namespace ae::virus::passage

// ======================================================================
