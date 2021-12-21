#pragma once

// https://infektor.net/posts/2017-03-31-range-based-enumerate.html

#include <iterator>
#include <utility>

// ----------------------------------------------------------------------

namespace acmacs
{
    namespace _enumerate_internal
    {
        template <typename Iterator, typename index_type> struct enumerate_iterator
        {
            using iterator = Iterator;
              //using index_type = typename std::iterator_traits<iterator>::difference_type;
            using reference = typename std::iterator_traits<iterator>::reference;

            enumerate_iterator(index_type aIndex, iterator aIterator) : index(aIndex), iter(aIterator) {}
            enumerate_iterator& operator++() { ++index; ++iter; return *this; }
            bool operator!=(const enumerate_iterator &other) const { return iter != other.iter; }
            std::pair<index_type, reference> operator*() { return {index, *iter}; }

         private:
            index_type index;
            iterator iter;
        };

          // ----------------------------------------------------------------------

        template <typename Iterator, typename index_type> struct enumerate_range
        {
              // using index_type = typename std::iterator_traits<Iterator>::difference_type;
            using iterator = enumerate_iterator<Iterator, index_type>;

            enumerate_range(Iterator aFirst, Iterator aLast, index_type aInitial) : first(aFirst), last(aLast), initial(aInitial) {}
            iterator begin() const { return iterator(initial, first); }
            iterator end() const { return iterator(0, last); }

         private:
            Iterator first;
            Iterator last;
            index_type initial;
        };

    } // namespace _enumerate_internal

// ----------------------------------------------------------------------

    template <typename Iterator, typename index_type> inline auto enumerate(Iterator first, Iterator last, index_type initial = 0)
    {
        return _enumerate_internal::enumerate_range<Iterator, index_type>(first, last, initial);
    }

    template <typename Container, typename index_type = size_t, typename = std::enable_if_t<std::is_integral_v<index_type>>> inline auto enumerate(Container& content, index_type initial = 0)
    {
        using iter_type = decltype(std::begin(content));
        return _enumerate_internal::enumerate_range<iter_type, index_type>(std::begin(content), std::end(content), initial);
    }

    template <typename Container, typename index_type = size_t, typename = std::enable_if_t<std::is_integral_v<index_type>>> inline auto enumerate(const Container& content, index_type initial = 0)
    {
        using iter_type = decltype(std::begin(content));
        return _enumerate_internal::enumerate_range<iter_type, index_type>(std::begin(content), std::end(content), initial);
    }

    template <typename Container, typename index_type = size_t, typename = std::enable_if_t<std::is_integral_v<index_type>>> inline auto enumerate(Container&&, index_type = 0)
    {
        static_assert(std::is_same_v<int, Container>, "acmacs::enumerate cannot use temp (&&) values as a container (g++9 will destroy container before starting enumeration");
    }

    template <typename Container, typename Func, typename index_type = size_t, typename = std::enable_if_t<std::is_integral_v<index_type>>> inline void enumerate(Container&& content, Func callback, index_type index = 0)
    {
        for (auto& element : content) {
            callback(index, element);
            ++index;
        }
    }

} // namespace acmacs

// ----------------------------------------------------------------------
