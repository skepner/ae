#include <algorithm>

#include "ext/simdjson.hh"
#include "utils/timeit.hh"
#include "utils/log.hh"
#include "sequences/clades.hh"

// ======================================================================

void ae::sequences::Clades::load(const std::filesystem::path& clades_file)
{
    if (std::filesystem::exists(clades_file)) {
        Timeit ti{fmt::format("Loading {}", clades_file), std::chrono::milliseconds{100}};

        using namespace ae::simdjson;
        using namespace std::string_view_literals;
        try {
            Parser parser{clades_file};
            try {
                for (auto field : parser.doc().get_object()) {
                    const std::string_view subtype_key = field.unescaped_key();
                    if (subtype_key == "  version") {
                        if (const std::string_view ver{field.value()}; ver != "clades-v2")
                            throw std::runtime_error{fmt::format("[clades] unsupported version: \"{}\"", ver)};
                    }
                    else if (subtype_key == "A(H1N1)"sv || subtype_key == "A(H3N2)"sv || subtype_key == "B/Vic"sv || subtype_key == "B/Yam"sv) {
                        auto& entries = data_.try_emplace(std::string{subtype_key}, entries_t{}).first->second;
                        for (auto clade_entry : field.value().get_array()) {
                            if (clade_entry.type() == ondemand::json_type::object) {
                                entry_t entry;
                                bool has_name{false};
                                for (auto entry_field : clade_entry.get_object()) {
                                    const std::string_view entry_key = entry_field.unescaped_key();
                                    if (entry_key == "N"sv) {
                                        entry.name = static_cast<std::string_view>(entry_field.value());
                                        has_name = true;
                                    }
                                    else if (entry_key == "aa"sv)
                                        entry.aa = extract_aa_nuc_at_pos1_eq_list(entry_field.value()); // static_cast<std::string_view>(entry_field.value());
                                    else if (entry_key == "nuc"sv)
                                        entry.nuc = extract_aa_nuc_at_pos1_eq_list(entry_field.value()); // static_cast<std::string_view>(entry_field.value());
                                    else if (entry_key == "set"sv)
                                        entry.set = static_cast<std::string_view>(entry_field.value());
                                    else if (!entry_key.empty() && entry_key[0] != '?') // not a comment
                                        fmt::print(">> [clades] unrecognized key \"{}\" for entry for \"{}\"\n", entry_key, subtype_key);
                                }
                                if (has_name) // ignore entries without name, e.g. commented out
                                    entries.push_back(std::move(entry));
                            }
                            else if (clade_entry.type() != ondemand::json_type::string) // strings (separators, comments) are silently ignored
                                fmt::print(">> [clades] invalid entry in \"{}\" for \"{}\": {}\n", clades_file, subtype_key, clade_entry);
                        }
                    }
                    else if (subtype_key[0] != '?' && subtype_key[0] != ' ' && subtype_key[0] != '_')
                        fmt::print(">> [clades] unhandled \"{}\"\n", subtype_key);
                }
            }
            catch (std::exception& err) {
                fmt::print("> {} parsing error: {} at {} \"{}\"\n", clades_file, err.what(), parser.current_location_offset(), parser.current_location_snippet(50));
            }
        }
        catch (simdjson_error& err) {
            fmt::print("> {} json parser creation error: {} (UNESCAPED_CHARS means a char < 0x20)\n", clades_file, err.what());
        }
    }
    else
        fmt::print(">> cannot load clades from {}: file not found\n", clades_file);

} // ae::sequences::Clades::load

// ----------------------------------------------------------------------

std::string ae::sequences::Clades::subtype_key(const ae::virus::type_subtype_t& subtype, const lineage_t& lineage) const
{
    std::string key{subtype};
    if (key == "B") {
        if (lineage == lineage_t{"VICTORIA"})
            key += "/Vic";
        else if (lineage == lineage_t{"YAMAGATA"})
            key += "/Yam";
    }
    return key;

} // ae::sequences::Clades::subtype_key

// ----------------------------------------------------------------------

const ae::sequences::Clades::entries_t* ae::sequences::Clades::get_entries(const ae::virus::type_subtype_t& subtype, const lineage_t& lineage) const
{
    if (const auto full_set = data_.find(subtype_key(subtype, lineage)); full_set != data_.end())
        return &full_set->second;
    else
        return nullptr;

} // ae::sequences::Clades::get_entries

// ----------------------------------------------------------------------

ae::sequences::Clades::subset_t ae::sequences::Clades::get_subset(const ae::virus::type_subtype_t& subtype, const lineage_t& lineage, std::string_view set) const
{
    subset_t subset;
    if (const auto* entries = get_entries(subtype, lineage); entries) {
        if (!set.empty()) {
            for (const auto& en : *entries) {
                if (en.set == set)
                    subset.push_back(&en);
            }
        }
        else
            std::transform(std::begin(*entries), std::end(*entries), std::back_inserter(subset), [](const auto& en) { return &en; });
    }
    // for (const auto* en : subset)
    //     fmt::print(">>>> \"{}\" {} \"{}\"\n", en->name, en->aa, en->set);
    return subset;

} // ae::sequences::Clades::get_subset

// ----------------------------------------------------------------------

std::vector<std::string> ae::sequences::Clades::clades(const sequence_aa_t& aa, const sequence_nuc_t& nuc, const entries_t& entries, std::string_view set)
{
    std::vector<std::string> clades;
    for (const auto& en : entries) {
        if ((set.empty() || set == en.set) && (aa.empty() || en.aa.empty() || matches_all(aa, en.aa)) && (nuc.empty() || en.nuc.empty() || matches_all(nuc, en.nuc))) {
            // AD_DEBUG("Clades::clades \"{}\"", en.name);
            clades.push_back(en.name);
        }
    }
    std::sort(std::begin(clades), std::end(clades));
    clades.erase(std::unique(std::begin(clades), std::end(clades)), std::end(clades));
    return clades;

} // ae::sequences::Clades::clades

// ----------------------------------------------------------------------

std::vector<std::string> ae::sequences::Clades::clades(const sequence_aa_t& aa, const sequence_nuc_t& nuc, const ae::virus::type_subtype_t& subtype, const lineage_t& lineage,
                                                       std::string_view set) const
{
    if (const auto* entries = get_entries(subtype, lineage); entries)
        return clades(aa, nuc, *entries, set);
    else
        return {};

} // ae::sequences::Clades::clades

// ----------------------------------------------------------------------
