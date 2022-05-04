#include "utils/log.hh"
#include "utils/timeit.hh"
#include "utils/file.hh"
#include "chart/v3/chart.hh"

// ----------------------------------------------------------------------

const auto not_empty = [](const auto& val) -> bool { return !val.empty(); };
const auto not_zero = [](auto val) -> bool { return val != 0; };
const auto always = [](const auto&) -> bool { return true; };

const auto put_comma = [](fmt::memory_buffer& out, bool comma) -> bool
{
    if (comma)
        fmt::format_to(std::back_inserter(out), ",");
    return true;
};

const auto put_comma_key = [](fmt::memory_buffer& out, bool comma, std::string_view key, std::string_view after_comma)
{
    put_comma(out, comma);
    if (!after_comma.empty())
        fmt::format_to(std::back_inserter(out), "{}", after_comma);
    fmt::format_to(std::back_inserter(out), "\"{}\":", key);
};

const auto put_str = [](fmt::memory_buffer& out, const auto& value, auto&& condition, std::string_view key, bool comma, std::string_view after_comma = {}) -> bool
{
    if (condition(value)) {
        put_comma_key(out, comma, key, after_comma);
        fmt::format_to(std::back_inserter(out), "\"{}\"", value);
        return true;
    }
    else
        return comma;
};

const auto put_int = [](fmt::memory_buffer& out, auto value, auto&& condition, std::string_view key, bool comma, std::string_view after_comma = {}) -> bool
{
    if (condition(value)) {
        put_comma_key(out, comma, key, after_comma);
        fmt::format_to(std::back_inserter(out), "{}", value);
        return true;
    }
    else
        return comma;
};

const auto double_to_str = [](auto value) -> std::string {
    if constexpr (std::is_convertible_v<decltype(value), double>)
        return ae::format_double(value);
    else
        return ae::format_double(*value);
};

const auto put_bool = [](fmt::memory_buffer& out, bool value, bool dflt, std::string_view key, bool comma, std::string_view after_comma = {}) -> bool {
    if (value != dflt) {
        put_comma_key(out, comma, key, after_comma);
        fmt::format_to(std::back_inserter(out), "{}", value);
        return true;
    }
    else
        return comma;
};

const auto put_optional = []<typename Value>(fmt::memory_buffer& out, const std::optional<Value>& value, std::string_view key, bool comma, std::string_view after_comma = {}) -> bool {
    if (value.has_value()) {
        put_comma_key(out, comma, key, after_comma);
        if constexpr (std::is_same_v<Value, double>)
            fmt::format_to(std::back_inserter(out), "{}", double_to_str(*value));
        else if constexpr (std::is_same_v<Value, std::string> || std::is_same_v<Value, ae::chart::v3::point_shape>)
            fmt::format_to(std::back_inserter(out), "\"{}\"", *value);
        else
            fmt::format_to(std::back_inserter(out), "{}", *value);
        return true;
    }
    else
        return comma;
};

const auto put_double = [](fmt::memory_buffer& out, const auto& value, auto&& condition, std::string_view key, bool comma, std::string_view after_comma = {}) -> bool
{
    if (condition(value)) {
        put_comma_key(out, comma, key, after_comma);
        fmt::format_to(std::back_inserter(out), "{}", double_to_str(value));
        return true;
    }
    else
        return comma;
};

const auto put_array_str = [](fmt::memory_buffer& out, const auto& value, auto&& condition, std::string_view key, bool comma) -> bool
{
    if (condition(value)) {
        put_comma(out, comma);
        fmt::format_to(std::back_inserter(out), "\"{}\":[\"{}\"]", key, fmt::join(value, "\",\""));
        return true;
    }
    else
        return comma;
};

const auto put_array_int = [](fmt::memory_buffer& out, const auto& value, auto&& condition, std::string_view key, bool comma, std::string_view after_comma = {}) -> bool
{
    if (condition(value)) {
        put_comma_key(out, comma, key, after_comma);
        fmt::format_to(std::back_inserter(out), "[{}]", fmt::join(value, ","));
        return true;
    }
    else
        return comma;
};

const auto put_array_double = [](fmt::memory_buffer& out, const auto& value, auto&& condition, std::string_view key, bool comma, std::string_view after_comma = {}) -> bool
{
    if (condition(value)) {
        put_comma_key(out, comma, key, after_comma);
        fmt::format_to(std::back_inserter(out), "[");
        bool comma2 = false;
        for (const auto en : value) {
            comma2 = put_comma(out, comma2);
            fmt::format_to(std::back_inserter(out), "{}", double_to_str(en));
        }
        fmt::format_to(std::back_inserter(out), "]");
        return true;
    }
    else
        return comma;
};

const auto put_insertions = [](fmt::memory_buffer& out, const ae::sequences::insertions_t& insertions, std::string_view key, bool comma, std::string_view after_comma = {}) -> bool
{
    if (!insertions.empty()) {
        put_comma_key(out, comma, key, after_comma);
        fmt::format_to(std::back_inserter(out), "[");
        bool comma2 = false;
        for (const auto& en : insertions) {
            comma2 = put_comma(out, comma2);
            fmt::format_to(std::back_inserter(out), "[{}, \"{}\"]", en.pos, en.insertion); // pos0_t and pos1_t are both formatted as pos1
        }
        fmt::format_to(std::back_inserter(out), "]");
        return true;
    }
    return comma;
};

const auto put_semantic = [](fmt::memory_buffer& out, const ae::chart::v3::SemanticAttributes& value, auto&& condition, std::string_view key, bool comma, std::string_view after_comma = {}) -> bool
{
    if (condition(value)) {
        put_comma_key(out, comma, key, after_comma);
        fmt::format_to(std::back_inserter(out), "{{");
        [[maybe_unused]] auto comma_inside = put_array_str(out, value.clades, not_empty, "C", false);
        fmt::format_to(std::back_inserter(out), "}}");
        return true;
    }
    else
        return comma;
};

const auto put_point_style = [](fmt::memory_buffer& out, const ae::chart::v3::PointStyle& style, bool shown_as_plus, bool comma) -> bool {
    if (style.shown().has_value()) {
        const auto shown = *style.shown();
        if (shown_as_plus)
            comma = put_bool(out, shown, !shown, "+", comma);
        else
            comma = put_bool(out, !shown, shown, "-", comma);
    }
    comma = put_str(out, style.fill(), not_empty, "F", comma);
    comma = put_str(out, style.outline(), not_empty, "O", comma);
    comma = put_optional(out, style.outline_width(), "o", comma);
    comma = put_optional(out, style.shape(), "S", comma);
    comma = put_optional(out, style.size(), "s", comma);
    comma = put_optional(out, style.rotation(), "r", comma);
    comma = put_optional(out, style.aspect(), "a", comma);
    return comma;
};

// ----------------------------------------------------------------------

    // "R" |     |     | key-value pairs       | sematic attributes based plot specifications, key: name of the style, value: style object
    //     | "z" |     | int                   | priority order when showing in GUI
    //     | "T" |     | str                   | title
    //     | "V" |     | [x, y, width, height] | viewport
    //     | "A" |     | list of objects       | modifiers to apply
    //     |     | "R" | str                   | name ("N") of another plot spec to use (inherited from), applied before adding other changes provided by this object
    //     |     | "T" | object                | {<name of semantic attribute>: <value>}
    //     |     | "A" | bool                  | true: select antigens only, false: select sera only, absent: select antigens and sera
    //     |     | "S" | str                   | shape: "C[IRCLE]" (default), "B[OX]", "T[RIANGLE]", "E[GG]", "U[GLYEGG]"
    //     |     | "F" | color, str            | fill color
    //     |     | "O" | color, str            | outline color
    //     |     | "o" | float                 | outline width
    //     |     | "s" | float                 | size, default 1.0
    //     |     | "r" | float                 | rotation in radians, default 0.0
    //     |     | "a" | float                 | aspect ratio, default 1.0
    //     |     | "-" | boolean               | hide point and its label
    //     |     | "D" | "r", "l"              | drawing order: raise, lower, absent: no change
    //     |     | "L" |     | object                | legend row
    //     |     |     | "p" | int                   | priority
    //     |     |     | "t" | str                   | text

    //     | "L"  |     |     | object                           | legend data
    //     |      | "-" |     | bool                             | hidden
    //     |      | "O" |     | [x, y]                           | offset, relative to "p"
    //     |      | "p" |     | "tl"                             | corner or center of the plot: t - top, c - center, b - bottom, l -left, r - right
    //     |      | "P" |     | [top, right, bottom, left]       | padding
    //     |      | "A" |     | object                           | plot spec of the area
    //     |      |     | "O" | Color: black                     | border
    //     |      |     | "o" | 1.0                              | outline width
    //     |      |     | "F" | Color: white                     | fill
    //     |      | "C" |     | bool                             | add counter
    //     |      | "S" |     | 10.0                             | point size
    //     |      | "T" |     | object                           | title
    //     |      |     | "t" | str                              | title text
    //     |      |     | "f" | str                              | font face
    //     |      |     | "S" | str                              | font slant: "normal" (default), "italic"
    //     |      |     | "W" | str                              | font weight: "normal" (default), "bold"
    //     |      |     | "s" | float                            | label size, default 1.0
    //     |      |     | "c" | color                            | label color, default: "black"
    //     |      | "z" |     | bool                             | show rows with zero count
    //     | "T"  |     |     | object                           | title data
    //     |      | "-" |     | bool                             | hidden
    //     |      | "O" |     | [x, y]                           | offset
    //     |      | "P" |     | [top, right, bottom, left]       | padding
    //     |      | "A" |     | object                           | plot spec of the area
    //     |      |     | "O" | Color: black                     | border
    //     |      |     | "o" | 1.0                              | outline width
    //     |      |     | "F" | Color: white                     | fill
    //     |      | "T" |     | object                           |
    //     |      |     | "t" | str                              | title text
    //     |      |     | "f" | str                              | font face
    //     |      |     | "S" | str                              | font slant: "normal" (default), "italic"
    //     |      |     | "W" | str                              | font weight: "normal" (default), "bold"
    //     |      |     | "s" | float                            | label size, default 1.0
    //     |      |     | "c" | color                            | label color, default: "black"

static inline void export_semnatic_plot_spec(fmt::memory_buffer& out, const ae::chart::v3::semantic::Styles& styles)
{
    using namespace ae::chart::v3::semantic;

    if (!styles.empty()) {
        fmt::format_to(std::back_inserter(out), ",\n  \"R\": {{");
        auto comma_R1 = false;
        for (const auto& style : styles) {
            comma_R1 = put_comma(out, comma_R1);
            fmt::format_to(std::back_inserter(out), "\n   \"{}\": {{", style.name);
            auto comma_R2 = put_int(out, style.priority, not_zero, "z", false);
            comma_R2 = put_str(out, style.title, not_empty, "T", comma_R2);
            // viewport
            if (!style.modifiers.empty()) {
                comma_R2 = put_comma(out, comma_R2);
                fmt::format_to(std::back_inserter(out), "\n      \"A\": [");
                bool comma_R3 = false;
                for (const auto& modifier : style.modifiers) {
                    comma_R3 = put_comma(out, comma_R3);
                    fmt::format_to(std::back_inserter(out), "\n        {{");
                    auto comma_R4 = put_str(out, modifier.parent, not_empty, "R", false);
                    if (!modifier.selector.empty()) {
                        comma_R4 = put_comma(out, comma_R4);
                        fmt::format_to(std::back_inserter(out), "\"T\":{{\"{}\":\"{}\"}}", modifier.selector.attribute, modifier.selector.value);
                    }
                    comma_R4 = put_point_style(out, modifier.point_style, false, comma_R4);
                    switch (modifier.order) {
                        case DrawingOrderModifier::no_change:
                            break;
                        case DrawingOrderModifier::raise:
                            put_str(out, "r", always, "D", comma_R4);
                            break;
                        case DrawingOrderModifier::lower:
                            put_str(out, "l", always, "D", comma_R4);
                            break;
                    }
                    switch (modifier.select_antigens_sera) {
                        case SelectAntigensSera::all:
                            break;
                        case SelectAntigensSera::antigens_only:
                            put_int(out, 1, always, "A", comma_R4);
                            break;
                        case SelectAntigensSera::sera_only:
                            put_int(out, 0, always, "A", comma_R4);
                            break;
                    }
                    if (!modifier.legend.text.empty()) {
                        comma_R4 = put_comma(out, comma_R4);
                        fmt::format_to(std::back_inserter(out), "\"L\":{{\"p\":{},\"t\":\"{}\"}}", modifier.legend.priority, modifier.legend.text);
                    }
                    fmt::format_to(std::back_inserter(out), "}}");
                }
                fmt::format_to(std::back_inserter(out), "\n      ]");
            }
            fmt::format_to(std::back_inserter(out), "\n   }}");
        }
        fmt::format_to(std::back_inserter(out), "\n  }}");
    }
}

// ----------------------------------------------------------------------

void ae::chart::v3::Chart::write(const std::filesystem::path& filename) const
{
    Timeit ti{fmt::format("exporting chart to {}", filename), std::chrono::milliseconds{1000}};

    fmt::memory_buffer out;

    // ----------------------------------------------------------------------

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

    const auto put_table_source = [&out](const TableSource& source) -> bool {
        auto comma = put_str(out, source.name(), not_empty, "N", false);
        comma = put_str(
            out, source.virus(), [](auto&& val) { return !val.empty() && val != ae::virus::virus_t{"INFLUENZA"}; }, "v", comma);
        comma = put_str(out, source.type_subtype(), not_empty, "V", comma);
        comma = put_str(out, source.lab(), not_empty, "l", comma);
        comma = put_str(out, source.assay(), not_empty, "A", comma);
        comma = put_str(out, source.date(), not_empty, "D", comma);
        comma = put_str(out, source.rbc_species(), not_empty, "r", comma);
        return comma;
    };

    fmt::format_to(std::back_inserter(out), "  \"i\": {{");
    auto comma1 = put_table_source(info());
    if (const auto& sources = info().sources(); !sources.empty()) {
        comma1 = put_comma(out, comma1);
        fmt::format_to(std::back_inserter(out), "\n        \"S\": [");
        auto comma2 = false;
        for (const auto& src : sources) {
            comma2 = put_comma(out, comma2);
            fmt::format_to(std::back_inserter(out), "\n         {{");
            put_table_source(src);
            fmt::format_to(std::back_inserter(out), "}}");
        }
        fmt::format_to(std::back_inserter(out), "\n        ]");
    }
    fmt::format_to(std::back_inserter(out), "}}");

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
    // "Ai" | list of [pos, "aas"]             | insertions at the aa level
    // "Bi" | list of [pos, "nucs"]            | insertions at the nuc level
    //  "T" | key-value  pairs                 | semantic attributes by group (see below the table)
    //  "C" | str                              | (DEPRECATED, use "s") continent: "ASIA", "AUSTRALIA-OCEANIA", "NORTH-AMERICA", "EUROPE", "RUSSIA", "AFRICA", "MIDDLE-EAST", "SOUTH-AMERICA",
    //  "CENTRAL-AMERICA" "c" | array of str                     | (DEPRECATED, use "s") clades, e.g. ["5.2.1"] "S" | str                              | (DEPRECATED, use "s") single letter semantic
    //  boolean attributes: R - reference, E - egg, V - current vaccine, v - previous vaccine, S - vaccine surrogate

    fmt::format_to(std::back_inserter(out), ",\n  \"a\": [");
    auto comma3 = false;
    for (const auto& antigen : antigens()) {
        comma3 = put_comma(out, comma3);
        fmt::format_to(std::back_inserter(out), "\n   {{");
        auto comma4 = put_str(out, antigen.name(), not_empty, "N", false);
        comma4 = put_array_str(out, antigen.annotations(), not_empty, "a", comma4);
        comma4 = put_str(out, antigen.reassortant(), not_empty, "R", comma4);
        comma4 = put_str(out, antigen.date(), not_empty, "D", comma4);
        comma4 = put_str(out, antigen.lineage(), not_empty, "L", comma4);
        comma4 = put_str(out, antigen.passage(), not_empty, "P", comma4);
        comma4 = put_array_str(out, antigen.lab_ids(), not_empty, "l", comma4);
        comma4 = put_semantic(out, antigen.semantic(), not_empty, "T", comma4);
        comma4 = put_str(out, antigen.aa(), not_empty, "A", comma4);  // , "\n    ");
        comma4 = put_str(out, antigen.nuc(), not_empty, "B", comma4); // , "\n    ");
        comma4 = put_insertions(out, antigen.aa_insertions(), "Ai", comma4);
        comma4 = put_insertions(out, antigen.nuc_insertions(), "Bi", comma4);
        fmt::format_to(std::back_inserter(out), "}}");
    }
    fmt::format_to(std::back_inserter(out), "\n  ]");

    // Sera
    //  "N" | str, mandatory   | name: TYPE(SUBTYPE)/[HOST/]LOCATION/ISOLATION/YEAR or CDC_ABBR NAME or UNRECOGNIZED NAME
    //  "a" | array of str     | annotations that distinguish sera (prevent from merging), e.g. ["BOOSTED", "CONC 2:1", "HA-Y156T"]
    //  "R" | str              | reassortant, e.g. "NYMC-51C"
    //  "L" | str              | lineage: "Y[AMAGATA]" or "V[ICTORIA]"
    //  "P" | str              | passage, e.g. "MDCK2/SIAT1 (2016-05-12)"
    //  "I" | str              | serum id, e.g "CDC 2016-045"
    //  "s" | str              | serum species, e.g "FERRET"
    //  "A" | str              | aligned amino-acid sequence
    //  "B" | str              | aligned nucleotide sequence
    // "Ai" | list of [pos, "aas"]             | insertions at the aa level
    // "Bi" | list of [pos, "nucs"]            | insertions at the nuc level
    //  "T" | key-value  pairs | semantic attributes by group (see below the table)
    //  "h" | array of numbers | DEPRECATED homologous antigen indices, e.g. [0]
    //  "C" | str              | (DEPRECATED, use "s") continent: "ASIA", "AUSTRALIA-OCEANIA", "NORTH-AMERICA", "EUROPE", "RUSSIA", "AFRICA", "MIDDLE-EAST", "SOUTH-AMERICA", "CENTRAL-AMERICA"
    //  "c" | array of str     | (DEPRECATED, use "s") clades, e.g. ["5.2.1"]
    //  "S" | str              | (DEPRECATED, use "s") single letter semantic boolean attributes: E - egg

    fmt::format_to(std::back_inserter(out), ",\n  \"s\": [");
    auto comma5 = false;
    for (const auto& serum : sera()) {
        comma5 = put_comma(out, comma5);
        fmt::format_to(std::back_inserter(out), "\n   {{");
        auto comma6 = put_str(out, serum.name(), not_empty, "N", false);
        comma6 = put_array_str(out, serum.annotations(), not_empty, "a", comma6);
        comma6 = put_str(out, serum.reassortant(), not_empty, "R", comma6);
        comma6 = put_str(out, serum.lineage(), not_empty, "L", comma6);
        comma6 = put_str(out, serum.passage(), not_empty, "P", comma6);
        comma6 = put_str(out, serum.serum_id(), not_empty, "I", comma6);
        comma6 = put_str(out, serum.serum_species(), not_empty, "s", comma6);
        // DEPRECATED comma6 = put_array_int(out, serum.homologous_antigens(), not_empty, "h", comma6);
        comma6 = put_semantic(out, serum.semantic(), not_empty, "T", comma6);
        comma6 = put_str(out, serum.aa(), not_empty, "A", comma6);    // , "\n    ");
        comma6 = put_str(out, serum.nuc(), not_empty, "B", comma6);   // , "\n    ");
        comma6 = put_insertions(out, serum.aa_insertions(), "Ai", comma6); // , "\n    ");
        comma6 = put_insertions(out, serum.aa_insertions(), "Ai", comma6);
        comma6 = put_insertions(out, serum.nuc_insertions(), "Bi", comma6);
        fmt::format_to(std::back_inserter(out), "}}");
    }
    fmt::format_to(std::back_inserter(out), "\n  ]");

    // Titers
    //  "l" | array of arrays of str       | dense matrix of titers
    //  "d" | array of key(str)-value(str) | sparse matrix, entry for each antigen present, key is serum index, value is titer, dont-care titers omitted
    //  "L" | array of arrays of key-value | layers of titers, each top level array element as in "d" or "l"

    const auto put_sparse = [&out, this](const Titers::sparse_t& data, std::string_view indent) {
        for (const auto ag_no : titers().number_of_antigens()) {
            if (ag_no != antigen_index{0})
                fmt::format_to(std::back_inserter(out), ",");
            fmt::format_to(std::back_inserter(out), "\n{}{{", indent);
            bool comma = false;
            for (const auto& [sr_no, titer] : data[*ag_no]) {
                comma = put_comma(out, comma);
                fmt::format_to(std::back_inserter(out), "\"{}\":\"{}\"", sr_no, titer);
            }
            fmt::format_to(std::back_inserter(out), "}}");
        }
    };

    fmt::format_to(std::back_inserter(out), ",\n  \"t\": {{");
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
        put_sparse(titers().sparse_titers(), "    ");
        fmt::format_to(std::back_inserter(out), "\n   ]");
    }
    if (titers().number_of_layers() > layer_index{1}) {
        fmt::format_to(std::back_inserter(out), ",\n   \"L\": [");
        for (const auto layer_no : titers().number_of_layers()) {
            if (layer_no != layer_index{0})
                fmt::format_to(std::back_inserter(out), ",");
            fmt::format_to(std::back_inserter(out), "\n    [");
            put_sparse(titers().layer(layer_no), "     ");
            fmt::format_to(std::back_inserter(out), "\n    ]");
        }
        fmt::format_to(std::back_inserter(out), "\n   ]");
    }
    fmt::format_to(std::back_inserter(out), "\n  }}");

    //  "C" |     |     |     | array of floats                  | forced column bases for a new projections
    // stored with sera
    std::vector<double> forced_column_bases(*sera().size(), 0.0);
    bool forced_column_bases_present = false;
    for (const auto sr_no : sera().size()) {
        if (const auto fcb = sera()[sr_no].forced_column_basis(); fcb.has_value()) {
            forced_column_bases[*sr_no] = *fcb;
            forced_column_bases_present = true;
        }
    }
    put_array_double(out,
        forced_column_bases, [forced_column_bases_present](auto&&) { return forced_column_bases_present; }, "C", true, "\n  ");

    // Projections
    // "c" | str (or any)              | comment
    // "s" | float                     | stress
    // "m" | str                       | minimum column basis, "none" (default), "1280"
    // "d" | boolean                   | dodgy_titer_is_regular, false is default
    // "C" | array of floats           | forced column bases
    // "t" | array of floats           | transformation matrix
    // "U" | array of integers         | list of indices of unmovable points (antigen/serum attribute for stress evaluation)
    // "D" | array of integers         | list of indices of disconnected points (antigen/serum attribute for stress evaluation)
    // "u" | array of integers         | list of indices of points unmovable in the last dimension (antigen/serum attribute for stress evaluation)
    // "l" | array of arrays of floats | layout, if point is disconnected: empty list or ?[NaN, NaN]
    // "i" | integer                   | UNUSED number of iterations?
    // "g" | array of floats           | antigens_sera_gradient_multipliers, float for each point
    // "f" | array of floats           | avidity adjusts (antigens_sera_titers_multipliers), float for each point
    // "e" | float                     | stress_diff_to_stop

    if (!projections().empty()) {
        fmt::format_to(std::back_inserter(out), ",\n  \"P\": [");
        auto comma7 = false;
        for (const auto& projection : projections()) {
            comma7 = put_comma(out, comma7);
            fmt::format_to(std::back_inserter(out), "\n   {{");
            auto comma8 = put_double(
                out, projection.stress(), [](double stress) { return !std::isnan(stress) && stress >= 0.0; }, "s", false);
            comma8 = put_str(
                out, projection.minimum_column_basis(), [](const auto& mcb) { return !mcb.is_none(); }, "m", comma8);
            comma8 = put_str(out, projection.comment(), not_empty, "c", comma8);
            comma8 = put_bool(out, projection.dodgy_titer_is_regular() == dodgy_titer_is_regular_e::yes, false, "d", comma8);
            comma8 = put_array_double(out, projection.forced_column_bases(), not_empty, "C", comma8);
            comma8 = put_array_double(out, projection.transformation().as_vector(), not_empty, "t", comma8, "\n    ");
            comma8 = put_array_int(out, projection.unmovable(), not_empty, "U", comma8, "\n    ");
            comma8 = put_array_int(out, projection.disconnected(), not_empty, "D", comma8, "\n    ");
            comma8 = put_array_int(out, projection.unmovable_in_the_last_dimension(), not_empty, "u", comma8, "\n    ");

            const auto& layout = projection.layout();
            comma8 = put_comma(out, comma8);
            fmt::format_to(std::back_inserter(out), "\n    \"l\": [");
            for (const auto point_no : layout.number_of_points()) {
                if (point_no != point_index{0})
                    fmt::format_to(std::back_inserter(out), ",");
                fmt::format_to(std::back_inserter(out), "\n     [");
                if (const auto point = layout[point_no]; point.exists()) {
                    bool comma9 = false;
                    for (const auto coord : point) {
                        comma9 = put_comma(out, comma9);
                        fmt::format_to(std::back_inserter(out), "{}", ae::format_double(coord));
                    }
                }
                fmt::format_to(std::back_inserter(out), "]");
            }
            fmt::format_to(std::back_inserter(out), "\n    ]");

            // "g"
            // "f"
            // "e"
            fmt::format_to(std::back_inserter(out), "\n   }}");
        }
        fmt::format_to(std::back_inserter(out), "\n  ]");
    }

    // ----------------------------------------------------------------------

    export_semnatic_plot_spec(out, styles());

    // legacy lispmds stype plot specification
    //  "d" |     |     | array of integers       | drawing order, point indices
    //  "E" |     |     | key-value pairs         | error line positive, default: {"c": "blue"}
    //  "e" |     |     | key-value pairs         | error line negative, default: {"c": "red"}
    //  "g" |     |     | ?                       | ? grid data
    //  "P" |     |     | array of key-value pairs| list of plot styles
    //      | "+" |     | boolean                 | if point is shown, default is true, disconnected points are usually not shown and having NaN coordinates in layout
    //      | "F" |     | color, str              | fill color: #FF0000 or T[RANSPARENT] or color name (red, green, blue, etc.), default is transparent
    //      | "O" |     | color, str              | outline color: #000000 or T[RANSPARENT] or color name (red, green, blue, etc.), default is black
    //      | "o" |     | float                   | outline width, default 1.0
    //      | "S" |     | str                     | shape: "C[IRCLE]" (default), "B[OX]", "T[RIANGLE]", "E[GG]", "U[GLYEGG]"
    //      | "s" |     | float                   | size, default 1.0
    //      | "r" |     | float                   | rotation in radians, default 0.0
    //      | "a" |     | float                   | aspect ratio, default 1.0
    //      | "l" |     | key-value pairs         | label style
    //      |     | "+" | boolean                 | if label is shown
    //      |     | "p" | list of two floats      | label position (2D only), list of two doubles, default is [0, 1] means under point
    //      |     | "t" | str                     | label text if forced by user
    //      |     | "f" | str                     | font face
    //      |     | "S" | str                     | font slant: "normal" (default), "italic"
    //      |     | "W" | str                     | font weight: "normal" (default), "bold"
    //      |     | "s" | float                   | label size, default 1.0
    //      |     | "c" | color, str              | label color, default: "black"
    //      |     | "r" | float                   | label rotation, default 0.0
    //      |     | "i" | float                   | addtional interval between lines as a fraction of line height, default 0.2
    //  "p" |     |     | array of integers       | index in "P" for each point, antigens followed by sera
    //  "l" |     |     | array of integers       | ? for each procrustes line, index in the "L" list
    //  "L" |     |     | array                   | ? list of procrustes lines styles "s"
    //      |     |     | array of integers       | list of point indices for point shown on all maps in the time series
    //  "t" |     |     | key-value pairs         | ? title style

    if (const auto& plot_spec = legacy_plot_spec(); !plot_spec.empty()) {
        fmt::format_to(std::back_inserter(out), ",\n  \"p\": {{");
        auto comma10 = put_array_int(out, plot_spec.drawing_order(), not_empty, "d", false, "\n   ");
        comma10 = put_array_int(out, plot_spec.style_for_point(), not_empty, "p", comma10, "\n   ");
        if (!plot_spec.styles().empty()) {
            comma10 = put_comma(out, comma10);
            fmt::format_to(std::back_inserter(out), "\n   \"P\": [");
            bool comma11 = false;
            for (const auto& style : plot_spec.styles()) {
                comma11 = put_comma(out, comma11);
                fmt::format_to(std::back_inserter(out), "\n    {{");
                auto comma12 = put_point_style(out, style, true, false);
                if (const auto& label_style = style.label(); !label_style.empty()) {
                    comma12 = put_comma(out, comma12);
                    fmt::format_to(std::back_inserter(out), "\"l\":{{");
                    auto comma13 = put_optional(out, style.label_text(), "t", false);
                    comma13 = put_bool(out, label_style.shown, true, "+", comma13);
                    // "p" offset
                    comma13 = put_str(
                        out, label_style.color, [](const auto& color) { return color != Color{"black"}; }, "c", comma13);
                    comma13 = put_str(
                        out, label_style.style.slant, [](const auto& slant) { return slant != ae::draw::v2::font_slant_t{"normal"}; }, "S", comma13);
                    comma13 = put_str(
                        out, label_style.style.weight, [](const auto& weight) { return weight != ae::draw::v2::font_weight_t{"normal"}; }, "W", comma13);
                    comma13 = put_str(out, label_style.style.font_family, not_empty, "f", comma13);
                    //      |     | "s" | float                   | label size, default 1.0
                    //      |     | "r" | float                   | label rotation, default 0.0
                    comma13 = put_double(
                        out, label_style.interline, [](double interline) { return !float_equal(interline, 0.2); }, "i", comma13);
                    fmt::format_to(std::back_inserter(out), "}}");
                }

                fmt::format_to(std::back_inserter(out), "}}");
            }
            fmt::format_to(std::back_inserter(out), "\n   ]");
        }
        fmt::format_to(std::back_inserter(out), "\n  }}");
    }

    // -----+-----+-----+-----+----------------------------------+---------------------------------------------------------------------------------------------------------------------
    //  "x" |     |     |     | key-value pairs                  | extensions not used by acmacs
    // -----+-----+-----+-----+----------------------------------+---------------------------------------------------------------------------------------------------------------------

    // fmt::format_to(std::back_inserter(out), ",\n  \"x\": {{");
    // fmt::format_to(std::back_inserter(out), "\n  }}");

    fmt::format_to(std::back_inserter(out), "\n }}\n}}\n");

    ae::file::write(filename, fmt::to_string(out), ae::file::force_compression::yes);

} // ae::chart::v3::Chart::write

// ----------------------------------------------------------------------
