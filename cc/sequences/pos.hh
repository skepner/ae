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
        template <std::integral T> constexpr pos1_t(T src) : named_number_t<size_t, struct seqdb_pos1_tag_t>{static_cast<size_t>(src)} {}
        constexpr pos1_t(const named_number_t<size_t, struct seqdb_pos1_tag_t>& src) : named_number_t<size_t, struct seqdb_pos1_tag_t>{src} {}

        constexpr operator pos0_t() const;
        constexpr size_t get0() const;
    };

    // constexpr const pos1_t NoPos1{static_cast<size_t>(-1)};
    constexpr const pos1_t NoPos1{-1};

    struct pos0_t : public named_number_t<size_t, struct seqdb_pos0_tag_t>
    {
        template <std::integral T> constexpr pos0_t(T src) : named_number_t<size_t, struct seqdb_pos0_tag_t>{static_cast<size_t>(src)} {}
        constexpr pos0_t(const named_number_t<size_t, struct seqdb_pos0_tag_t>& src) : named_number_t<size_t, struct seqdb_pos0_tag_t>{src} {}

        constexpr explicit operator pos1_t() const;
        constexpr size_t get0() const;

        constexpr pos0_t nuc_to_aa() const { return pos0_t{get() / 3}; }
        constexpr pos0_t aa_to_nuc() const { return pos0_t{get() * 3}; }
        constexpr size_t nuc_offset() const { return get() % 3; }
    };

    constexpr const pos0_t NoPos0{-1};

    constexpr pos0_t::operator pos1_t() const { return pos1_t{get() + 1}; }
    constexpr pos1_t::operator pos0_t() const { return pos0_t{get() - 1}; }

    constexpr size_t pos0_t::get0() const { return this->get(); }
    constexpr size_t pos1_t::get0() const { return static_cast<pos0_t>(*this).get0(); }

} // namespace ae::sequences

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::sequences::pos1_t> : fmt::formatter<eu::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const ae::sequences::pos1_t& pos1, FormatCtx& ctx) { return format_to(ctx.out(), "{}", pos1.get()); }
};

template <> struct fmt::formatter<ae::sequences::pos0_t> : fmt::formatter<eu::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::sequences::pos0_t& pos0, FormatCtx& ctx) { return format_to(ctx.out(), "{}", ae::sequences::pos1_t{pos0}); }
};

// ======================================================================
