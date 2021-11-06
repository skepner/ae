#pragma once

#include <string_view>
#include <bitset>

#include "virus/type-subtype.hh"
#include "sequences/sequence.hh"

// ======================================================================

namespace ae::sequences
{
    enum class issue {
        not_translated, //
        not_aligned,    //
        prefix_x, //
        size_
    };

    struct issues_t : public std::bitset<static_cast<size_t>(issue::size_)>
    {
        issues_t() = default;
        void set(issue iss) { std::bitset<static_cast<size_t>(issue::size_)>::set(static_cast<size_t>(iss)); }
        bool is_set(issue iss) { return std::bitset<static_cast<size_t>(issue::size_)>::operator[](static_cast<size_t>(iss)); }
    };

    // ----------------------------------------------------------------------

    struct RawSequence
    {
        issues_t issues;
        std::string raw_sequence; // nuc or aa, uppercased!
        sequence_nuc_t nuc;
        sequence_aa_t aa;

        std::string name;       // parsed or raw_name if parsing failed
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
