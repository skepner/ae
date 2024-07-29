#pragma once

#include <bitset>
#include <vector>

#include "ext/fmt.hh"
#include "utils/enum.hh"

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

    inline char issue_to_char(issue iss)
    {
        switch (iss) {
            case issue::not_translated:
                return 't';
            case issue::not_aligned:
                return 'a';
            case issue::prefix_x:
                return 'x';
            case issue::too_short:
                return 's';
            case issue::too_long:
                return 'l';
            case issue::too_many_x:
                return 'X';
            case issue::too_many_deletions:
                return 'D';
            case issue::garbage_at_the_end:
                return 'E';
            case issue::size_:
                return ' ';
        }
        return ' ';             // hey g++13

    }

    inline issue issue_from_char(char iss)
    {
        switch (iss) {
            case 't':
                return issue::not_translated;
            case 'a':
                return issue::not_aligned;
            case 'x':
                return issue::prefix_x;
            case 's':
                return issue::too_short;
            case 'l':
                return issue::too_long;
            case 'X':
                return issue::too_many_x;
            case 'D':
                return issue::too_many_deletions;
            case 'E':
                return issue::garbage_at_the_end;
            default:
                return issue::size_;
        }
    }

    struct issues_t : public std::bitset<static_cast<size_t>(issue::size_)>
    {
        issues_t() = default;
        void set(issue iss) { std::bitset<static_cast<size_t>(issue::size_)>::set(static_cast<size_t>(iss)); }
        bool is_set(issue iss) const { return std::bitset<static_cast<size_t>(issue::size_)>::operator[](static_cast<size_t>(iss)); }
        std::vector<std::string> to_strings() const;
    };

    struct seqdb_issues_t
    {
        std::string data_{};

        seqdb_issues_t() = default;
        seqdb_issues_t(const seqdb_issues_t&) = default;
        seqdb_issues_t(seqdb_issues_t&&) = default;
        seqdb_issues_t& operator=(const seqdb_issues_t&) = default;
        seqdb_issues_t& operator=(seqdb_issues_t&&) = default;
        seqdb_issues_t& operator=(std::string_view src)
        {
            data_ = src;
            return *this;
        }

        bool has_issues() const { return !data_.empty(); }
        bool just_too_short() const { return data_ == "s"; } // there is just one issue it is too_short

        bool update(const issues_t& issues)
        {
            bool updated { false };
            for (auto iss = issue::not_translated; iss < issue::size_; ++iss) {
                if (issues.is_set(iss)) {
                    if (const auto ic = issue_to_char(iss); data_.find(ic) == std::string::npos) {
                        data_.push_back(ic);
                        updated = true;
                    }
                }
            }
            return updated;
        }

        issues_t extract() const
        {
            issues_t issues;
            for (char cc : data_)
                issues.set(issue_from_char(cc));
            return issues;
        }

        std::vector<std::string> to_strings() const { return extract().to_strings(); }
    };

} // namespace ae::sequences

// ======================================================================

template <> struct fmt::formatter<ae::sequences::issue> : formatter<string_view> {
    auto format(ae::sequences::issue issue, format_context& ctx) const
    {
            using namespace ae::sequences;
            std::string_view text;
        switch (issue) {
            case issue::not_translated:
                text = "not_translated";
                break;
            case issue::not_aligned:
                text = "not_aligned";
                break;
            case issue::prefix_x:
                text = "prefix_x";
                break;
            case issue::too_short:
                text = "too_short";
                break;
            case issue::too_long:
                text = "too_long";
                break;
            case issue::too_many_x:
                text = "too_many_x";
                break;
            case issue::too_many_deletions:
                text = "too_many_deletions";
                break;
            case issue::garbage_at_the_end:
                text = "garbage_at_the_end";
                break;
            case issue::size_:
                break;
        }
        return formatter<string_view>::format(text, ctx);
    }
};

template <> struct fmt::formatter<ae::sequences::seqdb_issues_t> : fmt::formatter<ae::fmt_helper::default_formatter> {
    auto format(const ae::sequences::seqdb_issues_t& seqdb_issues, format_context& ctx) const
    {
        return fmt::format_to(ctx.out(), "{{{}}}", seqdb_issues.data_);
    }
};

// ======================================================================

inline std::vector<std::string> ae::sequences::issues_t::to_strings() const
{
    std::vector<std::string> result;
    for (auto iss = issue::not_translated; iss < issue::size_; ++iss) {
        if (is_set(iss))
            result.push_back(fmt::format("{}", iss));
    }
    return result;
}

// ======================================================================
