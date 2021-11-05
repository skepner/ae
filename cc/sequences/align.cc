#include <optional>

#include "sequences/align.hh"
#include "sequences/raw-sequence.hh"

// ======================================================================

template <typename T> inline std::optional<T>& operator||(std::optional<T>& lhs, std::optional<T>& rhs)
{
    if (lhs.has_value())
        return lhs;
    else
        return rhs;
}

// ======================================================================

struct aligned_data_t
{
    int aa_shift{0};
    ae::virus::type_subtype_t type_subtype;
};

static std::optional<aligned_data_t> check_signal_peptide(const ae::sequences::RawSequence& sequence);
static std::optional<aligned_data_t> check_specific_part(const ae::sequences::RawSequence& sequence);

// ======================================================================

inline ae::virus::type_subtype_t make_type_subtype(const ae::sequences::RawSequence& sequence, std::string_view detected_type_subtype)
{
    const auto dts = ae::virus::type_subtype_t{detected_type_subtype};
    if (sequence.type_subtype.h_or_b() == dts.h_or_b())
        return sequence.type_subtype;
    else
        return dts;
}

// ======================================================================

bool ae::sequences::align(RawSequence& sequence)
{
    auto aligned_data = check_signal_peptide(sequence) || check_specific_part(sequence);
    return true;
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
