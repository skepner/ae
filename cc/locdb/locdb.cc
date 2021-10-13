#undef NDEBUG

#include <memory>

#include "locdb/locdb.hh"
#include "ext/simdjson.hh"
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

ae::locationdb::v1::Db::Db(const std::string& path)
{
    simdjson::ondemand::parser parser;
    const auto json = simdjson::padded_string::load(path);
    auto doc = parser.iterate(json);
    auto object = doc.get_object();
    for(auto field : object) {
        const std::string_view key = field.unescaped_key();
        fmt::print("{}\n", key);
    }

} // ae::locationdb::v1::Db::Db

// ----------------------------------------------------------------------
