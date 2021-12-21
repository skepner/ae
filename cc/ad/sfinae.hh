#pragma once

#include <type_traits>
#include <string>
#include <string_view>

// ----------------------------------------------------------------------

namespace acmacs::sfinae
{
    inline namespace v1
    {
        template <typename T, typename U> struct decay_equiv : std::is_same<std::decay_t<T>, std::decay_t<U>>::type {};
        template <typename T, typename U> constexpr bool decay_equiv_v = decay_equiv<T, U>::value;

        // ----------------------------------------------------------------------

        namespace detail
        {
            template <typename Default, typename AlwaysVoid, template <typename...> typename Op, typename... Args> struct detector
            {
                using value_t = std::false_type;
                using type = Default;
            };

            template <typename Default, template <typename...> typename Op, typename... Args> struct detector<Default, std::void_t<Op<Args...>>, Op, Args...>
            {
                using value_t = std::true_type;
                using type = Op<Args...>;
            };

        } // namespace detail

        template <template <typename...> typename Op, typename... Args> using is_detected = typename detail::detector<void, void, Op, Args...>::value_t;
        template <template <typename...> typename Op, typename... Args> constexpr bool is_detected_v = is_detected<Op, Args...>::value;

        template <template <typename...> typename Op, typename... Args> using detected_t = typename detail::detector<void, void, Op, Args...>::type;

        template <typename Default, template <typename...> typename Op, typename... Args> using detected_or = detail::detector<Default, void, Op, Args...>;
        template <typename Default, template <typename...> typename Op, typename... Args> using detected_or_t = typename detected_or<Default, Op, Args...>::type;

        // ----------------------------------------------------------------------

        namespace detail
        {
            template <typename T> using container_begin_t = decltype(std::declval<T&>().begin());
            template <typename T> using container_end_t = decltype(std::declval<T&>().end());
            template <typename T> using container_resize_t = decltype(std::declval<T&>().resize(1));

            template <typename T> using operator_plusplus_t = decltype(++std::declval<T&>());
            template <typename T> using dereference = decltype(*std::declval<T&>());

        } // namespace detail

        template <typename T> constexpr bool container_has_begin_v = is_detected_v<detail::container_begin_t, T>;
        template <typename T> constexpr bool container_has_end_v = is_detected_v<detail::container_end_t, T>;
        template <typename T> constexpr bool container_has_iterator_v = container_has_begin_v<T>&& container_has_end_v<T>;
        template <typename T> constexpr bool container_has_resize_v = is_detected_v<detail::container_resize_t, T>;

        template <typename T> constexpr bool has_operator_plusplus_v = is_detected_v<detail::operator_plusplus_t, T>;
        template <typename T> constexpr bool has_dereference_v = is_detected_v<detail::dereference, T>;
        template <typename T> constexpr bool is_iterator_v = has_operator_plusplus_v<T> && has_dereference_v<T>;

          // iterator SFINAE: https://stackoverflow.com/questions/12161109/stdenable-if-or-sfinae-for-iterator-or-pointer
        template <typename T> using iterator_t = decltype(*std::declval<T&>(), void(), ++std::declval<T&>(), void());

        // ----------------------------------------------------------------------

        template <typename T> constexpr bool is_const_char_ptr_v = std::is_convertible_v<T, const char*>;
        template <typename T> constexpr bool is_string_v = decay_equiv_v<T, std::string> || decay_equiv_v<T, std::string_view> || is_const_char_ptr_v<T>;
        template <typename T> constexpr bool is_const_char_or_string_view_v = decay_equiv_v<T, std::string_view> || is_const_char_ptr_v<T>;
        template <typename T> using string_only_t = std::enable_if_t<is_string_v<T>>;

        // ----------------------------------------------------------------------

        // overload resolution dispatching (https://foonathan.net/blog/2015/11/16/overload-resolution-3.html)
        template <int priority> struct dispatching_priority : dispatching_priority<priority - 1> {};
        template <> struct dispatching_priority<0> {};
        using dispatching_priority_top = dispatching_priority<10>;

        // ----------------------------------------------------------------------

    } // namespace v1
} // namespace acmacs::sfinae

// ----------------------------------------------------------------------
