#pragma once

#include <filesystem>
#include <unordered_map>
#include <vector>

#include "ext/fmt.hh"
#include "ext/compare.hh"
#include "utils/named-vector.hh"
#include "virus/type-subtype.hh"
#include "sequences/lineage.hh"
#include "sequences/sequence.hh"

// ======================================================================

namespace ae::sequences
{
    using clades_t = ae::named_vector_t<std::string, struct chart_clades_tag>;

    // ----------------------------------------------------------------------

    // Data read from the clades file (usually ${AC_CLADES_JSON_V2})
    class Clades
    {
      public:
        struct entry_t
        {
            std::string name{};
            amino_acid_at_pos1_eq_list_t aa{};
            amino_acid_at_pos1_eq_list_t nuc{};
            std::string set{};
        };

        using entries_t = std::vector<entry_t>;
        using subset_t = std::vector<const entry_t*>;
        enum load_from_default_file_ { load_from_default_file };

        Clades() = default;
        Clades(load_from_default_file_) { load(); }
        Clades(const std::filesystem::path& clades_file) { load(clades_file); }

        ae::sequences::clades_t clades(const sequence_aa_t& aa, const sequence_nuc_t& nuc, const ae::virus::type_subtype_t& subtype, const lineage_t& lineage, std::string_view set = {}) const;
        const entries_t* get_entries(const ae::virus::type_subtype_t& subtype, const lineage_t& lineage) const;
        subset_t get_subset(const ae::virus::type_subtype_t& subtype, const lineage_t& lineage, std::string_view set = {}) const;

        static ae::sequences::clades_t clades(const sequence_aa_t& aa, const sequence_nuc_t& nuc, const entries_t& entries, std::string_view set = {});

      private:
        std::unordered_map<std::string, entries_t> data_{};

        void load();
        void load(const std::filesystem::path& clades_file);
        std::string subtype_key(const ae::virus::type_subtype_t& subtype, const lineage_t& lineage) const;
    };

} // namespace ae::sequences

// ----------------------------------------------------------------------

// template <> struct fmt::formatter<ae::sequences::> : fmt::formatter<std::string>
// {
//     template <typename FormatCtx> constexpr auto format(const ae::sequences::& , FormatCtx& ctx) const { return fmt::formatter<std::string>::format(, ctx); }
// };

// ======================================================================
