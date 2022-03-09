#include "utils/log.hh"
#include "utils/timeit.hh"
#include "utils/file.hh"
#include "chart/v3/chart.hh"

// ----------------------------------------------------------------------

void ae::chart::v3::Chart::write(const std::filesystem::path& filename) const
{
    Timeit ti{fmt::format("exporting chart to {}", filename), std::chrono::milliseconds{100}};

    fmt::memory_buffer out;

    const auto put = [&out](const auto& value, auto&& condition, std::string_view key, bool comma) -> bool {
        if (condition(value)) {
            if (comma)
                fmt::format_to(std::back_inserter(out), ",");
            fmt::format_to(std::back_inserter(out), "\"{}\":\"{}\"", key, value);
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

    //  "i" |     |     |     | key-value pairs                  | chart meta information                                                                                                                                         |
    //      | "N" |     |     | str                              | user supplied name                                                                                                                                             |
    //      | "v" |     |     | str                              | virus, e.g. INFLUENZA (default, if omitted), HPV, generic, DENGE                                                                                               |
    //      | "V" |     |     | str                              | virus type and subtype, e.g. B or A(H3N2) or serotype                                                                                                          |
    //      | "l" |     |     | str                              | lab                                                                                                                                                            |
    //      | "A" |     |     | str                              | assay: HI, HINT, FRA, FOCUST REDUCTION, PRNT                                                                                                                   |
    //      | "D" |     |     | str, date YYYYMMDD.NNN           | table/assay date and number (if multiple on that day), e.g. 20160602.002                                                                                       |
    //      | "r" |     |     | str                              | RBCs species of HI assay, e.g. "turkey"                                                                                                                        |
    //      | "s" |     |     | str                              | UNUSED subset/lineage, e.g. "2009PDM"                                                                                                                                 |
    //      | "T" |     |     | str                              | table type "A[NTIGENIC]" - default, "G[ENETIC]"                                                                                                                |
    //      | "S" |     |     | array of key-value pairs         | source table info list, each entry is like "i"                                                                                                                 |

    const auto put_table_source = [put, not_empty](const TableSource& source) -> bool {
        auto comma = put(source.name(), not_empty, "N", false);
        comma = put(source.virus(), [](auto&& val) { return !val.empty() && val != ae::virus::virus_t{"INFLUENZA"}; }, "v", comma);
        comma = put(source.type_subtype(), not_empty, "V", comma);
        comma = put(source.lab(), not_empty, "l", comma);
        comma = put(source.assay(), not_empty, "A", comma);
        comma = put(source.date(), not_empty, "D", comma);
        comma = put(source.rbc_species(), not_empty, "r", comma);
        return comma;
    };

    fmt::format_to(std::back_inserter(out), "  \"i\": {{");
    auto comma = put_table_source(info());
    if (const auto& sources = info().sources(); !sources.empty()) {
        if (comma)
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
        comma = true;
    }
    fmt::format_to(std::back_inserter(out), "}},\n");


//  "a" |     |     |     | array of key-value pairs         | Antigen list                                                                                                                                                   |
//      | "N" |     |     | str, mandatory                   | name: TYPE(SUBTYPE)/[HOST/]LOCATION/ISOLATION/YEAR or CDC_ABBR NAME or UNRECOGNIZED NAME                                                                       |
//      | "a" |     |     | array of str                     | annotations that distinguish antigens (prevent from merging): ["DISTINCT"], mutation information, unrecognized extra data                                      |
//      | "D" |     |     | str, date YYYYMMDD or YYYY-MM-DD | isolation date                                                                                                                                                 |
//      | "L" |     |     | str                              | lineage: "Y[AMAGATA]" or "V[ICTORIA]"                                                                                                                          |
//      | "P" |     |     | str                              | passage, e.g. "MDCK2/SIAT1 (2016-05-12)"                                                                                                                       |
//      | "R" |     |     | str                              | reassortant, e.g. "NYMC-51C"                                                                                                                                   |
//      | "l" |     |     | array of str                     | lab ids ([lab#id]), e.g. ["CDC#2013706008"]                                                                                                                    |
//      | "A" |     |     | str                              | aligned amino-acid sequence                                                                                                                                    |
//      | "B" |     |     | str                              | aligned nucleotide sequence                                                                                                                                    |
//      | "s" |     |     | key-value  pairs                 | semantic attributes by group (see below the table)                                                                                                             |
//      | "C" |     |     | str                              | (DEPRECATED, use "s") continent: "ASIA", "AUSTRALIA-OCEANIA", "NORTH-AMERICA", "EUROPE", "RUSSIA", "AFRICA", "MIDDLE-EAST", "SOUTH-AMERICA", "CENTRAL-AMERICA" |
//      | "c" |     |     | array of str                     | (DEPRECATED, use "s") clades, e.g. ["5.2.1"]                                                                                                                   |
//      | "S" |     |     | str                              | (DEPRECATED, use "s") single letter semantic boolean attributes: R - reference, E - egg, V - current vaccine, v - previous vaccine, S - vaccine surrogate      |

    fmt::format_to(std::back_inserter(out), "  \"a\": [\n");
    fmt::format_to(std::back_inserter(out), "  ],\n");


//  "s" |     |     |     | array of key-value pairs         | Serum list                                                                                                                                                     |
//      | "N" |     |     | str, mandatory                   | name: TYPE(SUBTYPE)/[HOST/]LOCATION/ISOLATION/YEAR or CDC_ABBR NAME or UNRECOGNIZED NAME                                                                       |
//      | "a" |     |     | array of str                     | annotations that distinguish sera (prevent from merging), e.g. ["BOOSTED", "CONC 2:1", "HA-Y156T"]                                                             |
//      | "s" |     |     | str                              | serum species, e.g "FERRET"                                                                                                                                    |
//      | "L" |     |     | str                              | lineage: "Y[AMAGATA]" or "V[ICTORIA]"                                                                                                                          |
//      | "P" |     |     | str                              | passage, e.g. "MDCK2/SIAT1 (2016-05-12)"                                                                                                                       |
//      | "R" |     |     | str                              | reassortant, e.g. "NYMC-51C"                                                                                                                                   |
//      | "I" |     |     | str                              | serum id, e.g "CDC 2016-045"                                                                                                                                   |
//      | "h" |     |     | array of numbers                 | homologous antigen indices, e.g. [0]                                                                                                                           |
//      | "A" |     |     | str                              | aligned amino-acid sequence                                                                                                                                    |
//      | "B" |     |     | str                              | aligned nucleotide sequence                                                                                                                                    |
//      | "s" |     |     | key-value  pairs                 | semantic attributes by group (see below the table)                                                                                                             |
//      | "C" |     |     | str                              | (DEPRECATED, use "s") continent: "ASIA", "AUSTRALIA-OCEANIA", "NORTH-AMERICA", "EUROPE", "RUSSIA", "AFRICA", "MIDDLE-EAST", "SOUTH-AMERICA", "CENTRAL-AMERICA" |
//      | "c" |     |     | array of str                     | (DEPRECATED, use "s") clades, e.g. ["5.2.1"]                                                                                                                   |
//      | "S" |     |     | str                              | (DEPRECATED, use "s") single letter semantic boolean attributes: E - egg                                                                                       |

    fmt::format_to(std::back_inserter(out), "  \"s\": [\n");
    fmt::format_to(std::back_inserter(out), "  ],\n");

//  "t" |     |     |     | key-value pairs                  | Titers                                                                                                                                                         |
//      | "l" |     |     | array of arrays of str           | dense matrix of titers                                                                                                                                         |
//      | "d" |     |     | array of key(str)-value(str)     | sparse matrix, entry for each antigen present, key is serum index, value is titer, dont-care titers omitted                                                    |
//      | "L" |     |     | array of arrays of key-value     | layers of titers, each top level array element as in "d" or "l"                                                                                                |
    fmt::format_to(std::back_inserter(out), "  \"t\": {{\n");
    fmt::format_to(std::back_inserter(out), "  }},\n");

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
