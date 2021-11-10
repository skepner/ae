#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <algorithm>

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

        void update(const RawSequence& raw_sequence);
        void add_date(std::string_view date);

        // std::string host() const;
        // bool date_within(std::string_view start, std::string_view end) const { return !dates.empty() && (start.empty() || dates.front() >= start) && (end.empty() || dates.front() < end); }
        // std::string_view date() const;
        // bool has_date(std::string_view date) const { return std::find(std::begin(dates), std::end(dates), date) != std::end(dates); }
        // std::string location() const;
    };

    // ----------------------------------------------------------------------

    struct SeqdbSeq
    {
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
