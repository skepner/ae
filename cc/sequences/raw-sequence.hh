#pragma once

#include <string_view>
#include <bitset>

#include "virus/type-subtype.hh"
#include "sequences/sequence.hh"

// ======================================================================

namespace ae::sequences
{
    enum class issue {
        not_translated,     //
        not_aligned,        //
        prefix_x,           //
        too_short,          //
        too_long,           //
        too_many_x,         //
        too_many_deletions, //
        garbage_at_the_end, //
        size_
    };

    struct issues_t : public std::bitset<static_cast<size_t>(issue::size_)>
    {
        issues_t() = default;
        void set(issue iss) { std::bitset<static_cast<size_t>(issue::size_)>::set(static_cast<size_t>(iss)); }
        bool is_set(issue iss) const { return std::bitset<static_cast<size_t>(issue::size_)>::operator[](static_cast<size_t>(iss)); }
    };

    // ----------------------------------------------------------------------

    struct RawSequence
    {
        issues_t issues;
        std::string raw_sequence; // nuc or aa, uppercased!
        sequence_pair_t sequence;
        insertions_t aa_insertions;
        insertions_t nuc_insertions;
        std::string hash_nuc;

        std::string name;       // parsed or raw_name if parsing failed
        std::string continent;
        std::string country;
        std::string reassortant;
        std::string annotations;
        std::string date;
        std::string accession_number; // gisaid isolate_id, ncbi sample_id_by_sample_provider
        virus::type_subtype_t type_subtype;
        std::string lab;
        std::string lab_id;     // cdcid
        std::string lineage;
        std::string passage;
        std::string gisaid_dna_accession_no;
        std::string gisaid_dna_insdc;
        std::string gisaid_identifier;
        std::string gisaid_last_modified;
        std::string_view raw_name{};

        RawSequence(std::string_view rn) : raw_name{rn} {}
    };
}

// ======================================================================

template <> struct fmt::formatter<ae::sequences::issue> : fmt::formatter<eu::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(ae::sequences::issue issue, FormatCtx& ctx)
    {
        using namespace ae::sequences;
        switch (issue) {
            case issue::not_translated:
                format_to(ctx.out(), "{}", "not_translated");
                break;
            case issue::not_aligned:
                format_to(ctx.out(), "{}", "not_aligned");
                break;
            case issue::prefix_x:
                format_to(ctx.out(), "{}", "prefix_x");
                break;
            case issue::too_short:
                format_to(ctx.out(), "{}", "too_short");
                break;
            case issue::too_long:
                format_to(ctx.out(), "{}", "too_long");
                break;
            case issue::too_many_x:
                format_to(ctx.out(), "{}", "too_many_x");
                break;
            case issue::too_many_deletions:
                format_to(ctx.out(), "{}", "too_many_deletions");
                break;
            case issue::garbage_at_the_end:
                format_to(ctx.out(), "{}", "garbage_at_the_end");
                break;
            case issue::size_:
                break;
        }
        return ctx.out();
    }
};

// ======================================================================
