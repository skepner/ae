#pragma once

// Dynamic data collection which can be read from json and exported to json

#include <variant>
#include <unordered_map>
#include <vector>

#include "ext/simdjson.hh"
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
        };

        using value = std::variant<int, double, bool, std::string, object, array, null>;

        class object
        {
          public:
            object() = default;

          private:
            std::unordered_map<std::string, value, string_hash_for_unordered_map, std::equal_to<>> data_;
        };

        class array
        {
          public:
            array() = default;

          private:
            std::vector<value> data_;
        };

    } // namespace dynamic

    // ----------------------------------------------------------------------

    class DynamicCollection
    {
      public:
        DynamicCollection() = default;
        DynamicCollection(simdjson::ondemand::value source);

      private:
        dynamic::value data_;
    };

} // namespace ae

// ======================================================================
