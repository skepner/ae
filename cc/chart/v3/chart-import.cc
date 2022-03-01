#include "ext/simdjson.hh"
#include "utils/timeit.hh"
// #include "utils/file.hh"
#include "chart/v3/chart.hh"

// ----------------------------------------------------------------------

void ae::chart::v3::Chart::write(const std::filesystem::path& filename) const
{
    Timeit ti{"exporting chart", std::chrono::milliseconds{5000}};

} // ae::chart::v3::Chart::write

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
                else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
                    fmt::print(">> [chart] unhandled \"{}\"\n", key);
            }
        }
        catch (simdjson_error& err) {
            // throw Error{"> {} parsing error: {} at {} \"{}\"\n", filename.native(), err.what(), parser.current_location_offset(), parser.current_location_snippet(50)};
        }
    }
    catch (simdjson_error& err) {
        throw Error{"> {} json parser creation error: {} (UNESCAPED_CHARS means a char < 0x20)\n", 5, 6}; // filename.native(), err.what()};
    }

} // ae::chart::v3::Chart::read

// ----------------------------------------------------------------------
