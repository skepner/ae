#pragma once

#include <concepts>

#include "ext/fmt.hh"

// ----------------------------------------------------------------------

namespace ae
{
    template <typename T, typename Tag> class named_t
    {
      public:
        using value_type = T;

        explicit constexpr named_t() = default;
        template <typename T2> requires std::constructible_from<T, T2> explicit constexpr named_t(T2&& value) : value_(std::forward<T2>(value)) {}

        template <typename T2> requires std::assignable_from<T&, T2> constexpr named_t& operator=(const T2& value) { value_ = value; return *this; }
        template <typename T2> requires std::assignable_from<T&, T2> constexpr named_t& operator=(T2&& value) { value_ = std::move(value); return *this; }

        constexpr T& get() noexcept { return value_; }
        constexpr const T& get() const noexcept { return value_; }
        constexpr T& operator*() noexcept { return value_; }
        constexpr const T& operator*() const noexcept { return value_; }
        constexpr const T* operator->() const noexcept { return &value_; }
        explicit constexpr operator T&() noexcept { return value_; }
        explicit constexpr operator const T&() const noexcept { return value_; }

      protected:
        T value_;
    };

    // ----------------------------------------------------------------------

    template <typename T, typename Tag> class named_string_t : public named_t<T, Tag>
    {
      public:
        using named_t<T, Tag>::named_t;
        using named_t<T, Tag>::operator=;

        constexpr bool empty() const { return this->get().empty(); }
        constexpr size_t size() const { return this->get().size(); }
    };
}

// ----------------------------------------------------------------------

// template <typename T, typename Tag> struct fmt::formatter<ae::named_t<T, Tag>> : fmt::formatter<T> {
//     template <typename FormatCtx> auto format(const ae::named_t<T, Tag>& nt, FormatCtx& ctx) const { return fmt::formatter<T>::format(nt.get(), ctx); }
// };

// template <typename T, typename Tag> struct fmt::formatter<ae::named_string_t<T, Tag>> : fmt::formatter<std::string> {
//     template <typename FormatCtx> auto format(const ae::named_string_t<T, Tag>& nt, FormatCtx& ctx) const { return fmt::formatter<std::string>::format(nt.get(), ctx); }
// };

// ----------------------------------------------------------------------
