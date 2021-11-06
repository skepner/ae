
#include "sequences/align.hh"
#include "sequences/raw-sequence.hh"
#include "sequences/detect.hh"

// ======================================================================

inline void update_type_subtype(ae::sequences::RawSequence& sequence, const ae::sequences::detect::aligned_data_t& aligned_data)
{
    if (sequence.type_subtype.empty())
        sequence.type_subtype = aligned_data.type_subtype;
    else if (sequence.type_subtype.h_or_b() != aligned_data.type_subtype.h_or_b()) {
        if (sequence.type_subtype != ae::virus::type_subtype_t{"A(H0N0)"})
            fmt::print(">> updating type_subtype for \"{}\": {} ({} \"{}\") <- {}\n{}\n", sequence.name, aligned_data.type_subtype, aligned_data.aa_shift, aligned_data.detector, sequence.type_subtype, sequence.aa);
        sequence.type_subtype = aligned_data.type_subtype;
    }
}

// ======================================================================

bool ae::sequences::align(RawSequence& sequence)
{
    // if (sequence.name == "A/NINGBO/9/2021")
    //     fmt::print("{} {}\n{}\n", sequence.type_subtype, sequence.name, sequence.aa);

    const std::string_view not_aligned_aa{sequence.aa};
    auto aligned_data =                   //
        detect::h3::mktii(not_aligned_aa) //
        | detect::h1::mkv(not_aligned_aa) //
        | detect::b::b(not_aligned_aa) //
        | detect::hx::second_stage(not_aligned_aa) //
        | detect::h3::third_stage(not_aligned_aa) //
        | detect::h1::third_stage(not_aligned_aa) //
        | detect::hx::third_stage(not_aligned_aa) //
        ;

    if (aligned_data.has_value()) {
        update_type_subtype(sequence, *aligned_data);
        if (aligned_data->aa_shift < 0) {
            sequence.aa.add_prefix(- aligned_data->aa_shift);
            sequence.nuc.add_prefix(- aligned_data->aa_shift * 3);
            sequence.issues.set(issue::prefix_x);
        }
        else {
            sequence.aa.remove_prefix(aligned_data->aa_shift);
            sequence.nuc.remove_prefix(aligned_data->aa_shift * 3);
        }
        // if (aligned_data->type_subtype == ae::virus::type_subtype_t{"B"})
        //     fmt::print("{} \"{}\"\n{}\n{}\n\n", sequence.type_subtype, sequence.name, sequence.aa, sequence.nuc);
        return true;
    }
    else {
        sequence.issues.set(issue::not_aligned);
        if (!sequence.issues.is_set(issue::not_translated))
            fmt::print(">> not aligned {} {}\n{}\n", sequence.type_subtype, sequence.name, sequence.aa);
        return false;
    }
}

// ======================================================================
