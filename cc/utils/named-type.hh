#pragma once

#include "ext/fmt.hh"
#include "utils/float.hh"
#include "utils/concepts.hh"

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
        template <constructible_from<T> T2> explicit constexpr named_t(T2&& value) : value_(std::forward<T2>(value)) {}

        named_t& operator=(const named_t&) = default;
        named_t& operator=(named_t&&) = default;

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif

        auto operator<=>(const named_t&) const = default;
        auto operator<=>(const T& rhs) const { return value_ <=> rhs; }
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
        T value_{};
    };

    // ----------------------------------------------------------------------

    template <typename T, typename Tag> class named_string_t : public named_t<T, Tag>
    {
      public:
        using named_t<T, Tag>::named_t;
        using named_t<T, Tag>::operator=;

        constexpr bool empty() const { return this->get().empty(); }
        constexpr size_t size() const { return this->get().size(); }
        void clear() { this->get().clear(); }
        auto operator[](size_t pos) const { return this->get().operator[](pos); }
        template <typename S> auto find(S look_for) const { return this->get().find(look_for); }

        operator std::string_view() const { return this->get(); }
    };

    // ----------------------------------------------------------------------

    template <std::integral T, typename Tag> class named_number_t : public named_t<T, Tag>
    {
      public:
        // using difference_type = long; // named_number_t<T, Tag>;

        constexpr named_number_t() = default;
        template <std::integral T2> explicit constexpr named_number_t(T2 val) : named_t<T, Tag>(static_cast<T>(val)) {}

        using named_t<T, Tag>::operator=;
        using named_t<T, Tag>::operator==;
        using named_t<T, Tag>::operator<=>;

        constexpr named_number_t<T, Tag>& operator++()
        {
            ++this->get();
            return *this;
        }

        constexpr named_number_t<T, Tag> operator++(int)
        {
            const named_number_t saved{*this};
            ++this->get();
            return saved;
        }

        constexpr named_number_t<T, Tag>& operator--()
        {
            --this->get();
            return *this;
        }

        constexpr named_number_t<T, Tag> operator--(int)
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

    template <typename Tag> using named_size_t = named_number_t<size_t, Tag>;

    // ----------------------------------------------------------------------

    template <typename Tag>
    class named_double_t : public named_t<double, Tag>
    {
      public:
        using named_t<double, Tag>::named_t;
        using named_t<double, Tag>::operator=;

        bool operator==(double rhs) const { return float_equal(this->get(), rhs); }
        bool operator==(named_t<double, Tag> rhs) const { return float_equal(this->get(), rhs.get()); }
        bool operator==(named_double_t rhs) const { return float_equal(this->get(), rhs.get()); }
    };

    template <typename Tag> constexpr named_double_t<Tag> operator-(named_double_t<Tag> rhs) noexcept { return named_double_t<Tag>{-rhs.get()}; }
    template <typename Tag> constexpr named_double_t<Tag> operator+(named_double_t<Tag> lhs, named_double_t<Tag> rhs) noexcept { return named_double_t<Tag>{lhs.get() + rhs.get()}; }
    template <typename Tag> constexpr named_double_t<Tag> operator+(named_double_t<Tag> lhs, double rhs) noexcept { return named_double_t<Tag>{lhs.get() + rhs}; }
    template <typename Tag> constexpr named_double_t<Tag> operator-(named_double_t<Tag> lhs, named_double_t<Tag> rhs) noexcept { return named_double_t<Tag>{lhs.get() - rhs.get()}; }
    template <typename Tag> constexpr named_double_t<Tag> operator-(named_double_t<Tag> lhs, double rhs) noexcept { return named_double_t<Tag>{lhs.get() - rhs}; }
    template <typename Tag> constexpr named_double_t<Tag> operator*(named_double_t<Tag> lhs, named_double_t<Tag> rhs) noexcept { return named_double_t<Tag>{lhs.get() * rhs.get()}; }
    template <typename Tag> constexpr named_double_t<Tag> operator*(named_double_t<Tag> lhs, double rhs) noexcept { return named_double_t<Tag>{lhs.get() * rhs}; }
    template <typename Tag> constexpr named_double_t<Tag> operator/(named_double_t<Tag> lhs, named_double_t<Tag> rhs) noexcept { return named_double_t<Tag>{lhs.get() / rhs.get()}; }
    template <typename Tag> constexpr named_double_t<Tag> operator/(named_double_t<Tag> lhs, double rhs) noexcept { return named_double_t<Tag>{lhs.get() / rhs}; }

    template <typename Tag> constexpr named_double_t<Tag>& operator+=(named_double_t<Tag>& lhs, named_double_t<Tag> rhs) noexcept { lhs.get() = lhs.get() + rhs.get(); return lhs; }
    template <typename Tag> constexpr named_double_t<Tag>& operator+=(named_double_t<Tag>& lhs, double rhs) noexcept { lhs.get() = lhs.get() + rhs; return lhs; }
    template <typename Tag> constexpr named_double_t<Tag>& operator-=(named_double_t<Tag>& lhs, named_double_t<Tag> rhs) noexcept { lhs.get() = lhs.get() - rhs.get(); return lhs; }
    template <typename Tag> constexpr named_double_t<Tag>& operator-=(named_double_t<Tag>& lhs, double rhs) noexcept { lhs.get() = lhs.get() - rhs; return lhs; }
    template <typename Tag> constexpr named_double_t<Tag>& operator*=(named_double_t<Tag>& lhs, named_double_t<Tag> rhs) noexcept { lhs.get() = lhs.get() * rhs.get(); return lhs; }
    template <typename Tag> constexpr named_double_t<Tag>& operator*=(named_double_t<Tag>& lhs, double rhs) noexcept { lhs.get() = lhs.get() * rhs; return lhs; }
    template <typename Tag> constexpr named_double_t<Tag>& operator/=(named_double_t<Tag>& lhs, named_double_t<Tag> rhs) noexcept { lhs.get() = lhs.get() / rhs.get(); return lhs; }
    template <typename Tag> constexpr named_double_t<Tag>& operator/=(named_double_t<Tag>& lhs, double rhs) noexcept { lhs.get() = lhs.get() / rhs; return lhs; }

    // ----------------------------------------------------------------------

}

// ----------------------------------------------------------------------

// template <typename T, typename Tag> struct fmt::formatter<ae::named_t<T, Tag>> : fmt::formatter<T> {
//     template <typename FormatCtx> auto format(const ae::named_t<T, Tag>& nt, FormatCtx& ctx) const { return fmt::formatter<T>::format(nt.get(), ctx); }
// };

template <typename T, typename Tag> struct fmt::formatter<ae::named_string_t<T, Tag>> : fmt::formatter<std::string_view>
{
    template <typename FormatCtx> auto format(const ae::named_string_t<T, Tag>& nt, FormatCtx& ctx) const { return fmt::formatter<std::string_view>::format(static_cast<std::string_view>(nt), ctx); }
};

template <typename T, typename Tag> struct fmt::formatter<ae::named_number_t<T, Tag>> : fmt::formatter<T>
{
    template <typename FormatCtx> auto format(const ae::named_number_t<T, Tag>& nt, FormatCtx& ctx) const { return fmt::formatter<T>::format(static_cast<T>(nt), ctx); }
};

template <typename Tag> struct fmt::formatter<ae::named_double_t<Tag>> : fmt::formatter<double>
{
    template <typename FormatCtx> auto format(const ae::named_double_t<Tag>& nt, FormatCtx& ctx) const { return fmt::formatter<double>::format(static_cast<double>(nt), ctx); }
};

// ----------------------------------------------------------------------
