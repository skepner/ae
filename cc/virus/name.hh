#pragma once

#include "ext/fmt.hh"
#include "ext/string.hh"
#include "utils/string.hh"

// ----------------------------------------------------------------------

namespace ae::virus::inline v2
{
    class Name
    {
      public:
        Name() = default;
        Name(const Name&) = default;
        Name(Name&&) = default;
        explicit Name(std::string_view src) : value_{string::uppercase(src)} {}
        Name& operator=(const Name&) = default;
        Name& operator=(Name&&) = default;

        operator std::string_view() const { return value_; }
        std::string_view operator*() const { return value_; }

        bool operator==(const Name& rhs) const = default;
        auto operator<=>(const Name& rhs) const = default;

        bool empty() const { return value_.empty(); }
        size_t size() const { return value_.size(); }
        auto operator[](size_t pos) const { return value_[pos]; }
        template <typename S> auto find(S look_for) const { return value_.find(look_for); }
        auto substr(size_t pos, size_t len) const { return value_.substr(pos, len); }

      private:
        std::string value_;

    };

} // namespace acmacs::virus::inline v2

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::virus::Name> : public fmt::formatter<std::string_view>
{
    template <typename FormatContext> auto format(const ae::virus::Name& ts, FormatContext& ctx) { return fmt::format(static_cast<std::string_view>(ts), ctx); }
};

// ----------------------------------------------------------------------
