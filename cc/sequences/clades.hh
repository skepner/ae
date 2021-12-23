#pragma once

#include <filesystem>
#include <unordered_map>
#include <vector>

#include "ext/fmt.hh"
#include "ext/string.hh"
#include "virus/type-subtype.hh"
#include "sequences/lineage.hh"
#include "sequences/sequence.hh"

// ======================================================================

namespace ae::sequences
{
    class Clades
    {
      public:
        struct entry_t
        {
            std::string name;
            amino_acid_at_pos1_eq_list_t aa;
            amino_acid_at_pos1_eq_list_t nuc;
            std::string set;
        };

        using subset_t = std::vector<const entry_t*>;

        Clades(const std::filesystem::path& clades_file) { load(clades_file); }

        std::vector<std::string> clades(const sequence_aa_t& aa, const sequence_nuc_t& nuc, const ae::virus::type_subtype_t& subtype, const lineage_t& lineage, std::string_view set = {}) const;
        subset_t get(const ae::virus::type_subtype_t& subtype, const lineage_t& lineage, std::string_view set = {}) const;

      private:
        using entries_t = std::vector<entry_t>;
        std::unordered_map<std::string, entries_t> data_{};

        void load(const std::filesystem::path& clades_file);
        std::string subtype_key(const ae::virus::type_subtype_t& subtype, const lineage_t& lineage) const;
    };

} // namespace ae::sequences

// ----------------------------------------------------------------------

// template <> struct fmt::formatter<ae::sequences::> : fmt::formatter<std::string>
// {
//     template <typename FormatCtx> constexpr auto format(const ae::sequences::& , FormatCtx& ctx) { return fmt::formatter<std::string>::format(, ctx); }
// };

// ======================================================================
