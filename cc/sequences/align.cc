
#include "sequences/align.hh"
#include "sequences/raw-sequence.hh"
#include "sequences/detect.hh"

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

bool ae::sequences::align(RawSequence& sequence)
{
    const std::string_view not_aligned_aa{sequence.aa};
    auto aligned_data =                   //
        detect::h3::mktii(not_aligned_aa) //
        | detect::h1::mkv(not_aligned_aa) //
        | detect::b::b(not_aligned_aa) //
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
        if (aligned_data->type_subtype == ae::virus::type_subtype_t{"B"})
            fmt::print("{} \"{}\"\n{}\n{}\n\n", sequence.type_subtype, sequence.name, sequence.aa, sequence.nuc);
        return true;
    }
    else {
        fmt::print(">> not aligned {} {}\n{}\n", sequence.type_subtype, sequence.name, sequence.aa);
        sequence.issues.set(issue::not_aligned);
        return false;
    }
}

// ======================================================================
