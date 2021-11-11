#pragma once

#include <bitset>

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
    }

    struct issues_t : public std::bitset<static_cast<size_t>(issue::size_)>
    {
        issues_t() = default;
        void set(issue iss) { std::bitset<static_cast<size_t>(issue::size_)>::set(static_cast<size_t>(iss)); }
        bool is_set(issue iss) const { return std::bitset<static_cast<size_t>(issue::size_)>::operator[](static_cast<size_t>(iss)); }
    };

    struct seqdb_issues_t
    {
        std::string data_;

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
    };

} // namespace ae::sequences

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

template <> struct fmt::formatter<ae::sequences::seqdb_issues_t> : fmt::formatter<eu::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const ae::sequences::seqdb_issues_t& seqdb_issues, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "{{{}}}", seqdb_issues.data_);
    }
};

// ======================================================================
