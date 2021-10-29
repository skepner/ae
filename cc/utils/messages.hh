#pragma once

#include <set>
#include <vector>
#include <string>

#include "ext/fmt.hh"

// ----------------------------------------------------------------------

namespace ae
{
    struct MessageLocation
    {
        std::string filename{};
        size_t line_no{0};
    };

    struct Message
    {
        enum message_type { unknown, unhandled_virus_name, invalid_subtype, unrecognized_location, invalid_year };

        message_type type;
        std::string value{};
        std::string context{};
        MessageLocation location;
    };

    class Messages
    {
      public:
        Messages() = default;
        Messages(const Messages&) = default;
        Messages(Messages&&) = default;

        std::string report() const;
        bool empty() const { return messages_.empty(); }
        constexpr const auto& messages() const { return messages_; }
        const auto& unrecognized_locations() const { return unrecognized_locations_; }

        void add(Message::message_type type, std::string_view value, std::string_view context, const MessageLocation& location)
            {
                messages_.push_back({type, std::string{value}, std::string{context},  location});
                switch (type) {
                    case Message::unrecognized_location:
                        unrecognized_locations_.emplace(value);
                        break;
                    case Message::invalid_subtype:
                    case Message::invalid_year:
                    case Message::unhandled_virus_name:
                    case Message::unknown:
                        break;
                }
            }

      private:
        std::vector<Message> messages_;
        std::set<std::string> unrecognized_locations_;
    };
}

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::Message::message_type> : fmt::formatter<eu::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(ae::Message::message_type type, FormatCtx& ctx)
    {
        using namespace ae;

        switch (type) {
            case Message::unknown:
                format_to(ctx.out(), "unknown");
                break;
            case Message::unrecognized_location:
                format_to(ctx.out(), "unrecognized location");
                break;
            case Message::invalid_subtype:
                format_to(ctx.out(), "invalid subtype");
                break;
            case Message::invalid_year:
                format_to(ctx.out(), "invalid year");
                break;
            case Message::unhandled_virus_name:
                format_to(ctx.out(), "unhandled virus name");
                break;
        }
        return ctx.out();
    }
};

// ----------------------------------------------------------------------
