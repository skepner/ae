#include "utils/messages.hh"

// ----------------------------------------------------------------------

std::string ae::Messages::report() const
{
    fmt::memory_buffer out;
    if (!unrecognized_locations_.empty()) {
        fmt::format_to(std::back_inserter(out), ">> Unrecognized locations ({}):\n", unrecognized_locations_.size());
        for (const auto& loc : unrecognized_locations_)
            fmt::format_to(std::back_inserter(out), "    \"{}\"\n", loc);
    }
    if (!messages_.empty()) {
        fmt::format_to(std::back_inserter(out), ">> Messages ({}):\n", messages_.size());
        for (const auto& msg : messages_)
            fmt::format_to(std::back_inserter(out), "    [{}] {} -- {} @@ {}:{}\n", msg.type, msg.value, msg.context, msg.location.filename, msg.location.line_no);
    }
    return fmt::to_string(out);

} // ae::Messages::report

// ----------------------------------------------------------------------
