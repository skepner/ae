#pragma once

#include <string>

#include <ext/fmt.hh>
#include <utils/named-type.hh>

// ======================================================================

namespace ae::sequences
{
    struct pos0_t;

    struct pos1_t : public named_number_t<size_t, struct seqdb_pos1_tag_t>
    {
        using named_number_t<size_t, struct seqdb_pos1_tag_t>::named_number_t;
        using named_number_t<size_t, struct seqdb_pos1_tag_t>::operator=;

        // constexpr pos1_t(pos0_t pos0);
        constexpr operator pos0_t() const;
    };

    constexpr const pos1_t NoPos1{static_cast<size_t>(-1)};

    struct pos0_t : public named_number_t<size_t, struct seqdb_pos0_tag_t>
    {
        using named_number_t<size_t, struct seqdb_pos0_tag_t>::named_number_t;
        using named_number_t<size_t, struct seqdb_pos0_tag_t>::operator=;

        // constexpr pos0_t(pos1_t pos1) : named_number_t<size_t, struct seqdb_pos0_tag_t>{pos1.get() - 1} {}
        // constexpr pos0_t(const pos0_t&) = default;
        // constexpr pos0_t& operator=(const pos0_t&) = default;
        // constexpr pos0_t& operator=(pos1_t pos1) { return operator=(pos0_t{pos1}); }
        // constexpr operator pos1_t() const { return pos1_t{get() + 1}; }
        constexpr pos0_t nuc_to_aa() const { return pos0_t{get() / 3}; }
        constexpr pos0_t aa_to_nuc() const { return pos0_t{get() * 3}; }
        constexpr size_t nuc_offset() const { return get() % 3; }
    };

    constexpr const pos0_t NoPos0{static_cast<size_t>(-1)};

    // constexpr inline pos1_t::pos1_t(pos0_t pos0) : named_number_t<size_t, struct seqdb_pos1_tag_t>{pos0.get() + 1} {}
    constexpr pos1_t::operator pos0_t() const { return pos0_t{get() - 1}; }

    // template <typename P1, typename P2> using enable_if_different_pos_t = std::enable_if_t<(std::is_same_v<P1, pos0_t> || std::is_same_v<P1, pos1_t>) && (std::is_same_v<P2, pos0_t> ||
    // std::is_same_v<P2, pos1_t>) && !std::is_same_v<P1, P2>, char>;

    // template <typename P1, typename P2, typename = enable_if_different_pos_t<P1, P2>> constexpr inline bool operator==(P1 p1, P2 p2) { return p1 == P1{p2}; }
    // template <typename P1, typename P2, typename = enable_if_different_pos_t<P1, P2>> constexpr inline bool operator!=(P1 p1, P2 p2) { return !operator==(p1, p2); }
    // template <typename P1, typename P2, typename = enable_if_different_pos_t<P1, P2>> constexpr inline bool operator>(P1 p1, P2 p2) { return p1 > P1{p2}; }
    // template <typename P1, typename P2, typename = enable_if_different_pos_t<P1, P2>> constexpr inline bool operator>=(P1 p1, P2 p2) { return p1 >= P1{p2}; }
    // template <typename P1, typename P2, typename = enable_if_different_pos_t<P1, P2>> constexpr inline bool operator<(P1 p1, P2 p2) { return p1 < P1{p2}; }
    // template <typename P1, typename P2, typename = enable_if_different_pos_t<P1, P2>> constexpr inline bool operator<=(P1 p1, P2 p2) { return p1 <= P1{p2}; }

} // namespace ae::sequences

// constexpr auto operator<=>(ae::sequences::pos0_t p1, ae::sequences::pos0_t p2) { return p1.get() <=> p2.get(); }
// constexpr auto operator<=>(ae::sequences::pos1_t p1, ae::sequences::pos1_t p2) { return p1.get() <=> p2.get(); }
// inline auto operator==(ae::sequences::pos0_t p1, ae::sequences::pos1_t p2) noexcept { return p1 == ae::sequences::pos0_t{p2}; }
// constexpr bool operator==(ae::sequences::pos0_t p1, ae::sequences::pos1_t p2) { return p1 == ae::sequences::pos0_t{p2}; }

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::sequences::pos1_t> : fmt::formatter<eu::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const ae::sequences::pos1_t& pos1, FormatCtx& ctx) { return format_to(ctx.out(), "{}", pos1.get()); }
};

template <> struct fmt::formatter<ae::sequences::pos0_t> : fmt::formatter<eu::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::sequences::pos0_t& pos0, FormatCtx& ctx) { return format_to(ctx.out(), "{}", ae::sequences::pos1_t{pos0}); }
};

// ======================================================================
