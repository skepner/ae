#pragma once

// Dynamic data collection which can be read from json and exported to json

#include <variant>
#include <unordered_map>
#include <vector>

#include "utils/string-hash.hh"

// ----------------------------------------------------------------------

namespace ae
{
    namespace dynamic
    {
        class object;
        class array;

        class null
        {
          public:
            null() = default;
            bool operator==(const null&) const = default;
        };

        using value = std::variant<long, double, bool, std::string, object, array, null>;

        class object
        {
          public:
            object() = default;
            bool operator==(const object&) const;

          private:
            std::unordered_map<std::string, value, string_hash_for_unordered_map, std::equal_to<>> data_;
        };

        class array
        {
          public:
            array() = default;
            bool operator==(const array&) const = default;

          private:
            std::vector<value> data_;
        };

        inline bool object::operator==(const object& rhs) const { return data_ == rhs.data_; }

    } // namespace dynamic

    // ----------------------------------------------------------------------

    class DynamicCollection
    {
      public:
        DynamicCollection() = default;
        bool operator==(const DynamicCollection&) const = default;

        bool empty() const { return std::holds_alternative<dynamic::null>(data_); }

      private:
        dynamic::value data_{dynamic::null{}};

        friend class DynamicCollectionJsonLoader;
    };

} // namespace ae

// ======================================================================
