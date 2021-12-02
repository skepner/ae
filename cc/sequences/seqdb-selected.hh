#pragma once

#include "sequences/seqdb.hh"

// ======================================================================

namespace ae::sequences
{
    class SeqdbSelected
    {
      public:
        SeqdbSelected(const Seqdb& seqdb) : seqdb_{seqdb} {}

        auto size() const { return refs_.size(); }
        auto empty() const { return refs_.empty(); }
        auto begin() const { return refs_.begin(); }
        auto end() const { return refs_.end(); }
        auto begin() { return refs_.begin(); }
        auto end() { return refs_.end(); }
        const auto& operator[](size_t index) const { return refs_[index]; }

        SeqdbSelected& exclude_with_issue(bool exclude = true)
        {
            if (exclude)
                erase_with_issues(*this);
            return *this;
        }

        SeqdbSelected& filter_dates(std::string_view first_date, std::string_view last_date)
        {
            if (!first_date.empty() || !last_date.empty())
                erase_if(*this, [first_date, last_date](const auto& ref) -> bool { return !ref.entry->date_within(first_date, last_date); });
            return *this;
        }

        SeqdbSelected& lineage(std::string_view lineage)
        {
            erase_if(*this, [lineage](const auto& ref) -> bool { return ref.entry->lineage != lineage; });
            return *this;
        }

        SeqdbSelected& filter_human()
        {
            erase_if(*this, [](const auto& ref) -> bool { return !ref.entry->host.empty(); });
            return *this;
        }

        SeqdbSelected& filter_host(std::string_view host)
        {
            erase_if(*this, [host](const auto& ref) -> bool { return ref.entry->host != host; });
            return *this;
        }

        SeqdbSelected& filter_name(const std::vector<std::string>& names);
        SeqdbSelected& exclude_name(const std::vector<std::string>& names);
        SeqdbSelected& include_name(const std::vector<std::string>& names, bool include_with_issue_too = false);

        SeqdbSelected& filter_name(std::string_view name, std::string_view reassortant, std::string_view passage);

        // keeps refs for which predicate returned true
        template <std::regular_invocable<const SeqdbSeqRef&> Func> SeqdbSelected& filter(Func&& predicate)
        {
            erase_if(*this, [&predicate](const auto& ref) -> bool { return !predicate(ref); });
            return *this;
        }

        // removes refs for which predicate returned true
        template <std::regular_invocable<const SeqdbSeqRef&> Func> SeqdbSelected& exclude(Func&& predicate)
        {
            erase_if(*this, std::forward<Func>(predicate));
            return *this;
        }

        SeqdbSelected& sort(order ord = order::date_ascending);
        SeqdbSelected& move_name_to_beginning(const std::vector<std::string>& names);

        SeqdbSelected& find_masters()
        {
            for (auto& ref : refs_)
                ref.set_master(seqdb_);
            return *this;
        }

        SeqdbSelected& find_clades(std::string_view clades_json_file);

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

      private:
        const Seqdb& seqdb_;
        SeqdbSeqRefList refs_;

        template <typename Pred> static inline void erase_if(SeqdbSelected& selected, Pred&& pred)
        {
            selected.refs_.erase(std::remove_if(std::begin(selected.refs_), std::end(selected.refs_), std::forward<Pred>(pred)), std::end(selected.refs_));
        }

        static inline void erase_with_issues(SeqdbSelected& selected)
        {
            erase_if(selected, [](const auto& ref) -> bool { return ref.seq->has_issues(); });
        }

        // void remove_empty()
        // {
        //     refs_.erase(std::remove_if(std::begin(refs_), std::end(refs_), [](const auto& ref) -> bool { return !ref; }), std::end(refs_));
        // }

        friend class Seqdb;
    };

} // namespace ae::sequences

// ======================================================================
