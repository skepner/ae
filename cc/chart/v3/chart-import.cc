#include "ext/simdjson.hh"
#include "ext/from_chars.hh"
#include "utils/log.hh"
#include "utils/timeit.hh"
#include "chart/v3/chart.hh"

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
            case 's':
                // target.lineage(ae::chart::v3::Lineage{static_cast<std::string_view>(value)});
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

inline void read_semantic_attributes(ae::chart::v3::SemanticAttributes& target, ::simdjson::ondemand::object source)
{
    for (auto field : source) {
        if (const std::string_view key = field.unescaped_key(); key == "C") { // clades
            for (auto ann : field.value().get_array())
                target.clades.insert_if_not_present(static_cast<std::string_view>(ann));
        }
        else {
            unhandled_key({"c", "a/s", "T", key});
        }
   }
}

// ----------------------------------------------------------------------

inline ae::sequences::insertions_t read_inserions(::simdjson::ondemand::array source) {
    ae::sequences::insertions_t insertions;
    for (::simdjson::ondemand::array en : source) {
        auto it = en.begin();
        const ae::sequences::pos1_t pos{static_cast<uint64_t>(*it)};
        ++it;
        insertions.push_back(ae::sequences::insertion_t{pos, std::string{static_cast<std::string_view>(*it)}});
    }
    return insertions;
}

// ----------------------------------------------------------------------

inline bool read_antigen_serum(ae::chart::v3::AntigenSerum& target, std::string_view key, ::simdjson::ondemand::value value)
{
    bool handled{true};
    if (key.size() == 1) {
        switch (key[0]) {
            case 'N': // str, mandatory                   | name: TYPE(SUBTYPE)/[HOST/]LOCATION/ISOLATION/YEAR or CDC_ABBR NAME or UNRECOGNIZED NAME
                target.name(ae::virus::Name{static_cast<std::string_view>(value)});
                break;
            case 'a': // array of str                     | annotations that distinguish antigens (prevent from merging): ["DISTINCT"], mutation information, unrecognized extra data
                for (auto ann : value.get_array())
                    target.annotations().insert_if_not_present(static_cast<std::string_view>(ann));
                break;
            case 'L': // str                              | lineage: "Y[AMAGATA]" or "V[ICTORIA]"
                target.lineage(ae::sequences::lineage_t{static_cast<std::string_view>(value)});
                break;
            case 'P': // str                              | passage, e.g. "MDCK2/SIAT1 (2016-05-12)"
                target.passage(ae::virus::Passage{static_cast<std::string_view>(value)});
                break;
            case 'R': // str                              | reassortant, e.g. "NYMC-51C"
                target.reassortant(ae::virus::Reassortant{static_cast<std::string_view>(value)});
                break;
            case 'A': // str                              | aligned amino-acid sequence
                target.aa(ae::sequences::sequence_aa_t{static_cast<std::string_view>(value)});
                break;
            case 'B': // str                              | aligned nucleotide sequence
                target.nuc(ae::sequences::sequence_nuc_t{static_cast<std::string_view>(value)});
                break;
            case 'T': // key-value  pairs                 | semantic attributes by group (see below the table)
                read_semantic_attributes(target.semantic(), value.get_object());
                break;
            case 'C': // str                              | (DEPRECATED, use "s") continent: "ASIA", "AUSTRALIA-OCEANIA", "NORTH-AMERICA", "EUROPE", "RUSSIA", "AFRICA", "MIDDLE-EAST", "SOUTH-AMERICA", "CENTRAL-AMERICA"
                // handled = false;
                break;
            case 'c': // array of str                     | (DEPRECATED, use "s") clades, e.g. ["5.2.1"]
                // handled = false;
                break;
            case 'S': // str                              | (DEPRECATED, use "s") single letter semantic boolean attributes: R - reference, E - egg, V - current vaccine, v - previous vaccine, S - vaccine surrogate
                // handled = false;
                break;
            default:
                handled = false;
                break;
            }
    }
    else if (key == "Ai") { // insertions at the aa level
        target.aa_insertions(read_inserions(value.get_array()));
    }
    else if (key == "Bi") { // insertions at the nucleotide level
        target.nuc_insertions(read_inserions(value.get_array()));
    }
    else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
        handled = false;
    return handled;
}

// ----------------------------------------------------------------------

inline void read_antigens(ae::chart::v3::Antigens& target, ::simdjson::ondemand::array source)
{
    for (auto en : source) {
        auto& antigen = target.add();
        for (auto field : en.get_object()) {
            if (const std::string_view key = field.unescaped_key(); !read_antigen_serum(antigen, key, field.value())) {
                if (key == "D") { // str, date YYYYMMDD or YYYY-MM-DD | isolation date
                    antigen.date(ae::chart::v3::Date{static_cast<std::string_view>(field.value())});
                }
                else if (key == "l") { // array of str | lab ids ([lab#id]), e.g. ["CDC#2013706008"]
                    for (auto lab_id : field.value().get_array())
                        antigen.lab_ids().insert_if_not_present(static_cast<std::string_view>(lab_id));
                }
                else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
                    unhandled_key({"c", "a", key});
            }
        }
    }
}

// ----------------------------------------------------------------------

inline void read_sera(ae::chart::v3::Sera& target, ::simdjson::ondemand::array source)
{
    for (auto en : source) {
        auto& serum = target.add();
        for (auto field : en.get_object()) {
            if (const std::string_view key = field.unescaped_key(); !read_antigen_serum(serum, key, field.value())) {
                if (key == "s") { // str | serum species, e.g "FERRET"
                    serum.serum_species(ae::chart::v3::SerumSpecies{static_cast<std::string_view>(field.value())});
                }
                else if (key == "I") { // str | serum id, e.g "CDC 2016-045"
                    serum.serum_id(ae::chart::v3::SerumId{static_cast<std::string_view>(field.value())});
                }
                else if (key == "h") { // array of numbers | homologous antigen indices, e.g. [0]
                    // deprecated, ignored // serum.homologous_antigens().push_back(ae::antigen_index{static_cast<uint64_t>(field.value())});
                }
                else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
                    unhandled_key({"c", "s", key});
            }
        }
    }
}

// ----------------------------------------------------------------------

// returns number of sera (max serum index + 1)
inline ae::serum_index read_sparse(ae::chart::v3::Titers::sparse_t& target, ::simdjson::ondemand::array source)
{
    ae::serum_index number_of_sera{0};
    // ae::antigen_index ag_no{0};
    for (auto row : source) {
        auto& target_row = target.emplace_back();
        for (auto source_entry : row.get_object()) {
            const ae::serum_index sr_no{ae::from_chars<ae::serum_index::value_type>(static_cast<std::string_view>(source_entry.unescaped_key()))};
            target_row.emplace_back(sr_no, ae::chart::v3::Titer{static_cast<std::string_view>(source_entry.value())});
            number_of_sera = std::max(number_of_sera, sr_no + ae::serum_index{1});
        }
        // row is perhaps not sorted by sr_no (produced by ae.chart_v2), sort it
        std::sort(target_row.begin(), target_row.end(), [](const auto& e1, const auto& e2) { return e1.first < e2.first; });
    }
    return number_of_sera;
}

// ----------------------------------------------------------------------

inline void read_titers(ae::chart::v3::Titers& target, ::simdjson::ondemand::object source)
{
    for (auto field : source) {
        if (const std::string_view key = field.unescaped_key(); key == "l") { // dense matrix
            auto& titers = target.create_dense_titers();
            ae::antigen_index ag_no{0};
            for (auto row : field.value().get_array()) {
                ae::serum_index sr_no{0};
                for (auto col : row.get_array()) {
                    titers.emplace_back(static_cast<std::string_view>(col.value()));
                    ++sr_no;
                }
                if (target.number_of_sera() == ae::serum_index{0})
                    target.number_of_sera(sr_no);
                else if (target.number_of_sera() != sr_no)
                    throw ae::chart::v3::Error{"invalid number of sera in dense matrix for antigen {}: {} (expected: {})", ag_no, sr_no, target.number_of_sera()};
                ++ag_no;
            }
        }
        else if (key == "d") {  // sparse matrix
            const auto number_of_sera = read_sparse(target.create_sparse_titers(), field.value().get_array());
            target.number_of_sera(number_of_sera);
        }
        else if (key == "L") {  // layers (sparse matrices)
            for (auto source_layer : field.value().get_array())
                read_sparse(target.layers().emplace_back(), source_layer.get_array());
        }
        else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
            unhandled_key({"c", "t", key});
    }
}

// ----------------------------------------------------------------------

inline void read_layout(ae::chart::v3::Layout& layout, ::simdjson::ondemand::array source)
{
    size_t point_no{0};
    for (auto point : source) {
        size_t dim{0};
        for (const double coord : point.get_array()) {
            layout.add_value(coord);
            ++dim;
        }
        if (dim == 0) { // empty array, set coords to nan
            if (layout.number_of_dimensions() != ae::number_of_dimensions_t{0}) {
                for ([[maybe_unused]] const auto _ : layout.number_of_dimensions())
                    layout.add_value(ae::chart::v3::point_coordinates::nan);
            }
            else
                throw std::runtime_error{AD_FORMAT("first array in the layout empty, cannot handle it")};
        }
        else if (point_no == 0)
            layout.number_of_dimensions(ae::number_of_dimensions_t{dim});
        else if (layout.number_of_dimensions() != ae::number_of_dimensions_t{dim})
            throw std::runtime_error{AD_FORMAT("invalid number of dimensions for point {}: {}, expected: {}", point_no, dim, layout.number_of_dimensions())};
        ++point_no;
    }
}

// ----------------------------------------------------------------------

inline void read_projections(ae::chart::v3::Projections& target, ::simdjson::ondemand::array source)
{
    for (auto source_proj : source) {
        auto& projection = target.add();
        for (auto field : source_proj.get_object()) {
            if (const std::string_view key = field.unescaped_key(); key == "c") {
                projection.comment(field.value());
            }
            else if (key == "l") { // layout, if point is disconnected: emtpy list or ?[NaN, NaN]
                read_layout(projection.layout(), field.value().get_array());
            }
            else if (key == "i") { // UNUSED number of iterations
            }
            else if (key == "s") { //
                projection.stress(static_cast<double>(field.value()));
            }
            else if (key == "m") { // minimum column basis, "none" (default), "1280"
                projection.minimum_column_basis(field.value());
            }
            else if (key == "C") { // forced column bases
                for (double cb : field.value().get_array())
                    projection.forced_column_bases().add(cb);
            }
            else if (key == "t") { // transformation matrix
                std::vector<double> vals;
                for (const double val : field.value().get_array())
                    vals.push_back(val);
                projection.transformation().set(vals.begin(), vals.size());
            }
            else if (key == "g") { // antigens_sera_gradient_multipliers, float for each point
                unhandled_key({"c", "p", key});
            }
            else if (key == "f") { // avidity adjusts (antigens_sera_titers_multipliers), float for each point
                for (const double val : field.value().get_array())
                    projection.avidity_adjusts_access().push_back(val);
            }
            else if (key == "d") { // dodgy_titer_is_regular, false is default
                projection.dodgy_titer_is_regular(static_cast<bool>(field.value()) ? ae::chart::v3::dodgy_titer_is_regular_e::yes : ae::chart::v3::dodgy_titer_is_regular_e::no);
            }
            else if (key == "e") { // stress_diff_to_stop
                unhandled_key({"c", "p", key});
            }
            else if (key == "U") { // list of indices of unmovable points (antigen/serum attribute for stress evaluation)
                for (const uint64_t val : field.value().get_array())
                    projection.unmovable().push_back(ae::point_index{val});
            }
            else if (key == "D") { // list of indices of disconnected points (antigen/serum attribute for stress evaluation)
                for (const uint64_t val : field.value().get_array())
                    projection.disconnected().push_back(ae::point_index{val});
            }
            else if (key == "u") { // list of indices of points unmovable in the last dimension (antigen/serum attribute for stress evaluation)
                for (const uint64_t val : field.value().get_array())
                    projection.unmovable_in_the_last_dimension().push_back(ae::point_index{val});
            }
            else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
                unhandled_key({"c", "p", key});
        }
    }
}

// ----------------------------------------------------------------------

inline void read_offset(ae::draw::v2::offset_t& target, ::simdjson::ondemand::array source)
{
    auto it = source.begin();
    target.x = *it;
    ++it;
    target.y = *it;
}

// ----------------------------------------------------------------------

inline bool read_text_and_offset(ae::draw::v2::text_and_offset& target, std::string_view key, ::simdjson::ondemand::value value)
{
    if (key == "-") {
        target.shown = !value;
    }
    else if (key == "p") {
        read_offset(target.offset, value);
    }
    else if (key == "t") {
        target.text = std::string{static_cast<std::string_view>(value)};
    }
    else if (key == "s") {
        target.size = static_cast<double>(value);
    }
    else if (key == "c") {
        target.color = ae::chart::v3::Color{value};
    }
    else if (key == "S") {
        target.slant = ae::draw::v2::font_slant_t{value};
    }
    else if (key == "W") {
        target.weight = ae::draw::v2::font_weight_t{value};
    }
    else if (key == "f") {
        target.font_family = static_cast<std::string_view>(value);
    }
    else if (key == "r") {
        target.rotation = ae::draw::v2::Rotation{value};
    }
    else if (key == "i") {
        target.interline = static_cast<double>(value);
    }
    else
        return false;
    return true;
}

// ----------------------------------------------------------------------

// returns if key/value was processed
inline bool read_point_style_field(ae::chart::v3::PointStyle& target, std::string_view key, ::simdjson::simdjson_result<::simdjson::ondemand::value> value)
{
    if (key.size() == 1) {
        switch (key[0]) {
            case '+': // if point shown, default is true, disconnected points are usually not shown and having NaN coordinates in layout
                target.shown(value);
                break;
            case '-': // if point hidden, default is false, disconnected points are usually not shown and having NaN coordinates in layout
                target.shown(!value);
                break;
            case 'F': // fill color: #FF0000 or T[RANSPARENT] or color name (red, green, blue, etc.), default is transparent
                target.fill(ae::chart::v3::Color{value});
                break;
            case 'O': // outline color: #000000 or T[RANSPARENT] or color name (red, green, blue, etc.), default is black
                target.outline(ae::chart::v3::Color{value});
                break;
            case 'o': // outline width, default 1.0
                target.outline_width(value);
                break;
            case 'S': // shape: "C[IRCLE]" (default), "B[OX]", "T[RIANGLE]", "E[GG]", "U[GLYEGG]"
                target.shape(ae::chart::v3::point_shape{value});
                break;
            case 's': //  size, default 1.0
                target.size(value);
                break;
            case 'r': // rotation in radians, default 0.0
                target.rotation(ae::draw::v2::Rotation{value});
                break;
            case 'a': //  aspect ratio, default 1.0
                target.aspect(ae::draw::v2::Aspect{value});
                break;
            case 'l': // label style
                for (auto label_field : value.get_object()) {
                    if (const std::string_view label_key = label_field.unescaped_key(); !read_text_and_offset(target.label(), label_key, label_field.value()))
                        unhandled_key({"c", "p", "P", "l", label_key});
                }
                break;
            default:
                return false;
        }
        return true;
    }
    else
        return false;
}

// ----------------------------------------------------------------------

inline void read_legacy_point_style(ae::chart::v3::PointStyle& target, ::simdjson::ondemand::object source)
{
    for (auto field : source) {
        const std::string_view key = field.unescaped_key();
        auto value = field.value();
        if (!read_point_style_field(target, key, value)) {
            if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
                unhandled_key({"c", "p", "P", key});
        }
    }
}

// ----------------------------------------------------------------------

inline void read_legacy_plot_specification(ae::chart::v3::legacy::PlotSpec& target, ::simdjson::ondemand::object source)
{
    for (auto field : source) {
        if (const std::string_view key = field.unescaped_key(); key == "d") {  // drawing order, point indices
            for (const uint64_t val : field.value().get_array())
                target.drawing_order().push_back(ae::point_index{val});
        }
        else if (key == "E") { // error line positive, default: {"c": "blue"}
            for (auto fld2 : field.value().get_object()) {
                if (const std::string_view k2 = fld2.unescaped_key(); k2 == "c")
                    target.error_line_positive_color(ae::draw::v2::Color{fld2.value()});
                else
                    unhandled_key({"c", "p", key, k2});
            }
        }
        else if (key == "e") {  // error line negative, default: {"c": "red"}
            for (auto fld2 : field.value().get_object()) {
                if (const std::string_view k2 = fld2.unescaped_key(); k2 == "c")
                    target.error_line_negative_color(ae::draw::v2::Color{fld2.value()});
                else
                    unhandled_key({"c", "p", key, k2});
            }
        }
        else if (key == "P") {  // list of plot styles
            for (auto entry : field.value().get_array())
                read_legacy_point_style(target.styles().emplace_back(), entry.get_object());
        }
        else if (key == "p") {  // index in "P" for each point, antigens followed by sera
            for (const uint64_t val : field.value().get_array())
                target.style_for_point().push_back(val);
        }
        else if (key == "L") {  // list of procrustes lines styles
            unhandled_key({"c", "p", key});
        }
        else if (key == "l") {  // for each procrustes line, index in the "L" list
            unhandled_key({"c", "p", key});
        }
        else if (key == "s") {  // list of point indices for point shown on all maps in the time series
            unhandled_key({"c", "p", key});
        }
        else if (key == "t") {  // title style
            unhandled_key({"c", "p", key});
        }
        else if (key == "g") {  // grid data
            unhandled_key({"c", "p", key});
        }
        else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
            unhandled_key({"c", "p", key});
    }

} // read_legacy_plot_specification

// ----------------------------------------------------------------------

inline void read_semantic_plot_style_modifier(ae::chart::v3::semantic::StyleModifier& target, ::simdjson::ondemand::object source)
{
    for (auto field : source) {
        const std::string_view key = field.unescaped_key();
        auto value = field.value();
        if (read_point_style_field(target.point_style, key, value)) {
            // pass
        }
        else if (key == "R") { // reference
            target.parent.assign(static_cast<std::string_view>(value));
        }
        else if (key == "T") { // selector: {"C":"3C.2a"}
            for (auto selector_field : value.get_object()) {
                if (target.selector.empty())
                    target.selector = ae::chart::v3::semantic::Selector{.attribute = std::string{static_cast<std::string_view>(selector_field.unescaped_key())},
                                                                              .value = std::string{static_cast<std::string_view>(selector_field.value())}};
                else
                    AD_WARNING("[chart semantic plot spec]: unhandled additional modifier selector {}: {}", static_cast<std::string_view>(selector_field.unescaped_key()), selector_field.value());
            }
        }
        else if (key == "A") { // antigens only or sera only (bool or int)
            bool val;
            if (value.get(val)) // not bool
                val = static_cast<int64_t>(value) != 0;
            if (val)
                target.select_antigens_sera = ae::chart::v3::semantic::SelectAntigensSera::antigens_only;
            else
                target.select_antigens_sera = ae::chart::v3::semantic::SelectAntigensSera::sera_only;
        }
        else if (key == "D") { // drawing order: raise, lower, absent: no change
            if (const std::string_view val = value; val == "r")
                target.order = ae::chart::v3::semantic::DrawingOrderModifier::raise;
            else if (val == "l")
                target.order = ae::chart::v3::semantic::DrawingOrderModifier::lower;
            else
                AD_WARNING("[chart semantic plot spec]: unrecognized drawing order modiifer \"{}\", \"r\" or \"l\" expected", val);
        }
        else if (key == "L") { // legend row
            for (auto legend_field : value.get_object()) {
                if (const std::string_view legend_field_key = legend_field.unescaped_key(); legend_field_key == "p") // priority
                    target.legend.priority = static_cast<int>(static_cast<int64_t>(legend_field.value()));
                else if (legend_field_key == "t") // text
                    target.legend.text.assign(static_cast<std::string_view>(legend_field.value()));
                else
                    AD_WARNING("[chart semantic plot spec]: unrecognized legend row field \"{}\"", legend_field_key);
            }
        }
        else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
            unhandled_key({"c", "R", "<name>", "A", "[index]", key});
    }
}

// ----------------------------------------------------------------------

inline ae::chart::v3::semantic::box_t read_semantic_box(::simdjson::ondemand::object source)
{
    ae::chart::v3::semantic::box_t box;
    for (auto field : source) {
        if (const std::string_view key = field.unescaped_key(); key == "o") {
            box.set_origin(field.value());
        }
        else if (key == "p") {
            ae::chart::v3::semantic::padding_t padding{};
            for (auto field_padding : field.value().get_object()) {
                if (const std::string_view key_padding = field_padding.unescaped_key(); key_padding == "t") {
                    padding[0] = field_padding.value();
                }
                else if (key_padding == "r") {
                    padding[1] = field_padding.value();
                }
                else if (key_padding == "b") {
                    padding[2] = field_padding.value();
                }
                else if (key_padding == "l") {
                    padding[3] = field_padding.value();
                }
                else if (key_padding[0] != '?' && key_padding[0] != ' ' && key_padding[0] != '_')
                    unhandled_key({"c", "R", "<name>", "T", "B", "p", key_padding});
            }
            box.padding = padding;
        }
        else if (key == "O") {
            box.offset = ae::chart::v3::semantic::offset_t{};
            auto offset = field.value().get_array();
            auto it = offset.begin();
            (*box.offset)[0] = *it;
            ++it;
            (*box.offset)[1] = *it;
        }
        else if (key == "B") {
            box.border_color = ae::chart::v3::semantic::color_t{static_cast<std::string_view>(field.value())};
        }
        else if (key == "W") {
            box.border_width = field.value();
        }
        else if (key == "F") {
            box.background_color = ae::chart::v3::semantic::color_t{static_cast<std::string_view>(field.value())};
        }
        else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
            unhandled_key({"c", "R", "<name>", "T", "B", key});
    }
    return box;
}

inline ae::chart::v3::semantic::text_t read_semantic_text(::simdjson::ondemand::object source)
{
    ae::chart::v3::semantic::text_t text;
    for (auto field : source) {
        if (const std::string_view key = field.unescaped_key(); key == "t") {
            text.text = std::string{static_cast<std::string_view>(field.value())};
        }
        else if (key == "f") {
            text.set_font_face(field.value());
        }
        else if (key == "S") {
            text.set_font_slant(field.value());
        }
        else if (key == "W") {
            text.set_font_weight(field.value());
        }
        else if (key == "s") {
            text.font_size = field.value();
        }
        else if (key == "c") {
            text.color = ae::chart::v3::semantic::color_t{static_cast<std::string_view>(field.value())};
        }
        else if (key == "i") {
            text.interline = field.value();
        }
        else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
            unhandled_key({"c", "R", "<name>", "T", "T", key});
    }
    return text;
}

inline void read_semantic_plot_style_title(ae::chart::v3::semantic::Title& target, ::simdjson::ondemand::object source)
{
    for (auto field : source) {
        if (const std::string_view key = field.unescaped_key(); key == "-") {
            target.shown = !field.value();
        }
        else if (key == "B") {
            target.box = read_semantic_box(field.value());
        }
        else if (key == "T") {
            target.text = read_semantic_text(field.value());
        }
        else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
            unhandled_key({"c", "R", "<name>", "T", key});
    }
}

// ----------------------------------------------------------------------

inline void read_semantic_plot_style_legend(ae::chart::v3::semantic::Legend& target, ::simdjson::ondemand::object source)
{
    for (auto field : source) {
        if (const std::string_view key = field.unescaped_key(); key == "-") {
            target.shown = !field.value();
        }
        else if (key == "C") {
            target.add_counter = field.value();
        }
        else if (key == "S") {
            target.point_size = field.value();
        }
        else if (key == "z") {
            target.show_rows_with_zero_count = field.value();
        }
        else if (key == "B") {
            target.box = read_semantic_box(field.value());
        }
        else if (key == "t") {
            target.row_style = read_semantic_text(field.value());
        }
        else if (key == "T") {
            target.title = read_semantic_text(field.value());
        }
        else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
            unhandled_key({"c", "R", "<name>", "L", key});
    }
}

// ----------------------------------------------------------------------

inline void read_semantic_plot_style(ae::chart::v3::semantic::Style& target, ::simdjson::ondemand::object source)
{
    for (auto field : source) {
        if (const std::string_view key = field.unescaped_key(); key == "z") {
            target.priority = static_cast<int>(static_cast<int64_t>(field.value()));
        }
        else if (key == "t") {
            target.title.assign(static_cast<std::string_view>(field.value()));
        }
        else if (key == "V") { // viewport
            target.viewport = ae::draw::v2::Viewport{};
            auto src = field.value().get_array();
            auto it = src.begin();
            target.viewport->x = *it;
            ++it;
            target.viewport->y = *it;
            ++it;
            target.viewport->width = *it;
            ++it;
            target.viewport->height = it == src.end() ? static_cast<double>(target.viewport->width) : *it;
        }
        else if (key == "A") {  // apply
            for (auto apply_field : field.value().get_array())
                read_semantic_plot_style_modifier(target.modifiers.emplace_back(), apply_field);
        }
        else if (key == "L") {  // legend
            read_semantic_plot_style_legend(target.legend, field.value());
        }
        else if (key == "T") {  // title
            read_semantic_plot_style_title(target.plot_title, field.value());
        }
        else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
            unhandled_key({"c", "R", target.name, key});
    }
}

// ----------------------------------------------------------------------

inline void read_semantic_plot_specification(ae::chart::v3::semantic::Styles& target, ::simdjson::ondemand::object source)
{
    for (auto field : source)
        read_semantic_plot_style(target.find(field.unescaped_key()), field.value());
}

// ----------------------------------------------------------------------

void ae::chart::v3::Chart::read(const std::filesystem::path& filename)
{
    Timeit ti{fmt::format("importing chart from {}", filename), std::chrono::milliseconds{1000}};
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
                    std::vector<double> forced_column_bases_for_a_new_projections;
                    for (auto field_c : field.value().get_object()) {
                        if (const std::string_view key_c = field_c.unescaped_key(); key_c.size() == 1) {
                            switch (key_c[0]) {
                                case 'i':
                                    read_info(info(), field_c.value().get_object());
                                    break;
                                case 'a':
                                    read_antigens(antigens(), field_c.value().get_array());
                                    break;
                                case 's':
                                    read_sera(sera(), field_c.value().get_array());
                                    break;
                                case 't':
                                    read_titers(titers(), field_c.value().get_object());
                                    break;
                                case 'P':
                                    read_projections(projections(), field_c.value().get_array());
                                    break;
                                case 'R':
                                    read_semantic_plot_specification(styles(), field_c.value().get_object());
                                    break;
                                case 'p':
                                    read_legacy_plot_specification(legacy_plot_spec(), field_c.value().get_object());
                                    break;
                                case 'C': // forced column bases for a new projections
                                    for (const double cb : field_c.value().get_array())
                                        forced_column_bases_for_a_new_projections.push_back(cb);
                                    break;
                                // case 'x':
                                //     read_extension(extension_data(), field_c.value().get_object());
                                //     break;
                                default:
                                    unhandled_key({"c", key_c});
                                    break;
                            }
                        }
                        else if (key_c[0] != '?' && key_c[0] != ' ' && key_c[0] != '_')
                            unhandled_key({"c", key_c});
                    }
                    if (!forced_column_bases_for_a_new_projections.empty()) {
                        if (serum_index{forced_column_bases_for_a_new_projections.size()} != sera().size())
                            throw Error{"{}: invalid number of entries in the forced_column_bases_for_a_new_projections (\"c\":\"C\"): {}, number of sera: {}", filename.native(), forced_column_bases_for_a_new_projections.size(), sera().size()};
                        for (const auto sr_no : sera().size()) {
                            if (const auto cb = forced_column_bases_for_a_new_projections[sr_no.get()]; cb > 0.0)
                                sera()[sr_no].forced_column_basis(cb);
                            else
                                sera()[sr_no].not_forced_column_basis();
                        }
                    }
                }
                else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
                    unhandled_key({key});
            }
        }
        catch (simdjson_error& err) {
            throw Error{"{} parsing error: {} at {} \"{}\"\n", filename.native(), err.what(), parser.current_location_offset(), parser.current_location_snippet(50)};
        }
    }
    catch (simdjson_error& err) {
        throw Error{"{} json parser creation error: {} (UNESCAPED_CHARS means a char < 0x20)\n", filename.native(), err.what()};
    }

} // ae::chart::v3::Chart::read

// ----------------------------------------------------------------------
