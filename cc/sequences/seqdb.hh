#pragma once

#include <filesystem>

#include "sequences/sequence.hh"
#include "sequences/issues.hh"

// ======================================================================

namespace ae::sequences
{
    struct RawSequence;
    class Seqdb;
    struct SeqdbEntry;
    struct SeqdbSeq;

    Seqdb& seqdb_for_subtype(std::string_view subtype);
    void seqdb_save();

    // ----------------------------------------------------------------------

    class Seqdb
    {
      public:
        Seqdb(std::string_view subtype);
        Seqdb(const Seqdb&) = delete;
        Seqdb(Seqdb&&) = delete;
        Seqdb& operator=(const Seqdb&) = delete;
        Seqdb& operator=(Seqdb&&) = delete;

        void add(const RawSequence& raw_sequence);
        void save();

        const SeqdbEntry* find_by_name(std::string_view name) const;

      private:
        std::string subtype_;
        std::vector<SeqdbEntry> entries_;
        bool modified_{false};

        std::filesystem::path filename() const;
    };

    // ----------------------------------------------------------------------

    struct SeqdbEntry
    {
        std::string name;
        std::string continent;
        std::string country;
        std::vector<std::string> dates;
        std::string lineage;
        std::vector<SeqdbSeq> seqs;

        SeqdbEntry() = default;
        SeqdbEntry(const RawSequence& raw_sequence);

        bool update(const RawSequence& raw_sequence); // returns if entry was modified
        bool add_date(std::string_view date);

        // std::string host() const;
        // bool date_within(std::string_view start, std::string_view end) const { return !dates.empty() && (start.empty() || dates.front() >= start) && (end.empty() || dates.front() < end); }
        // std::string_view date() const;
        // bool has_date(std::string_view date) const { return std::find(std::begin(dates), std::end(dates), date) != std::end(dates); }
        // std::string location() const;
    };

    // ----------------------------------------------------------------------

    struct SeqdbSeq
    {
        using lab_ids_t = std::vector<std::string>;
        using labs_t = std::vector<std::pair<std::string_view, lab_ids_t>>;

        struct gisaid_data_t
        {
            std::vector<std::string> isolate_ids; // gisaid accession numbers
            std::vector<std::string> sample_ids_by_sample_provider; // ncbi accession numbers
        };

        // master_ref_t master; // for slave only
        sequence_aa_t aa;
        sequence_nuc_t nuc;
        std::vector<std::string> annotations;
        std::vector<std::string> reassortants;
        std::vector<std::string> passages;
        std::string hash;
        seqdb_issues_t issues;
        labs_t lab_ids;
        gisaid_data_t gisaid;

        SeqdbSeq() = default;
        bool update(const RawSequence& raw_sequence); // returns if entry was modified
    };

    // ----------------------------------------------------------------------

    inline const SeqdbEntry* Seqdb::find_by_name(std::string_view name) const
    {
        if (const auto found = std::lower_bound(std::begin(entries_), std::end(entries_), name, [](const auto& entry, std::string_view nam) { return entry.name < nam; });
            found != std::end(entries_) && found->name == name)
            return &*found;
        else
            return nullptr;
    }

} // namespace ae::sequences

// ======================================================================
