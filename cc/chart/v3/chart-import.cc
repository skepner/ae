#include "ext/simdjson.hh"
#include "ext/from_chars.hh"
#include "utils/log.hh"
#include "utils/timeit.hh"
// #include "utils/file.hh"
#include "chart/v3/chart.hh"

// ----------------------------------------------------------------------

void ae::chart::v3::Chart::write(const std::filesystem::path& filename) const
{
    Timeit ti{"exporting chart", std::chrono::milliseconds{5000}};

} // ae::chart::v3::Chart::write

// ======================================================================

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
            case 's': // key-value  pairs                 | semantic attributes by group (see below the table)
                handled = false;
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
                    serum.homologous_antigens().push_back(ae::antigen_index{static_cast<uint64_t>(field.value())});
                }
                else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
                    unhandled_key({"c", "s", key});
            }
        }
    }
}

// ----------------------------------------------------------------------

inline void read_sparse(ae::chart::v3::Titers::sparse_t& target, ::simdjson::ondemand::array source)
{
    ae::antigen_index ag_no{0};
    for (auto row : source) {
        auto& target_row = target.emplace_back();
        for (auto source_entry : row.get_object()) {
            target_row.emplace_back(ae::serum_index{ae::from_chars<ae::serum_index::value_type>(static_cast<std::string_view>(source_entry.unescaped_key()))},
                                    ae::chart::v3::Titer{static_cast<std::string_view>(source_entry.value())});
        }
    }
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
            read_sparse(target.create_sparse_titers(), field.value().get_array());
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
                projection.dodgy_titer_is_regular(static_cast<bool>(field.value()) ? ae::chart::v3::dodgy_titer_is_regular::yes : ae::chart::v3::dodgy_titer_is_regular::no);
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

inline void read_legacy_plot_specification(ae::chart::v3::LegacyPlotSpec& target, ::simdjson::ondemand::object source)
{
    for (auto field : source) {
        if (const std::string_view key = field.unescaped_key(); key == "") {
        }
        else if (key == "") {  //
        }
        else if (key[0] != '?' && key[0] != ' ' && key[0] != '_')
            unhandled_key({"c", "p", key});
    }

} // read_legacy_plot_specification

// ----------------------------------------------------------------------

void ae::chart::v3::Chart::read(const std::filesystem::path& filename)
{
    Timeit ti{"importing chart"}; // , std::chrono::milliseconds{500}};
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
                                case 'p':
                                    read_legacy_plot_specification(legacy_plot_spec(), field_c.value().get_object());
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
            throw Error{"{} parsing error: {} at {} \"{}\"\n", filename.native(), err.what(), parser.current_location_offset(), parser.current_location_snippet(50)};
        }
    }
    catch (simdjson_error& err) {
        throw Error{"{} json parser creation error: {} (UNESCAPED_CHARS means a char < 0x20)\n", filename.native(), err.what()};
    }

} // ae::chart::v3::Chart::read

// ----------------------------------------------------------------------
