#include <optional>

#include "sequences/align.hh"
#include "sequences/raw-sequence.hh"

// ======================================================================

// ======================================================================

struct aligned_data_t
{
    int aa_shift{0};
    ae::virus::type_subtype_t type_subtype;

    aligned_data_t(size_t offset, int add, std::string_view subtype) : aa_shift{static_cast<int>(offset) + add}, type_subtype{subtype} {}
};

constexpr std::optional<aligned_data_t>&& operator|(std::optional<aligned_data_t>&& lhs, std::optional<aligned_data_t>&& rhs)
{
    if (lhs.has_value())
        return std::move(lhs);
    else
        return std::move(rhs);
}


static std::optional<aligned_data_t> check_signal_peptide(const ae::sequences::RawSequence& sequence);
static std::optional<aligned_data_t> check_specific_part(const ae::sequences::RawSequence& sequence);

// ======================================================================

inline void update_type_subtype(ae::sequences::RawSequence& sequence, const ae::virus::type_subtype_t& detected_type_subtype)
{
    if (sequence.type_subtype.empty())
        sequence.type_subtype = detected_type_subtype;
    else if (sequence.type_subtype.h_or_b() != detected_type_subtype.h_or_b()) {
        if (sequence.type_subtype != ae::virus::type_subtype_t{"A(H0N0)"})
            fmt::print(">> updating type_subtype for \"{}\": {} <- {}\n", sequence.name, detected_type_subtype, sequence.type_subtype);
        sequence.type_subtype = detected_type_subtype;
    }
}

// ======================================================================

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

using namespace std::string_view_literals;

namespace ae::sequences::detect::h3
{
    inline std::optional<aligned_data_t> mktii(std::string_view not_aligned_aa)
    {
        if (const auto pos = find_in_sequence(not_aligned_aa, 20, {"MKTII"sv}); pos != std::string::npos && (not_aligned_aa[pos + 16] == 'Q' || not_aligned_aa[pos + 15] == 'A'))
            // not_aligned_aa.substr(pos + 15, 2) != "DR") { // DR[ISV]C - start of the B sequence (signal peptide is 15 aas!)
            return aligned_data_t{pos, 16, "A(H3)"sv};
        else
            return std::nullopt;
    }

} // namespace ae::sequence::detect::h3

// ======================================================================

bool ae::sequences::align(RawSequence& sequence)
{
    const std::string_view not_aligned_aa{sequence.aa};
    std::optional<aligned_data_t> aligned_data =                   //
        detect::h3::mktii(not_aligned_aa) //
        | check_signal_peptide(sequence) //
        | check_specific_part(sequence)  //
        ;

    if (aligned_data.has_value()) {
        update_type_subtype(sequence, aligned_data->type_subtype);
        if (aligned_data->aa_shift < 0) {
            sequence.aa.add_prefix(aligned_data->aa_shift);
            sequence.nuc.add_prefix(aligned_data->aa_shift * 3);
        }
        else {
            sequence.aa.remove_prefix(aligned_data->aa_shift);
            sequence.nuc.remove_prefix(aligned_data->aa_shift * 3);
        }
        fmt::print("{} \"{}\"\n{}\n{}\n\n", sequence.type_subtype, sequence.name, sequence.aa, sequence.nuc);
        return true;
    }
    else
        return false;
}

// ======================================================================

std::optional<aligned_data_t> check_signal_peptide(const ae::sequences::RawSequence& sequence)
{
    return std::nullopt;

} // check_signal_peptide

// ----------------------------------------------------------------------

std::optional<aligned_data_t> check_specific_part(const ae::sequences::RawSequence& sequence)
{
    return std::nullopt;

} // check_specific_part

// ----------------------------------------------------------------------
