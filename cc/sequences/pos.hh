#pragma once

#include <charconv>
#include <vector>
#include <algorithm>

#include <utils/named-type.hh>
#include "utils/string.hh"

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

        constexpr pos1_t aa_to_nuc() const;
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

    constexpr pos1_t pos1_t::aa_to_nuc() const { return static_cast<pos1_t>(static_cast<pos0_t>(*this).aa_to_nuc()); }

    // ----------------------------------------------------------------------

    namespace detail
    {
        inline size_t from_chars(std::string_view src)
        {
            size_t result;
            if (const auto [p, ec] = std::from_chars(&*src.begin(), &*src.end(), result); ec == std::errc{} && p == &*src.end())
                return result;
            else
                return std::numeric_limits<size_t>::max();
        }

    } // namespace detail

    class extract_at_pos_error : public std::runtime_error
    {
      public:
        using std::runtime_error::runtime_error;
    };

    struct aa_nuc_at_pos1_eq_t : public std::tuple<pos1_t, char, bool> // pos (1-based), aa, equal/not-equal
    {
        using std::tuple<pos1_t, char, bool>::tuple;
        constexpr aa_nuc_at_pos1_eq_t() : std::tuple<pos1_t, char, bool>{pos1_t{0}, ' ', false} {}
    };

    using amino_acid_at_pos1_eq_list_t = std::vector<aa_nuc_at_pos1_eq_t>;

    template <size_t MIN_SIZE, size_t MAX_SIZE> inline aa_nuc_at_pos1_eq_t extract_aa_nuc_at_pos1_eq(std::string_view source)
    {
        if (source.size() >= MIN_SIZE && source.size() <= MAX_SIZE && std::isdigit(source.front()) && (std::isalpha(source.back()) || source.back() == '-'))
            return {pos1_t{detail::from_chars(source.substr(0, source.size() - 1))}, source.back(), true};
        else if (source.size() >= (MIN_SIZE + 1) && source.size() <= (MAX_SIZE + 1) && source.front() == '!' && std::isdigit(source[1]) && (std::isalpha(source.back()) || source.back() == '-'))
            return {pos1_t{detail::from_chars(source.substr(1, source.size() - 2))}, source.back(), false};
        else
            throw extract_at_pos_error{fmt::format("invalid aa/nuc-pos: \"{}\" (expected 183P or !183P)", source)};
    }

    template <size_t MIN_SIZE, size_t MAX_SIZE> inline amino_acid_at_pos1_eq_list_t extract_aa_nuc_at_pos1_eq_list(std::string_view source)
    {
        const auto fields = ae::string::split(source, ae::string::split_emtpy::remove);
        amino_acid_at_pos1_eq_list_t pos1_aa_eq(fields.size());
        std::transform(std::begin(fields), std::end(fields), std::begin(pos1_aa_eq), [](std::string_view field) { return extract_aa_nuc_at_pos1_eq<MIN_SIZE, MAX_SIZE>(field); });
        return pos1_aa_eq;

    } // acmacs::seqdb::v3::extract_aa_at_pos_eq_list

    inline amino_acid_at_pos1_eq_list_t extract_aa_nuc_at_pos1_eq_list(const std::vector<std::string>& source)
    {
        amino_acid_at_pos1_eq_list_t list(source.size());
        std::transform(std::begin(source), std::end(source), std::begin(list), [](const auto& en) { return extract_aa_nuc_at_pos1_eq<2, 6>(en); });
        return list;

    } // acmacs::seqdb::v3::extract_aa_at_pos1_eq_list

    inline amino_acid_at_pos1_eq_list_t extract_aa_nuc_at_pos1_eq_list(std::string_view source) { return extract_aa_nuc_at_pos1_eq_list<2, 6>(source); }

    // ----------------------------------------------------------------------

    template <typename Seq, typename ARG> inline bool matches_all(const Seq& seq, ARG data)
    {
        using namespace ae::sequences;
        const auto matches = [&seq](const auto& en) {
            const auto eq = seq[std::get<pos1_t>(en)] == std::get<char>(en);
            return std::get<bool>(en) == eq;
        };
        const auto elts = extract_aa_nuc_at_pos1_eq_list(data);
        return std::all_of(std::begin(elts), std::end(elts), matches);
    }

} // namespace ae::sequences

// ----------------------------------------------------------------------

template <> struct fmt::formatter<ae::sequences::pos1_t> : fmt::formatter<eu::fmt_helper::default_formatter> {
    template <typename FormatCtx> auto format(const ae::sequences::pos1_t& pos1, FormatCtx& ctx) { return format_to(ctx.out(), "{}", pos1.get()); }
};

template <> struct fmt::formatter<ae::sequences::pos0_t> : fmt::formatter<eu::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::sequences::pos0_t& pos0, FormatCtx& ctx) { return format_to(ctx.out(), "{}", ae::sequences::pos1_t{pos0}); }
};

template <> struct fmt::formatter<ae::sequences::aa_nuc_at_pos1_eq_t> : fmt::formatter<eu::fmt_helper::default_formatter>
{
    template <typename FormatCtx> auto format(const ae::sequences::aa_nuc_at_pos1_eq_t& pos1_eq, FormatCtx& ctx)
    {
        return format_to(ctx.out(), "{}{}{}", std::get<bool>(pos1_eq) ? "" : "!", std::get<ae::sequences::pos1_t>(pos1_eq), std::get<char>(pos1_eq));
    }
};

// ======================================================================
