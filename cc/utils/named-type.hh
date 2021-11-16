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
        constexpr named_t(const named_t&) = default;
        constexpr named_t(named_t&&) = default;
        template <typename T2> requires std::constructible_from<T, T2> explicit constexpr named_t(T2&& value) : value_(std::forward<T2>(value)) {}

        named_t& operator=(const named_t&) = default;
        named_t& operator=(named_t&&) = default;
        template <typename T2> requires std::assignable_from<T&, T2> constexpr named_t& operator=(const T2& value) { value_ = value; return *this; }
        template <typename T2> requires std::assignable_from<T&, T2> constexpr named_t& operator=(T2&& value) { value_ = std::move(value); return *this; }

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

        auto operator<=>(const named_t&) const = default;
        bool operator==(const named_t&) const = default;

#pragma GCC diagnostic pop

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

        operator std::string_view() const { return this->get(); }
    };

    // ----------------------------------------------------------------------

    template <std::integral T, typename Tag> class named_number_t : public named_t<T, Tag>
    {
      public:
        using named_t<T, Tag>::named_t;
        using named_t<T, Tag>::operator=;

        constexpr auto& operator++()
        {
            ++this->get();
            return *this;
        }

        constexpr auto operator++(int)
        {
            const auto saved{*this};
            ++this->get();
            return saved;
        }

        constexpr auto& operator--()
        {
            --this->get();
            return *this;
        }

        constexpr auto operator--(int)
        {
            const auto saved{*this};
            --this->get();
            return saved;
        }
    };

    template <std::integral Number, typename Tag> constexpr named_number_t<Number, Tag> operator-(named_number_t<Number, Tag> rhs) noexcept { return named_number_t<Number, Tag>{-rhs.get()}; }
    template <std::integral Number, typename Tag> constexpr named_number_t<Number, Tag> operator+(named_number_t<Number, Tag> lhs, named_number_t<Number, Tag> rhs) noexcept { return named_number_t<Number, Tag>{lhs.get() + rhs.get()}; }
    template <std::integral Number, typename Tag> constexpr named_number_t<Number, Tag> operator+(named_number_t<Number, Tag> lhs, Number rhs) noexcept { return named_number_t<Number, Tag>{lhs.get() + rhs}; }
    template <std::integral Number, typename Tag> constexpr named_number_t<Number, Tag> operator-(named_number_t<Number, Tag> lhs, named_number_t<Number, Tag> rhs) noexcept { return named_number_t<Number, Tag>{lhs.get() - rhs.get()}; }
    template <std::integral Number, typename Tag> constexpr named_number_t<Number, Tag> operator-(named_number_t<Number, Tag> lhs, Number rhs) noexcept { return named_number_t<Number, Tag>{lhs.get() - rhs}; }
    template <std::integral Number, typename Tag> constexpr named_number_t<Number, Tag> operator*(named_number_t<Number, Tag> lhs, named_number_t<Number, Tag> rhs) noexcept { return named_number_t<Number, Tag>{lhs.get() * rhs.get()}; }
    template <std::integral Number, typename Tag> constexpr named_number_t<Number, Tag> operator*(named_number_t<Number, Tag> lhs, Number rhs) noexcept { return named_number_t<Number, Tag>{lhs.get() * rhs}; }
    // no operator/

    template <std::integral Number, typename Tag> constexpr named_number_t<Number, Tag>& operator+=(named_number_t<Number, Tag>& lhs, named_number_t<Number, Tag> rhs) noexcept { lhs.get() = lhs.get() + rhs.get(); return lhs; }
    template <std::integral Number, typename Tag> constexpr named_number_t<Number, Tag>& operator+=(named_number_t<Number, Tag>& lhs, Number rhs) noexcept { lhs.get() = lhs.get() + rhs; return lhs; }
    template <std::integral Number, typename Tag> constexpr named_number_t<Number, Tag>& operator-=(named_number_t<Number, Tag>& lhs, named_number_t<Number, Tag> rhs) noexcept { lhs.get() = lhs.get() - rhs.get(); return lhs; }
    template <std::integral Number, typename Tag> constexpr named_number_t<Number, Tag>& operator-=(named_number_t<Number, Tag>& lhs, Number rhs) noexcept { lhs.get() = lhs.get() - rhs; return lhs; }
    template <std::integral Number, typename Tag> constexpr named_number_t<Number, Tag>& operator*=(named_number_t<Number, Tag>& lhs, named_number_t<Number, Tag> rhs) noexcept { lhs.get() = lhs.get() * rhs.get(); return lhs; }
    template <std::integral Number, typename Tag> constexpr named_number_t<Number, Tag>& operator*=(named_number_t<Number, Tag>& lhs, Number rhs) noexcept { lhs.get() = lhs.get() * rhs; return lhs; }
    // no operator/

}



// ----------------------------------------------------------------------

// template <typename T, typename Tag> struct fmt::formatter<ae::named_t<T, Tag>> : fmt::formatter<T> {
//     template <typename FormatCtx> auto format(const ae::named_t<T, Tag>& nt, FormatCtx& ctx) const { return fmt::formatter<T>::format(nt.get(), ctx); }
// };

template <typename T, typename Tag> struct fmt::formatter<ae::named_string_t<T, Tag>> : fmt::formatter<std::string_view>
{
    template <typename FormatCtx> auto format(const ae::named_string_t<T, Tag>& nt, FormatCtx& ctx) const { return fmt::formatter<std::string_view>::format(static_cast<std::string_view>(nt), ctx); }
};

// ----------------------------------------------------------------------
