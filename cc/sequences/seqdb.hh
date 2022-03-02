#pragma once

#include <filesystem>
#include <unordered_map>
#include <stdexcept>
#include <memory>

#include "ext/string.hh"
#include "utils/log.hh"
#include "utils/hash.hh"
#include "utils/string-hash.hh"
#include "virus/type-subtype.hh"
#include "sequences/sequence.hh"
#include "sequences/issues.hh"
#include "sequences/lineage.hh"

// ======================================================================

namespace ae::sequences
{
    struct RawSequence;
    class Seqdb;
    struct SeqdbEntry;
    struct SeqdbSeq;
    class SeqdbSelected;

    enum class order { none, date_ascending, date_descending, name_ascending, name_descending, hash };

    using seq_id_t = ae::named_string_t<std::string, struct acmacs_seqdb_SeqId_tag>;

    class Error : public std::runtime_error
    {
      public:
        template <typename... Args> Error(fmt::format_string<Args...> format, Args&&... args) : std::runtime_error{fmt::format("[seqdb] {}", fmt::format(format, std::forward<Args>(args)...))} {}
    };

    Seqdb& seqdb_for_subtype(const virus::type_subtype_t& subtype, verbose verb = verbose::no);
    void seqdb_save();

    // ----------------------------------------------------------------------

    struct SeqdbSeqRef
    {
        const SeqdbEntry* entry{nullptr};
        const SeqdbSeq* seq{nullptr};
        const SeqdbSeq* master{nullptr};
        std::vector<std::string> clades{};

        constexpr bool operator==(const SeqdbSeqRef& rhs) const { return seq == rhs.seq; }
        constexpr operator bool() const { return entry != nullptr && seq != nullptr; }
        void erase()
        {
            entry = nullptr;
            seq = nullptr;
        }

        bool is_master() const;
        void set_master(const Seqdb& seqdb);
        seq_id_t seq_id() const;
        std::string_view date() const;
        const sequence_aa_t& aa() const;
        const sequence_nuc_t& nuc() const;
    };

    using SeqdbSeqRefList = std::vector<SeqdbSeqRef>;
    extern SeqdbSeqRefList SeqdbSeqRefList_empty;

    // ----------------------------------------------------------------------

    class Seqdb
    {
      public:
        Seqdb(const virus::type_subtype_t& subtype) : subtype_{subtype} { load(); }
        Seqdb(const Seqdb&) = delete;
        Seqdb(Seqdb&&) = delete;
        Seqdb& operator=(const Seqdb&) = delete;
        Seqdb& operator=(Seqdb&&) = delete;

        constexpr const virus::type_subtype_t& subtype() const { return subtype_; }

        bool add(const RawSequence& raw_sequence); // returns if sequence was added
        void save() const;
        void save(const std::filesystem::path& filename) const;

        const SeqdbEntry* find_by_name(std::string_view name) const;
        SeqdbSeqRef find_by_name_hash(std::string_view name, const hash_t& hash) const;
        SeqdbSeqRef find_by_hash(const hash_t& hash) const; // via hash_index_
        const SeqdbSeqRefList& find_all_by_hash(const hash_t& hash) const
        {
            if (const auto found = hash_index_all_.find(hash); found != hash_index_all_.end())
                return found->second;
            else
                return SeqdbSeqRefList_empty;
        }

        enum class set_master { no, yes };

        SeqdbSeqRef find_by_seq_id(const seq_id_t& seq_id, set_master sm) const
        {
            if (const auto found = seq_id_index().find(seq_id); found != seq_id_index_.end()) {
                SeqdbSeqRef ref{found->second};
                if (sm == set_master::yes)
                    ref.set_master(*this);
                return ref;
            }
            else
                return {};
        }

        std::shared_ptr<SeqdbSelected> select_all() const;

        void set_verbose(verbose verb) const { verbose_ = verb; }
        bool is_verbose() const { return verbose_ == verbose::yes; }

      private:
        using hash_index_t = std::unordered_map<hash_t, std::string, ae::string_hash_for_unordered_map, std::equal_to<>>;
        using hash_index_all_t = std::unordered_map<hash_t, SeqdbSeqRefList, ae::string_hash_for_unordered_map, std::equal_to<>>;
        using seq_id_index_t = std::unordered_map<seq_id_t, SeqdbSeqRef, ae::string_hash_for_unordered_map, std::equal_to<>>;

        virus::type_subtype_t subtype_{};
        std::vector<SeqdbEntry> entries_{};
        bool modified_{false};
        hash_index_t hash_index_{}; // hash -> name
        hash_index_all_t hash_index_all_{}; // hash -> vector<ref>
        mutable seq_id_index_t seq_id_index_{};
        mutable verbose verbose_{verbose::no};

        std::filesystem::path filename() const;
        std::string export_to_string() const;
        void load();
        void make_hash_index();
        void remove(const SeqdbSeqRef& ref);
        void remove(const hash_t& hash); // remove all seqs (and perhaps some entries) with that hash
        const seq_id_index_t& seq_id_index() const;

    };

    // ----------------------------------------------------------------------

    struct SeqdbEntry
    {
        std::string name{};
        std::string continent{};
        std::string country{};
        std::vector<std::string> dates{};
        lineage_t lineage{};
        std::vector<SeqdbSeq> seqs{};
        std::string host{};

        SeqdbEntry() = default;
        SeqdbEntry(const RawSequence& raw_sequence);

        const SeqdbSeq* find_by_hash(const hash_t& hash) const;
        SeqdbSeq* find_by_hash(const hash_t& hash);

        bool update(const RawSequence& raw_sequence, bool keep_sequence = true); // returns if entry was modified
        bool add_date(std::string_view date);

        bool date_within(std::string_view first_date, std::string_view last_date) const
        {
            return !dates.empty() && (first_date.empty() || first_date <= dates.back()) && (last_date.empty() || last_date > dates.back());
        }

        bool date_less_than(const SeqdbEntry& another) const
        {
            if (!dates.empty()) {
                if (!another.dates.empty())
                    return dates.back() < another.dates.back();
                else
                    return true;
            }
            else
                return false;
        }

        bool date_more_than(const SeqdbEntry& another) const
        {
            if (!dates.empty()) {
                if (!another.dates.empty())
                    return dates.back() > another.dates.back();
                else
                    return true;
            }
            else
                return false;
        }

        std::string_view date() const { return dates.empty() ? std::string_view{} : std::string_view{dates.back()}; }

        std::vector<std::string> labs() const;

        // std::string host() const;
        // bool date_within(std::string_view start, std::string_view end) const { return !dates.empty() && (start.empty() || dates.front() >= start) && (end.empty() ||
        // dates.front() < end); } std::string_view date() const; bool has_date(std::string_view date) const { return std::find(std::begin(dates), std::end(dates), date)
        // != std::end(dates); } std::string location() const;
    };

    // ----------------------------------------------------------------------

    struct SeqdbSeq
    {
        using lab_ids_t = std::vector<std::string>;
        using labs_t = std::vector<std::pair<std::string, lab_ids_t>>;

        struct gisaid_data_t
        {
            std::vector<std::string> accession_number; // gisaid accession numbers, ncbi accession numbers
            std::vector<std::string> gisaid_dna_accession_no;
            std::vector<std::string> gisaid_dna_insdc;
            std::vector<std::string> gisaid_identifier;
            std::vector<std::string> gisaid_last_modified;
            std::vector<std::string> gisaid_submitter;
            std::vector<std::string> gisaid_originating_lab;

            bool empty() const { return accession_number.empty() && gisaid_dna_accession_no.empty() && gisaid_dna_insdc.empty() && gisaid_identifier.empty() && gisaid_last_modified.empty(); }
        };

        // master_ref_t master{}; // for slave only
        sequence_aa_t aa{};
        sequence_nuc_t nuc{};
        insertions_t aa_insertions{};
        std::vector<std::string> annotations{};
        std::vector<std::string> reassortants{};
        std::vector<std::string> passages{};
        hash_t hash{};
        seqdb_issues_t issues{};
        labs_t lab_ids{};
        gisaid_data_t gisaid{};

        SeqdbSeq() = default;
        bool update(const RawSequence& raw_sequence, bool keep_sequence); // returns if entry was modified

        bool has_issues() const { return issues.has_issues(); }

        void dont_keep_sequence()
        {
            aa.get().clear();
            nuc.get().clear();
        }

        bool is_master() const { return !nuc.empty(); }
    };

    // ----------------------------------------------------------------------

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

    inline std::string_view SeqdbSeqRef::date() const { return entry->date(); }

    inline const sequence_aa_t& SeqdbSeqRef::aa() const
    {
        if (master)
            return master->aa;
        else
            return seq->aa;
    }

    inline const sequence_nuc_t& SeqdbSeqRef::nuc() const
    {
        if (master)
            return master->nuc;
        else
            return seq->nuc;
    }

    inline bool SeqdbSeqRef::is_master() const { return !seq->nuc.empty(); }

    inline void SeqdbSeqRef::set_master(const Seqdb& seqdb)
    {
        if (!is_master())
            master = seqdb.find_by_hash(seq->hash).seq;
    }

} // namespace ae::sequences

// ======================================================================
