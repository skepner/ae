#include "ext/simdjson.hh"
#include "utils/log.hh"
#include "utils/timeit.hh"
// #include "utils/file.hh"
#include "chart/v3/chart.hh"

// ----------------------------------------------------------------------

void ae::chart::v3::Chart::write(const std::filesystem::path& filename) const
{
    Timeit ti{"exporting chart", std::chrono::milliseconds{5000}};

} // ae::chart::v3::Chart::write

// ----------------------------------------------------------------------

// class unhandled_key : public std::runtime_error
// {
//   public:
//     unhandled_key(std::initializer_list<std::string_view> keys) : std::runtime_error{fmt::format("[chart] unhandled \"{}\"", fmt::join(keys, "\":\""))} {}
// };

inline void unhandled_key(std::initializer_list<std::string_view> keys)
{
    fmt::print(stderr, ">> [chart] unhandled \"{}\"\n", fmt::join(keys, "\":\""));
}

// ----------------------------------------------------------------------

// returns if key was handled
inline bool read_table_source(ae::chart::v3::TableSource& target, std::string_view key, ::simdjson::ondemand::value value)
{
    bool handled{true};
    if (key.size() == 1) {
        switch (key[0]) {
            case 'v':
                target.virus(ae::virus::virus_t{static_cast<std::string_view>(value)});
                break;
            case 'V':
                target.type_subtype(ae::virus::type_subtype_t{static_cast<std::string_view>(value)});
                break;
            case 'A':
                target.assay(ae::chart::v3::Assay{static_cast<std::string_view>(value)});
                break;
            case 'D':
                target.date(ae::chart::v3::TableDate{static_cast<std::string_view>(value)});
                break;
            case 'N':
                target.name(std::string{static_cast<std::string_view>(value)});
                break;
            case 'l':
                target.lab(ae::chart::v3::Lab{static_cast<std::string_view>(value)});
                break;
            case 'r':
                target.rbc_species(ae::chart::v3::RbcSpecies{static_cast<std::string_view>(value)});
                break;
            default:
                handled = false;
                break;
        }
    }
    else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
        handled = false;
    return handled;
}

inline void read_info(ae::chart::v3::Info& info, ::simdjson::ondemand::object source)
{
    for (auto field : source) {
        if (const std::string_view key = field.unescaped_key(); !read_table_source(info, key, field.value())) {
            if (key == "S") {
                for (auto en : field.value().get_array()) {
                    auto& target = info.sources().emplace_back();
                    for (auto s_field : en.get_object())
                        read_table_source(target, s_field.unescaped_key(), s_field.value());
                }
            }
            else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
                unhandled_key({"c", "i", key});
        }
    }
}

// ----------------------------------------------------------------------

void ae::chart::v3::Chart::read(const std::filesystem::path& filename)
{
    Timeit ti{"importing chart", std::chrono::milliseconds{5000}};
    using namespace ae::simdjson;
    try {
        Parser parser{filename};
        try {
            for (auto field : parser.doc().get_object()) {
                const std::string_view key = field.unescaped_key();
                // fmt::print(">>>> key \"{}\"\n", key);
                if (key == "  version") {
                    if (const std::string_view ver{field.value()}; ver != "acmacs-ace-v1")
                        throw Error{"unsupported version: \"{}\"", ver};
                }
                else if (key == "c") {
                    for (auto field_c : field.value().get_object()) {
                        if (const std::string_view key_c = field_c.unescaped_key(); key_c.size() == 1) {
                            switch (key_c[0]) {
                                case 'i':
                                    read_info(info(), field_c.value().get_object());
                                    break;
                                default:
                                    unhandled_key({"c", key_c});
                                    break;
                            }
                        }
                        else if (key_c[0] != '?' && key_c[0] != ' ' && key_c[0] != '_')
                            unhandled_key({"c", key_c});
                    }
                }
                else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
                    unhandled_key({key});
            }
        }
        catch (simdjson_error& err) {
            throw Error{"> {} parsing error: {} at {} \"{}\"\n", filename.native(), err.what(), parser.current_location_offset(), parser.current_location_snippet(50)};
        }
    }
    catch (simdjson_error& err) {
        throw Error{"> {} json parser creation error: {} (UNESCAPED_CHARS means a char < 0x20)\n", filename.native(), err.what()};
    }

} // ae::chart::v3::Chart::read

// ----------------------------------------------------------------------
