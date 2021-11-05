#pragma once

#include <string_view>
#include <optional>

#include "virus/type-subtype.hh"

// ======================================================================

template <typename T> constexpr std::optional<T>&& operator|(std::optional<T>&& lhs, std::optional<T>&& rhs)
{
    if (lhs.has_value())
        return std::move(lhs);
    else
        return std::move(rhs);
}

// ======================================================================

namespace ae::sequences::detect
{
    using namespace std::string_view_literals;

    struct aligned_data_t
    {
        int aa_shift{0};
        ae::virus::type_subtype_t type_subtype;

        aligned_data_t(size_t offset, int add, std::string_view subtype) : aa_shift{static_cast<int>(offset) + add}, type_subtype{subtype} {}
    };

    std::optional<aligned_data_t> find_in_sequence(std::string_view sequence, size_t limit, std::initializer_list<std::string_view> look_for, int add, std::string_view subtype);
    std::string::size_type find_in_sequence(std::string_view sequence, size_t limit, std::initializer_list<std::string_view> look_for);
    bool has_infix(std::string_view source, size_t pos, std::string_view match);

    // ----------------------------------------------------------------------
    // H1
    // ----------------------------------------------------------------------

    namespace h1
    {
        inline std::optional<aligned_data_t> mkv(std::string_view not_aligned_aa)
        {
            if (const auto pos = find_in_sequence(not_aligned_aa, 20, {"MKV"sv, "MKA"sv, "MEA"sv, "MEV"sv});
                pos != std::string::npos && (has_infix(not_aligned_aa, pos + 17, "DTLC"sv) || has_infix(not_aligned_aa, pos + 17, "DTIC"sv)))
                return aligned_data_t{pos, 17, "A(H1)"sv};
            else
                return std::nullopt;
        }
    } // namespace h1

    // ----------------------------------------------------------------------
    // H3
    // ----------------------------------------------------------------------

    namespace h3
    {
        inline std::optional<aligned_data_t> mktii(std::string_view not_aligned_aa)
        {
            if (const auto pos = find_in_sequence(not_aligned_aa, 20, {"MKTII"sv}); pos != std::string::npos && (not_aligned_aa[pos + 16] == 'Q' || not_aligned_aa[pos + 15] == 'A'))
                // not_aligned_aa.substr(pos + 15, 2) != "DR") { // DR[ISV]C - start of the B sequence (signal peptide is 15 aas!)
                return aligned_data_t{pos, 16, "A(H3)"sv};
            else
                return std::nullopt;
        }
    } // namespace h3

    // ----------------------------------------------------------------------
    // B
    // ----------------------------------------------------------------------

    namespace b
    {
        inline std::optional<aligned_data_t> b(std::string_view not_aligned_aa)
        {
            const std::string_view B { "B" };
            return find_in_sequence(not_aligned_aa, 100, {"CTDL"}, -59, B) // Only B has CTDL at first 100 AAs
                | find_in_sequence(not_aligned_aa, 100, {"NSPHVV"}, -10, B)        // Only B has NSPHVV at first 100 AAs
                | find_in_sequence(not_aligned_aa, 150, {"EHIRL"}, -114, B)
                | find_in_sequence(not_aligned_aa, 250, {"CPNATS"}, -142, B)// Only B (YAMAGATA?) has CPNATS in whole AA sequence
                | find_in_sequence(not_aligned_aa, 250, {"PNATSK"}, -143, B)
                | find_in_sequence(not_aligned_aa, 150, {"NVTNG"}, -144, B) // VICTORIA?
                ;
       }

    } // namespace b

    // ----------------------------------------------------------------------
    // utils
    // ----------------------------------------------------------------------

    inline std::string::size_type find_in_sequence(std::string_view sequence, size_t limit, std::initializer_list<std::string_view> look_for)
    {
        if (sequence.size() > limit) {
            sequence.remove_suffix(sequence.size() - limit);
            for (const auto str : look_for) {
                if (const auto pos = sequence.find(str); pos != std::string::npos)
                    return pos;
            }
        }
        return std::string::npos;
    }

    inline bool has_infix(std::string_view source, size_t pos, std::string_view match) { return source.substr(pos, match.size()) == match; }

    inline std::optional<aligned_data_t> find_in_sequence(std::string_view sequence, size_t limit, std::initializer_list<std::string_view> look_for, int add, std::string_view subtype)
    {
        if (const auto pos = find_in_sequence(sequence, limit, look_for); pos != std::string::npos)
            return aligned_data_t{pos, add, subtype};
        else
            return std::nullopt;
    }

} // namespace ae::sequences::detect

// ======================================================================

// constexpr std::optional<ae::sequences::detect::aligned_data_t>&& operator|(std::optional<ae::sequences::detect::aligned_data_t>&& lhs, std::optional<ae::sequences::detect::aligned_data_t>&& rhs)
// {
//     if (lhs.has_value())
//         return std::move(lhs);
//     else
//         return std::move(rhs);
// }

// ======================================================================
