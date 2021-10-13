// #include <string>
// #include <string_view>
#include <map>
#include <vector>

#include "ext/simdjson.hh"

// ----------------------------------------------------------------------

namespace ae::locationdb::inline v1
{
    class Db
    {
      public:
        struct location
        {
            double latitude;
            double longitude;
            std::string_view country;
            std::string_view province;
        };

        // returns empty string if not found
        // returns replacement if available
        std::string_view find(std::string_view look_for) const;

      private:
        simdjson::padded_string json_;
        std::map<std::string_view, std::string_view> cdc_abbreviations_;
        std::map<std::string_view, location> locations_;
        std::map<std::string_view, uint64_t> countries_; // name -> index in continents_
        std::vector<std::string_view> continents_;
        std::map<std::string_view, std::string_view> names_;
        std::map<std::string_view, std::string_view> replacements_;

        Db(std::string_view path);

        friend const Db& get();
    };

    const Db& get();
}

// ----------------------------------------------------------------------
