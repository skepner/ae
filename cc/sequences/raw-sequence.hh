#pragma once

#include <string_view>

#include "virus/type-subtype.hh"
#include "sequences/sequence.hh"
#include "sequences/issues.hh"

// ======================================================================

namespace ae::sequences
{
    struct RawSequence
    {
        issues_t issues;
        std::string raw_sequence; // nuc or aa, uppercased!
        sequence_pair_t sequence;
        insertions_t aa_insertions;
        insertions_t nuc_insertions;
        hash_t hash_nuc;

        std::string name;       // parsed or raw_name if parsing failed
        std::string host;
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
        std::string gisaid_submitter;
        std::string gisaid_originating_lab;
        std::string_view raw_name{};

        RawSequence(std::string_view rn) : raw_name{rn} {}
    };
}

// ======================================================================
