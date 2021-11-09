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
        enum message_type {        //
            unknown,               //
            unhandled_virus_name,  //
            invalid_subtype,       //
            unrecognized_location, //
            invalid_sequence,      //
            invalid_year           //
        };

        message_type type;
        std::string value{};
        std::string context{};
        MessageLocation location;

        static inline std::string_view format_short(message_type type)
        {
            switch (type) {
                case unknown:
                    return "U";
                case unrecognized_location:
                    return "L";
                case invalid_subtype:
                    return "S";
                case invalid_year:
                    return "Y";
                case unhandled_virus_name:
                    return "-";
                case invalid_sequence:
                    return "X";
            }
            return "U";
        }

        static inline std::string_view format_long(message_type type)
        {
            switch (type) {
                case unknown:
                    return "unknown";
                case unrecognized_location:
                    return "unrecognized location";
                case invalid_subtype:
                    return "invalid subtype";
                case invalid_year:
                    return "invalid year";
                case unhandled_virus_name:
                    return "unhandled virus name";
                case invalid_sequence:
                    return "invalid sequence";
            }
            return "unknown";
        }
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

        void add(Message::message_type type, std::string_view value, std::string_view context, const MessageLocation& location = {})
        {
            messages_.push_back({type, std::string{value}, std::string{context}, location});
            switch (type) {
                case Message::unrecognized_location:
                    unrecognized_locations_.emplace(value);
                    break;
                case Message::invalid_subtype:
                case Message::invalid_year:
                case Message::unhandled_virus_name:
                case Message::invalid_sequence:
                case Message::unknown:
                    break;
            }
        }

      private:
        std::vector<Message> messages_;
        std::set<std::string> unrecognized_locations_;
    };
} // namespace ae

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::Message::message_type> : fmt::formatter<eu::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(ae::Message::message_type type, FormatCtx& ctx) { return format_to(ctx.out(), "{}", ae::Message::format_long(type)); }
};

// ----------------------------------------------------------------------
