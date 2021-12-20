#undef NDEBUG

#include <memory>
#include <cstdlib>

#include "locdb/v3/locdb.hh"
#include "ext/fmt.hh"

// ----------------------------------------------------------------------

namespace ae::locdb::inline v3::detail
{
    static Db* db{nullptr};

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif

    static std::filesystem::path db_path;
    static const std::filesystem::path& get_db_path();

#pragma GCC diagnostic pop
}

// ----------------------------------------------------------------------

const ae::locdb::v3::Db& ae::locdb::v3::get()
{
    if (!detail::db)
        detail::db = new Db(detail::get_db_path());
    return *detail::db;

} // ae::locdb::v3::get

// ----------------------------------------------------------------------

void ae::locdb::v3::db_path(const std::filesystem::path& path)
{
    detail::db_path = path;

} // ae::locdb::v3::db_path

// ----------------------------------------------------------------------

const std::filesystem::path& ae::locdb::v3::detail::get_db_path()
{
    if (db_path.empty()) {
        if (const char* ldb2_path = std::getenv("LOCDB_V2"); ldb2_path)
            db_path = ldb2_path;
        else
            throw error{"\nenv LOCDB_V2 is not set\n"};
    }
    return db_path;

} // ae::locdb::v3::detail::get_db_path

// ----------------------------------------------------------------------

ae::locdb::v3::Db::Db(const std::filesystem::path& path)
    : parser_{path}
{
    for(auto field : parser_.doc().get_object()) {
        const std::string_view key = field.unescaped_key();
        if (key == "  version") {
            if (const std::string_view ver{field.value()}; ver != "locdb-v2")
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
            }
        }
        else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
            fmt::print(">> locdb: unhandled \"{}\"\n", key);
    }

} // ae::locdb::v3::Db::Db

// ----------------------------------------------------------------------

std::string_view ae::locdb::v3::Db::continent(std::string_view country) const
{
    if (const auto found = countries_.find(country); found != countries_.end())
        return continents_[found->second];
    else {
        // fmt::print("no continent for \"{}\"\n", country);
        return {};
    }

} // ae::locdb::v3::Db::continent

// ----------------------------------------------------------------------

std::pair<std::string_view, const ae::locdb::v3::Db::location*> ae::locdb::v3::Db::find(std::string_view look_for) const
{
    if (const auto name_found = names_.find(look_for); name_found != names_.end())
        return {name_found->first, &locations_.find(name_found->second)->second};
    else if (const auto replacement_found = replacements_.find(look_for); replacement_found != replacements_.end())
        return find(replacement_found->second);

    return {{}, nullptr};

} // ae::locdb::v3::Db::find

// ----------------------------------------------------------------------
