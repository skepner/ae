// #include <string>
// #include <string_view>
#include <map>
#include <vector>

#include "ext/simdjson.hh"

// ----------------------------------------------------------------------

namespace ae::locationdb::inline v1
{
    class error : public std::runtime_error { public: using std::runtime_error::runtime_error; };

    class Db;
    const Db& get();
    void db_path(const std::filesystem::path& path);

    // ----------------------------------------------------------------------

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
        std::pair<std::string_view, const location*> find(std::string_view look_for) const;

        std::string_view continent(std::string_view country) const;

      private:
        simdjson::Parser parser_;
        std::map<std::string_view, std::string_view> cdc_abbreviations_;
        std::map<std::string_view, location> locations_;
        std::map<std::string_view, uint64_t> countries_; // name -> index in continents_
        std::vector<std::string_view> continents_;
        std::map<std::string_view, std::string_view> names_;
        std::map<std::string_view, std::string_view> replacements_;

        Db(const std::filesystem::path& path);

        friend const Db& get();
    };

}

// ----------------------------------------------------------------------
