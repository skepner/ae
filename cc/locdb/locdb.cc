#undef NDEBUG

#include <memory>

#include "locdb/locdb.hh"
#include "ext/fmt.hh"

// ----------------------------------------------------------------------

namespace ae::locationdb::inline v1
{
    static Db* db{nullptr};

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif

    static std::string db_path{"/r/locationdb.json"};

#pragma GCC diagnostic pop
}

// ----------------------------------------------------------------------

const ae::locationdb::v1::Db& ae::locationdb::v1::get()
{
    if (!db)
        db = new Db(db_path);
    return *db;

} // ae::locationdb::v1::get

// ----------------------------------------------------------------------

ae::locationdb::v1::Db::Db(std::string_view path)
{
    simdjson::ondemand::parser parser;
    json_ = simdjson::padded_string::load(path);
    auto doc = parser.iterate(json_);
    // auto object = doc.get_object();
    for(auto field : doc.get_object()) {
        const std::string_view key = field.unescaped_key();
        if (key == "  version") {
            if (const std::string_view ver{field.value()}; ver != "locationdb-v2")
                throw std::runtime_error{fmt::format("locdb: unsupported version: \"{}\"", ver)};
        }
        else if (key == "cdc_abbreviations") {
            for (auto ca_en : field.value().get_object())
                cdc_abbreviations_.emplace(ca_en.unescaped_key(), ca_en.value());
        }
        else if (key == "continents") {
            for (auto continent : field.value().get_array())
                continents_.emplace_back(continent);
        }
        else if (key == "countries") {
            for (auto country : field.value().get_object())
                countries_.emplace(country.unescaped_key(), country.value());
        }
        else if (key == "names") {
            for (auto name : field.value().get_object())
                names_.emplace(name.unescaped_key(), name.value());
        }
        else if (key == "replacements") {
            for (auto replacement : field.value().get_object())
                replacements_.emplace(replacement.unescaped_key(), replacement.value());
        }
        else if (key == "locations") {
            for (auto loc_en : field.value().get_object()) {
                auto val = loc_en.value().get_array();
                auto it = val.begin();
                // location loc{*it, *++it, *++it, *++it};
                locations_.emplace(loc_en.unescaped_key(), location{*it, *++it, *++it, *++it});

            //     auto it = val.begin();
            //     location loc;
            //     loc.latitude = *it;
            //     ++it;
            //     loc.longitude = *it;
            //     ++it;
            //     loc.country = std::string_view{*it};
            //     ++it;
            //     loc.province = std::string_view{*it};
            //     locations_.emplace(std::string_view{loc_en.unescaped_key()}, loc);
            }
        }
        else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
            fmt::print(">> locdb: unhandled \"{}\"\n", key);
    }

} // ae::locationdb::v1::Db::Db

// ----------------------------------------------------------------------

std::string_view ae::locationdb::v1::Db::find(std::string_view look_for) const
{
    if (const auto name_found = names_.find(look_for); name_found != names_.end())
        return name_found->first;
    else if (const auto replacement_found = replacements_.find(look_for); replacement_found != replacements_.end())
        return replacement_found->second;

    return {};

} // ae::locationdb::v1::Db::find

// ----------------------------------------------------------------------
