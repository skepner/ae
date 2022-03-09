#include "utils/log.hh"
#include "utils/timeit.hh"
#include "utils/file.hh"
#include "chart/v3/chart.hh"

// ----------------------------------------------------------------------

void ae::chart::v3::Chart::write(const std::filesystem::path& filename) const
{
    Timeit ti{fmt::format("exporting chart to {}", filename), std::chrono::milliseconds{100}};

    fmt::memory_buffer out;

    const auto put_str = [&out](const auto& value, auto&& condition, std::string_view key, bool comma, std::string_view after_comma = {}) -> bool {
        if (condition(value)) {
            if (comma)
                fmt::format_to(std::back_inserter(out), ",");
            if (!after_comma.empty())
                fmt::format_to(std::back_inserter(out), "{}", after_comma);
            fmt::format_to(std::back_inserter(out), "\"{}\":\"{}\"", key, value);
            return true;
        }
        else
            return comma;
    };

    const auto put_array_str = [&out](const auto& value, auto&& condition, std::string_view key, bool comma) -> bool {
        if (condition(value)) {
            if (comma)
                fmt::format_to(std::back_inserter(out), ",");
            fmt::format_to(std::back_inserter(out), "\"{}\":[\"{}\"]", key, fmt::join(value, "\",\""));
            return true;
        }
        else
            return comma;
    };

    const auto put_array_int = [&out](const auto& value, auto&& condition, std::string_view key, bool comma) -> bool {
        if (condition(value)) {
            if (comma)
                fmt::format_to(std::back_inserter(out), ",");
            fmt::format_to(std::back_inserter(out), "\"{}\":[{}]", key, fmt::join(value, ","));
            return true;
        }
        else
            return comma;
    };

    const auto not_empty = [](const auto& val) { return !val.empty(); };

    fmt::format_to(std::back_inserter(out), R"({{"_": "-*- js-indent-level: 1 -*-",
 "  version": "acmacs-ace-v1",
 "?created": "ae",
 "c": {{
)");

    //  "i" |     |     |     | key-value pairs                  | chart meta information
    //      | "N" |     |     | str                              | user supplied name
    //      | "v" |     |     | str                              | virus, e.g. INFLUENZA (default, if omitted), HPV, generic, DENGE
    //      | "V" |     |     | str                              | virus type and subtype, e.g. B or A(H3N2) or serotype
    //      | "l" |     |     | str                              | lab
    //      | "A" |     |     | str                              | assay: HI, HINT, FRA, FOCUST REDUCTION, PRNT
    //      | "D" |     |     | str, date YYYYMMDD.NNN           | table/assay date and number (if multiple on that day), e.g. 20160602.002
    //      | "r" |     |     | str                              | RBCs species of HI assay, e.g. "turkey"
    //      | "s" |     |     | str                              | UNUSED subset/lineage, e.g. "2009PDM"
    //      | "T" |     |     | str                              | table type "A[NTIGENIC]" - default, "G[ENETIC]"
    //      | "S" |     |     | array of key-value pairs         | source table info list, each entry is like "i"

    const auto put_table_source = [put_str, not_empty](const TableSource& source) -> bool {
        auto comma = put_str(source.name(), not_empty, "N", false);
        comma = put_str(source.virus(), [](auto&& val) { return !val.empty() && val != ae::virus::virus_t{"INFLUENZA"}; }, "v", comma);
        comma = put_str(source.type_subtype(), not_empty, "V", comma);
        comma = put_str(source.lab(), not_empty, "l", comma);
        comma = put_str(source.assay(), not_empty, "A", comma);
        comma = put_str(source.date(), not_empty, "D", comma);
        comma = put_str(source.rbc_species(), not_empty, "r", comma);
        return comma;
    };

    fmt::format_to(std::back_inserter(out), "  \"i\": {{");
    auto comma1 = put_table_source(info());
    if (const auto& sources = info().sources(); !sources.empty()) {
        if (comma1)
            fmt::format_to(std::back_inserter(out), ",");
        fmt::format_to(std::back_inserter(out), "\n        \"S\": [");
        auto comma2 = false;
        for (const auto& src : sources) {
            if (comma2)
                fmt::format_to(std::back_inserter(out), ",");
            fmt::format_to(std::back_inserter(out), "\n         {{");
            comma2 = put_table_source(src);
            fmt::format_to(std::back_inserter(out), "}}");
        }
        fmt::format_to(std::back_inserter(out), "\n        ]");
        comma1 = true;
    }
    fmt::format_to(std::back_inserter(out), "}},\n");

    // Antigens
    //  "N" | str, mandatory                   | name: TYPE(SUBTYPE)/[HOST/]LOCATION/ISOLATION/YEAR or CDC_ABBR NAME or UNRECOGNIZED NAME
    //  "a" | array of str                     | annotations that distinguish antigens (prevent from merging): ["DISTINCT"], mutation information, unrecognized extra data
    //  "R" | str                              | reassortant, e.g. "NYMC-51C"
    //  "D" | str, date YYYYMMDD or YYYY-MM-DD | isolation date
    //  "L" | str                              | lineage: "Y[AMAGATA]" or "V[ICTORIA]"
    //  "P" | str                              | passage, e.g. "MDCK2/SIAT1 (2016-05-12)"
    //  "l" | array of str                     | lab ids ([lab#id]), e.g. ["CDC#2013706008"]
    //  "A" | str                              | aligned amino-acid sequence
    //  "B" | str                              | aligned nucleotide sequence
    //  "s" | key-value  pairs                 | semantic attributes by group (see below the table)
    //  "C" | str                              | (DEPRECATED, use "s") continent: "ASIA", "AUSTRALIA-OCEANIA", "NORTH-AMERICA", "EUROPE", "RUSSIA", "AFRICA", "MIDDLE-EAST", "SOUTH-AMERICA", "CENTRAL-AMERICA"
    //  "c" | array of str                     | (DEPRECATED, use "s") clades, e.g. ["5.2.1"]
    //  "S" | str                              | (DEPRECATED, use "s") single letter semantic boolean attributes: R - reference, E - egg, V - current vaccine, v - previous vaccine, S - vaccine surrogate

    fmt::format_to(std::back_inserter(out), "  \"a\": [");
    auto comma3 = false;
    for (const auto& antigen : antigens()) {
        if (comma3)
            fmt::format_to(std::back_inserter(out), ",");
        fmt::format_to(std::back_inserter(out), "\n   {{");
        auto comma4 = put_str(antigen.name(), not_empty, "N", false);
        comma4 = put_array_str(antigen.annotations(), not_empty, "a", comma4);
        comma4 = put_str(antigen.reassortant(), not_empty, "R", comma4);
        comma4 = put_str(antigen.date(), not_empty, "D", comma4);
        comma4 = put_str(antigen.lineage(), not_empty, "L", comma4);
        comma4 = put_str(antigen.passage(), not_empty, "P", comma4);
        comma4 = put_array_str(antigen.lab_ids(), not_empty, "l", comma4);
        comma4 = put_str(antigen.aa(), not_empty, "A", comma4); // , "\n    ");
        comma4 = put_str(antigen.nuc(), not_empty, "B", comma4); // , "\n    ");
        // "s": {} -- semantic
        fmt::format_to(std::back_inserter(out), "}}");
        comma3 = true;
    }
    fmt::format_to(std::back_inserter(out), "\n  ],\n");

    // Sera
    //  "N" | str, mandatory   | name: TYPE(SUBTYPE)/[HOST/]LOCATION/ISOLATION/YEAR or CDC_ABBR NAME or UNRECOGNIZED NAME
    //  "a" | array of str     | annotations that distinguish sera (prevent from merging), e.g. ["BOOSTED", "CONC 2:1", "HA-Y156T"]
    //  "R" | str              | reassortant, e.g. "NYMC-51C"
    //  "L" | str              | lineage: "Y[AMAGATA]" or "V[ICTORIA]"
    //  "P" | str              | passage, e.g. "MDCK2/SIAT1 (2016-05-12)"
    //  "I" | str              | serum id, e.g "CDC 2016-045"
    //  "s" | str              | serum species, e.g "FERRET"
    //  "h" | array of numbers | homologous antigen indices, e.g. [0]
    //  "A" | str              | aligned amino-acid sequence
    //  "B" | str              | aligned nucleotide sequence
    //  "s" | key-value  pairs | semantic attributes by group (see below the table)
    //  "C" | str              | (DEPRECATED, use "s") continent: "ASIA", "AUSTRALIA-OCEANIA", "NORTH-AMERICA", "EUROPE", "RUSSIA", "AFRICA", "MIDDLE-EAST", "SOUTH-AMERICA", "CENTRAL-AMERICA"
    //  "c" | array of str     | (DEPRECATED, use "s") clades, e.g. ["5.2.1"]
    //  "S" | str              | (DEPRECATED, use "s") single letter semantic boolean attributes: E - egg

    fmt::format_to(std::back_inserter(out), "  \"s\": [");
    auto comma5 = false;
    for (const auto& serum : sera()) {
        if (comma5)
            fmt::format_to(std::back_inserter(out), ",");
        fmt::format_to(std::back_inserter(out), "\n   {{");
        auto comma6 = put_str(serum.name(), not_empty, "N", false);
        comma6 = put_array_str(serum.annotations(), not_empty, "a", comma6);
        comma6 = put_str(serum.reassortant(), not_empty, "R", comma6);
        comma6 = put_str(serum.lineage(), not_empty, "L", comma6);
        comma6 = put_str(serum.passage(), not_empty, "P", comma6);
        comma6 = put_str(serum.serum_id(), not_empty, "I", comma6);
        comma6 = put_str(serum.serum_species(), not_empty, "s", comma6);
        comma6 = put_array_int(serum.homologous_antigens(), not_empty, "h", comma6);
        comma6 = put_str(serum.aa(), not_empty, "A", comma6); // , "\n    ");
        comma6 = put_str(serum.nuc(), not_empty, "B", comma6); // , "\n    ");
        // "s": {} -- semantic
        fmt::format_to(std::back_inserter(out), "}}");
        comma5 = true;
    }
    fmt::format_to(std::back_inserter(out), "  ],\n");

    // Titers
    //  "l" | array of arrays of str       | dense matrix of titers
    //  "d" | array of key(str)-value(str) | sparse matrix, entry for each antigen present, key is serum index, value is titer, dont-care titers omitted
    //  "L" | array of arrays of key-value | layers of titers, each top level array element as in "d" or "l"

    fmt::format_to(std::back_inserter(out), "  \"t\": {{");
    if (titers().is_dense()) {
        fmt::format_to(std::back_inserter(out), "\n   \"l\": [");
        for (const auto ag_no : titers().number_of_antigens()) {
            if (ag_no != antigen_index{0})
                fmt::format_to(std::back_inserter(out), ",");
            fmt::format_to(std::back_inserter(out), "\n    [");
            for (const auto sr_no : titers().number_of_sera()) {
                if (sr_no != serum_index{0})
                    fmt::format_to(std::back_inserter(out), ",");
                fmt::format_to(std::back_inserter(out), "{:>7s}", fmt::format("\"{}\"", titers().titer(ag_no, sr_no)));
            }
            fmt::format_to(std::back_inserter(out), "]");
        }
        fmt::format_to(std::back_inserter(out), "\n   ]");
    }
    else {
        fmt::format_to(std::back_inserter(out), "\n   \"d\": [");
        fmt::format_to(std::back_inserter(out), "\n   ]");
    }
    if (titers().number_of_layers() > layer_index{1}) {
        fmt::format_to(std::back_inserter(out), ",\n   \"L\": [");
        fmt::format_to(std::back_inserter(out), "\n   ]");
    }
    fmt::format_to(std::back_inserter(out), "\n  }},\n");

// -----+-----+-----+-----+----------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------|
//  "C" |     |     |     | array of floats                  | forced column bases for a new projections                                                                                                                      |
// -----+-----+-----+-----+----------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------|
//  "P" |     |     |     | array of key-value pairs         | Projections                                                                                                                                                    |
//      | "c" |     |     | str (or any)                     | comment                                                                                                                                                        |
//      | "l" |     |     | array of arrays of floats        | layout, if point is disconnected: emtpy list or ?[NaN, NaN]                                                                                                    |
//      | "i" |     |     | integer                          | UNUSED number of iterations?                                                                                                                                   |
//      | "s" |     |     | float                            | stress                                                                                                                                                         |
//      | "m" |     |     | str                              | minimum column basis, "none" (default), "1280"                                                                                                                 |
//      | "C" |     |     | array of floats                  | forced column bases                                                                                                                                            |
//      | "t" |     |     | array of floats                  | transformation matrix                                                                                                                                          |
//      | "g" |     |     | array of floats                  | antigens_sera_gradient_multipliers, float for each point                                                                                                       |
//      | "f" |     |     | array of floats                  | avidity adjusts (antigens_sera_titers_multipliers), float for each point                                                                                       |
//      | "d" |     |     | boolean                          | dodgy_titer_is_regular, false is default                                                                                                                       |
//      | "e" |     |     | float                            | stress_diff_to_stop                                                                                                                                            |
//      | "U" |     |     | array of integers                | list of indices of unmovable points (antigen/serum attribute for stress evaluation)                                                                            |
//      | "D" |     |     | array of integers                | list of indices of disconnected points (antigen/serum attribute for stress evaluation)                                                                         |
//      | "u" |     |     | array of integers                | list of indices of points unmovable in the last dimension (antigen/serum attribute for stress evaluation)                                                      |
// -----+-----+-----+-----+----------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------|
//  "R" |     |     |     | key-value(key-value) pairs       | sematic attributes based plot specifications, key: name of the style                                                                                           |
//      |     |     |     |                                  |                                                                                                                                                                |
//      |     |     |     |                                  |                                                                                                                                                                |
//      |     |     |     |                                  |                                                                                                                                                                |
//      |     |     |     |                                  |                                                                                                                                                                |
//      |     |     |     |                                  |                                                                                                                                                                |
//      |     |     |     |                                  |                                                                                                                                                                |
//      |     |     |     |                                  |                                                                                                                                                                |
//      |     |     |     |                                  |                                                                                                                                                                |
//      |     |     |     |                                  |                                                                                                                                                                |
//      |     |     |     |                                  |                                                                                                                                                                |
// -----+-----+-----+-----+----------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------|
//  "p" |     |     |     | key-value pairs                  | legacy lispmds stype plot specification                                                                                                                        |
//      | "d" |     |     | array of integers                | drawing order, point indices                                                                                                                                   |
//      | "E" |     |     | key-value pairs                  | error line positive, default: {"c": "blue"}                                                                                                                    |
//      | "e" |     |     | key-value pairs                  | error line negative, default: {"c": "red"}                                                                                                                     |
//      | "g" |     |     | ?                                | ? grid data                                                                                                                                                    |
//      | "P" |     |     | array of key-value pairs         | list of plot styles                                                                                                                                            |
//      |     | "+" |     | boolean                          | if point is shown, default is true, disconnected points are usually not shown and having NaN coordinates in layout                                             |
//      |     | "F" |     | color, str                       | fill color: #FF0000 or T[RANSPARENT] or color name (red, green, blue, etc.), default is transparent                                                            |
//      |     | "O" |     | color, str                       | outline color: #000000 or T[RANSPARENT] or color name (red, green, blue, etc.), default is black                                                               |
//      |     | "o" |     | float                            | outline width, default 1.0                                                                                                                                     |
//      |     | "S" |     | str                              | shape: "C[IRCLE]" (default), "B[OX]", "T[RIANGLE]", "E[GG]", "U[GLYEGG]"                                                                                       |
//      |     | "s" |     | float                            | size, default 1.0                                                                                                                                              |
//      |     | "r" |     | float                            | rotation in radians, default 0.0                                                                                                                               |
//      |     | "a" |     | float                            | aspect ratio, default 1.0                                                                                                                                      |
//      |     | "l" |     | key-value pairs                  | label style                                                                                                                                                    |
//      |     |     | "+" | boolean                          | if label is shown                                                                                                                                              |
//      |     |     | "p" | list of two floats               | label position (2D only), list of two doubles, default is [0, 1] means under point                                                                             |
//      |     |     | "t" | str                              | label text if forced by user                                                                                                                                   |
//      |     |     | "f" | str                              | font face                                                                                                                                                      |
//      |     |     | "S" | str                              | font slant: "normal" (default), "italic"                                                                                                                       |
//      |     |     | "W" | str                              | font weight: "normal" (default), "bold"                                                                                                                        |
//      |     |     | "s" | float                            | label size, default 1.0                                                                                                                                        |
//      |     |     | "c" | color, str                       | label color, default: "black"                                                                                                                                  |
//      |     |     | "r" | float                            | label rotation, default 0.0                                                                                                                                    |
//      |     |     | "i" | float                            | addtional interval between lines as a fraction of line height, default 0.2                                                                                     |
//      | "p" |     |     | arrsy of integers                | index in "P" for each point, antigens followed by sera                                                                                                                                                               |
//      | "l" |     |     | array of integers                | ? for each procrustes line, index in the "L" list                                                                                                              |
//      | "L" |     |     | array                            | ? list of procrustes lines styles                                                                                                                              |
//      | "s" |     |     | array of integers                | list of point indices for point shown on all maps in the time series                                                                                           |
//      | "t" |     |     | key-value pairs                  | ? title style                                                                                                                                                  |
// -----+-----+-----+-----+----------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------|
//  "x" |     |     |     | key-value pairs                  | extensions not used by acmacs                                                                                                                                  |
// -----+-----+-----+-----+----------------------------------+----------------------------------------------------------------------------------------------------------------------------------------------------------------|

    fmt::format_to(std::back_inserter(out), "  \"x\": {{\n");
    fmt::format_to(std::back_inserter(out), "  }}\n");

    fmt::format_to(std::back_inserter(out), " }}\n}}\n");

    ae::file::write(filename, fmt::to_string(out), ae::file::force_compression::yes);

} // ae::chart::v3::Chart::write

// ----------------------------------------------------------------------
