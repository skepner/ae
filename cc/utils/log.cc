#include <map>

#include "ext/range-v3.hh"
#include "utils/log.hh"
#include "utils/string.hh"

size_t ae::log::v1::detail::indent{0};
bool ae::log::v1::detail::print_debug_messages{true}; // to disable by acmacs.r

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif

namespace ae::log::inline v1
{
    detail::enabled_t detail::enabled{};

    const log_key_t all{"all"};
    const log_key_t timer{"timer"};
    const log_key_t settings{"settings"};
    const log_key_t vaccines{"vaccines"};
    const log_key_t name_parsing{"name"};
    const log_key_t passage_parsing{"passage"};

} // namespace ae::log::v1

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------

void ae::log::v1::enable(std::string_view names)
{
    using namespace std::string_view_literals;

    for (const auto& name : ae::string::split(names, ","sv, ae::string::split_emtpy::remove)) {
        if (name == all) {
            if (detail::enabled.empty() || detail::enabled.front() != name)
                detail::enabled.insert(detail::enabled.begin(), log_key_t{name});
        }
        else if (std::find(std::begin(detail::enabled), std::end(detail::enabled), name) == std::end(detail::enabled))
            detail::enabled.push_back(log_key_t{name});
    }

} // ae::log::v1::enable

// ----------------------------------------------------------------------

void ae::log::v1::enable(const std::vector<std::string_view>& names)
{
    for (const auto& name : names)
        enable(name);

} // ae::log::v1::enable

// ----------------------------------------------------------------------

std::string ae::log::v1::report_enabled()
{
    fmt::memory_buffer out;
    fmt::format_to(std::back_inserter(out), "log messages enabled for ({}):", detail::enabled.size());
    for (const auto& key : detail::enabled)
        fmt::format_to(std::back_inserter(out), " \"{}\"", static_cast<std::string_view>(key));
    return fmt::to_string(out);

} // ae::log::v1::report_enabled

// ----------------------------------------------------------------------
