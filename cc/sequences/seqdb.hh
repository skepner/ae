#pragma once

#include <filesystem>
#include <unordered_map>
#include <stdexcept>
#include <memory>

#include "ext/string.hh"
#include "utils/debug.hh"
#include "utils/string-hash.hh"
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
        template <typename... Args> Error(fmt::format_string<Args...> format, Args&&... args) : std::runtime_error{fmt::format("seqdb: {}", fmt::format(format, args...))} {}
    };

    Seqdb& seqdb_for_subtype(std::string_view subtype, verbose verb = verbose::no);
    void seqdb_save();

    // ----------------------------------------------------------------------

    struct SeqdbSeqRef
    {
        const SeqdbEntry* entry{nullptr};
        const SeqdbSeq* seq{nullptr};
        const SeqdbSeq* master{nullptr};

        constexpr bool operator==(const SeqdbSeqRef& rhs) const { return seq == rhs.seq; }
        constexpr operator bool() const { return entry != nullptr && seq != nullptr; }
        void erase()
        {
            entry = nullptr;
            seq = nullptr;
        }

        bool is_master() const;
        seq_id_t seq_id() const;
        std::string_view date() const;
        const sequence_aa_t& aa() const;
        const sequence_nuc_t& nuc() const;
    };

    // ----------------------------------------------------------------------

    class Seqdb
    {
      public:
        Seqdb(std::string_view subtype) : subtype_{subtype} { load(); }
        Seqdb(const Seqdb&) = delete;
        Seqdb(Seqdb&&) = delete;
        Seqdb& operator=(const Seqdb&) = delete;
        Seqdb& operator=(Seqdb&&) = delete;

        void add(const RawSequence& raw_sequence);
        void save() const;
        void save(const std::filesystem::path& filename) const;

        const SeqdbEntry* find_by_name(std::string_view name) const;
        SeqdbSeqRef find_by_name_hash(std::string_view name, const hash_t& hash) const;
        SeqdbSeqRef find_by_hash(const hash_t& hash) const; // via hash_index_

        std::shared_ptr<SeqdbSelected> select_all() const;

        void set_verbose(verbose verb) const { verbose_ = verb; }
        bool is_verbose() const { return verbose_ == verbose::yes; }

      private:
        std::string subtype_;
        std::vector<SeqdbEntry> entries_;
        bool modified_{false};
        std::unordered_map<hash_t, std::string, ae::string_hash_for_unordered_map, std::equal_to<>> hash_index_; // hash -> name
        mutable verbose verbose_{verbose::no};

        std::filesystem::path filename() const;
        std::string export_to_string() const;
        void load();
        void make_hash_index();
    };

    // ----------------------------------------------------------------------

    struct SeqdbEntry
    {
        std::string name;
        std::string continent;
        std::string country;
        std::vector<std::string> dates;
        lineage_t lineage;
        std::vector<SeqdbSeq> seqs;
        std::string host;

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

        bool has_issues() const { return issues.has_issues(); }

        void dont_keep_sequence()
        {
            aa.get().clear();
            nuc.get().clear();
        }

        bool is_master() const { return !nuc.empty(); }
    };

    // ----------------------------------------------------------------------

    class SeqdbSelected
    {
      public:
        SeqdbSelected(const Seqdb& seqdb) : seqdb_{seqdb} {}

        auto size() const { return refs_.size(); }

        SeqdbSelected& exclude_with_issue(bool exclude = true)
        {
            if (exclude)
                refs_.erase(std::remove_if(std::begin(refs_), std::end(refs_), [](const auto& ref) -> bool { return ref.seq->has_issues(); }), std::end(refs_));
            return *this;
        }

        SeqdbSelected& filter_dates(std::string_view first_date, std::string_view last_date)
        {
            if (!first_date.empty() || !last_date.empty())
                refs_.erase(std::remove_if(std::begin(refs_), std::end(refs_), [first_date, last_date](const auto& ref) -> bool { return !ref.entry->date_within(first_date, last_date); }),
                            std::end(refs_));
            return *this;
        }

        SeqdbSelected& lineage(std::string_view lineage)
        {
            refs_.erase(std::remove_if(std::begin(refs_), std::end(refs_), [lineage](const auto& ref) -> bool { return ref.entry->lineage != lineage; }), std::end(refs_));
            return *this;
        }

        SeqdbSelected& filter_human()
        {
            refs_.erase(std::remove_if(std::begin(refs_), std::end(refs_), [](const auto& ref) -> bool { return !ref.entry->host.empty(); }), std::end(refs_));
            return *this;
        }

        SeqdbSelected& filter_host(std::string_view host)
        {
            refs_.erase(std::remove_if(std::begin(refs_), std::end(refs_), [host](const auto& ref) -> bool { return ref.entry->host != host; }), std::end(refs_));
            return *this;
        }

        SeqdbSelected& filter_name(const std::vector<std::string>& names);
        SeqdbSelected& exclude_name(const std::vector<std::string>& names);
        SeqdbSelected& include_name(const std::vector<std::string>& names);

        // keeps refs for which predicate returned true
        template <std::regular_invocable<const SeqdbSeqRef&> Func> SeqdbSelected& filter(Func&& predicate) {
            refs_.erase(std::remove_if(std::begin(refs_), std::end(refs_), [&predicate](const auto& ref) -> bool { return !predicate(ref); }), std::end(refs_));
            return *this;
        }

        // removes refs for which predicate returned true
        template <std::regular_invocable<const SeqdbSeqRef&> Func> SeqdbSelected& exclude(Func&& predicate) {
            refs_.erase(std::remove_if(std::begin(refs_), std::end(refs_), [&predicate](const auto& ref) -> bool { return predicate(ref); }), std::end(refs_));
            return *this;
        }

        SeqdbSelected& sort(order ord = order::date_ascending);
        SeqdbSelected& move_name_to_beginning(const std::vector<std::string>& names);

        SeqdbSelected& find_masters()
        {
            for (auto& ref : refs_) {
                if (!ref.is_master())
                    ref.master = seqdb_.find_by_hash(ref.seq->hash).seq;
            }
            return *this;
        }

        SeqdbSelected& remove_hash_duplicates() // keep most recent
        {
            sort(order::hash);
            refs_.erase(std::unique(std::begin(refs_), std::end(refs_), [](const auto& ref1, const auto& ref2) { return ref1.seq->hash == ref2.seq->hash; }), std::end(refs_));
            return *this;
        }

        SeqdbSelected& replace_with_master()
        {
            for (auto& ref : refs_) {
                if (!ref.is_master())
                    ref = seqdb_.find_by_hash(ref.seq->hash);
            }
            return *this;
        }

        auto begin() const { return refs_.begin(); }
        auto end() const { return refs_.end(); }

      private:
        const Seqdb& seqdb_;
        std::vector<SeqdbSeqRef> refs_;

        // void remove_empty()
        // {
        //     refs_.erase(std::remove_if(std::begin(refs_), std::end(refs_), [](const auto& ref) -> bool { return !ref; }), std::end(refs_));
        // }

        friend class Seqdb;
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

} // namespace ae::sequences

// ======================================================================
