#pragma once

#include <filesystem>
#include <unordered_map>

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

    struct SeqdbSeqRef
    {
        const SeqdbEntry* entry{nullptr};
        const SeqdbSeq* seq{nullptr};
        constexpr operator bool() const { return entry != nullptr && seq != nullptr; }
    };

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
        SeqdbSeqRef find_by_name_hash(std::string_view name, const hash_t& hash) const;
        SeqdbSeqRef find_by_hash(const hash_t& hash) const; // via hash_index_

      private:
        std::string subtype_;
        std::vector<SeqdbEntry> entries_;
        bool modified_{false};
        std::unordered_map<hash_t, std::string_view> hash_index_; // hash -> name

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

        const SeqdbSeq* find_by_hash(const hash_t& hash) const;
        SeqdbSeq* find_by_hash(const hash_t& hash);

        bool update(const RawSequence& raw_sequence, bool keep_sequence = true); // returns if entry was modified
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
        using labs_t = std::vector<std::pair<std::string, lab_ids_t>>;

        struct gisaid_data_t
        {
            std::vector<std::string> accession_number;                   // gisaid accession numbers, ncbi accession numbers
            std::vector<std::string> gisaid_dna_accession_no;
            std::vector<std::string> gisaid_dna_insdc;
            std::vector<std::string> gisaid_identifier;
            std::vector<std::string> gisaid_last_modified;
            std::vector<std::string> gisaid_submitter;
            std::vector<std::string> gisaid_originating_lab;

            bool empty() const { return accession_number.empty() && gisaid_dna_accession_no.empty() && gisaid_dna_insdc.empty() && gisaid_identifier.empty() && gisaid_last_modified.empty(); }
        };

        // master_ref_t master; // for slave only
        sequence_aa_t aa;
        sequence_nuc_t nuc;
        std::vector<std::string> annotations;
        std::vector<std::string> reassortants;
        std::vector<std::string> passages;
        hash_t hash;
        seqdb_issues_t issues;
        labs_t lab_ids;
        gisaid_data_t gisaid;

        SeqdbSeq() = default;
        bool update(const RawSequence& raw_sequence, bool keep_sequence); // returns if entry was modified

        void dont_keep_sequence()
        {
            aa.get().clear();
            nuc.get().clear();
        }
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

    inline SeqdbSeqRef Seqdb::find_by_name_hash(std::string_view name, const hash_t& hash) const
    {
        if (const auto* entry = find_by_name(name); entry) {
            if (const auto* seq = entry->find_by_hash(hash); seq)
                return SeqdbSeqRef{.entry = entry, .seq = seq};
        }
        return {};
    }

    inline SeqdbSeqRef Seqdb::find_by_hash(const hash_t& hash) const
    {
        if (const auto found = hash_index_.find(hash); found != hash_index_.end())
            return find_by_name_hash(found->second, hash);
        else
            return {};
    }

    inline const SeqdbSeq* SeqdbEntry::find_by_hash(const hash_t& hash) const
    {
        if (const auto found = std::find_if(std::begin(seqs), std::end(seqs), [&hash](const auto& seq) { return seq.hash == hash; }); found != std::end(seqs))
            return &*found;
        else
            return nullptr;
    }

    inline SeqdbSeq* SeqdbEntry::find_by_hash(const hash_t& hash)
    {
        if (const auto found = std::find_if(std::begin(seqs), std::end(seqs), [&hash](const auto& seq) { return seq.hash == hash; }); found != std::end(seqs))
            return &*found;
        else
            return nullptr;
    }

} // namespace ae::sequences

// ======================================================================
