#pragma once

#include <map>
#include <vector>

#include "ext/simdjson.hh"

// ----------------------------------------------------------------------

namespace ae::locdb::inline v3
{
    class error : public std::runtime_error
    {
      public:
        using std::runtime_error::runtime_error;
    };

    class Db;
    const Db& get();
    void db_path(const std::filesystem::path& path);

    using Latitude = double;
    using Longitude = double;

    // ----------------------------------------------------------------------

    class Db
    {
      public:
        struct location
        {
            Latitude latitude;
            Longitude longitude;
            std::string_view country;
            std::string_view province;
        };

        // returns empty string if not found
        // returns replacement if available
        std::pair<std::string_view, const location*> find(std::string_view look_for) const;

        std::string_view continent(std::string_view country) const;

        std::string_view country(std::string_view loc) const
        {
            if (const auto [_, found] = find(loc); found)
                return found->country;
            else
                return {};
        }

        std::string abbreviation(std::string_view loc) const;

        std::string_view find_cdc_abbreviation_by_name(std::string_view name) const;

      private:
        simdjson::Parser parser_;
        std::map<std::string_view, std::string_view> cdc_abbreviations_{};
        std::map<std::string_view, location> locations_{};
        std::map<std::string_view, uint64_t> countries_{}; // name -> index in continents_
        std::vector<std::string_view> continents_{};
        std::map<std::string_view, std::string_view> names_{};
        std::map<std::string_view, std::string_view> replacements_{};

        Db(const std::filesystem::path& path);

        friend const Db& get();
    };

} // namespace ae::locdb::inline v3

// ----------------------------------------------------------------------
