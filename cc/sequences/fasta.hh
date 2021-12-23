#pragma once

#include <iterator>
#include <memory>

#include "ext/string.hh"
#include "utils/file.hh"

#include "sequences/raw-sequence.hh"

// ======================================================================

namespace ae::sequences::fasta
{
    class Reader
    {
      public:
        Reader(const std::filesystem::path& filename) : filename_{filename}, data_{ae::file::read(filename)} {}

        struct value_t
        {
            std::shared_ptr<RawSequence> sequence{}; // shared_ptr for accessing via pybind11
            std::string_view filename{};
            size_t line_no{0};
            friend bool operator==(const value_t&, const value_t&) = default;
        };

        class iterator
        {
          public:
            using value_type = value_t;
            // using const_reference = const value_type&;
            // using const_pointer = const value_type*;
            // using element_type = value_t;
            using iterator_category = std::input_iterator_tag;
            using difference_type =  std::ptrdiff_t;

            iterator() = default;
            iterator(const iterator&) = default;
            iterator(iterator&&) = default;
            iterator& operator=(iterator&&) = default;
            iterator& operator=(const iterator&) = default;

            const value_type* operator->() const { return &value_; }
            const value_type& operator*() const  { return value_; }
            iterator& operator++();
            // iterator operator++(int);
            bool operator==(const iterator& rh) const { return value_ == rh.value_; }

          private:
            value_type value_{};
            std::string_view data_{};
            std::string_view::const_iterator next_;
            size_t line_no_{1};
            std::string_view filename_{};

            iterator(std::string_view data, std::string_view filename) : data_{data}, next_{data_.begin()}, filename_{filename} { operator++(); }

            friend class Reader;
        };

        iterator begin() { return iterator{data_, filename_.native()}; }
        iterator end() { return iterator{std::string_view{}, filename_.native()}; }

      private:
        std::filesystem::path filename_;
        std::string data_;
    };
}

// ======================================================================

// static_assert(std::input_iterator<ae::sequences::fasta::Reader::iterator>);

// ======================================================================
