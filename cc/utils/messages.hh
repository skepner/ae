#pragma once

#include <set>
#include <vector>
#include <string>

#include "ext/fmt.hh"

// ----------------------------------------------------------------------

namespace ae
{
    struct Message
    {
        Message() = default;
        template <typename A1, typename A2> Message(A1 a1, A2 a2) : message{a1}, context{a2} {}

        std::string message{};
        std::string context{};
    };

    class Messages
    {
      public:
        Messages() = default;

        std::string report() const;
        bool empty() const { return messages_.empty(); }

        void message(std::string_view msg, std::string_view context)
        {
            messages_.emplace_back(msg, context);
        }

        void unrecognized_location(std::string_view location, std::string_view name)
        {
            unrecognized_locations_.emplace(location);
            message(fmt::format("unrecognized location: \"{}\"", location), name);
        }

      private:
        std::vector<Message> messages_;
        std::set<std::string> unrecognized_locations_;
    };
}

// ----------------------------------------------------------------------
