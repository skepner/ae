
#include "sequences/align.hh"
#include "sequences/raw-sequence.hh"
#include "sequences/detect.hh"
#include "sequences/master.hh"

// ======================================================================

constexpr size_t hamming_distance_report_threshold{100};

// ======================================================================

inline void update_type_subtype(ae::sequences::RawSequence& sequence, const ae::sequences::detect::aligned_data_t& aligned_data)
{
    if (sequence.type_subtype.empty())
        sequence.type_subtype = aligned_data.type_subtype;
    else if (sequence.type_subtype.h_or_b() != aligned_data.type_subtype.h_or_b()) {
        const auto [subtype, hamming] = ae::sequences::closest_subtype_by_min_hamming_distance_to_master(sequence.aa);
        if (hamming < hamming_distance_report_threshold && subtype.h_or_b() != aligned_data.type_subtype.h_or_b())
            fmt::print(">> detected subtype {} does not correspond to the closest by hamming distance to master subtype {} {} for \"{}\"\n", aligned_data.type_subtype, subtype, hamming,
                       sequence.name);
        if (sequence.type_subtype.empty() || sequence.type_subtype.h_or_b() == "H0") {
            sequence.type_subtype = aligned_data.type_subtype;
        }
        else if (subtype.h_or_b() == aligned_data.type_subtype.h_or_b()) {
            fmt::print(">>> detected subtype {} is used (gisaid: {}), confirmed by closest hamming distance subtype {} {}, for \"{}\"\n", aligned_data.type_subtype, sequence.type_subtype, subtype,
                       hamming, sequence.name);
            sequence.type_subtype = aligned_data.type_subtype;
        }
        else {
            fmt::print(">>> gisaid subtype {} is used (detected: {}), confirmed by closest hamming distance subtype {} {}, for \"{}\"\n", sequence.type_subtype, aligned_data.type_subtype, subtype,
                       hamming, sequence.name);
        }
    }
}

// ======================================================================

bool ae::sequences::align(RawSequence& sequence)
{
    const std::string_view not_aligned_aa{sequence.aa};
    auto aligned_data =                            //
        detect::h3::mktii(not_aligned_aa)          //
        | detect::h1::mkv(not_aligned_aa)          //
        | detect::b::b(not_aligned_aa)             //
        | detect::hx::second_stage(not_aligned_aa) //
        | detect::h3::third_stage(not_aligned_aa)  //
        | detect::h1::third_stage(not_aligned_aa)  //
        | detect::hx::third_stage(not_aligned_aa)  //
        | detect::h1::end_ici(not_aligned_aa)      //
        | detect::h3::end_ici(not_aligned_aa)      //
        | detect::b::end_icl(not_aligned_aa)      //
        ;

    if (aligned_data.has_value()) {
        update_type_subtype(sequence, *aligned_data);
        if (aligned_data->aa_shift < 0) {
            sequence.aa.add_prefix(-aligned_data->aa_shift);
            sequence.nuc.add_prefix(-aligned_data->aa_shift * 3);
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
        // if (!sequence.issues.is_set(issue::not_translated))
        //     fmt::print(">> not aligned {} {}\n{}\n", sequence.type_subtype, sequence.name, sequence.aa);
        return false;
    }
}

// ======================================================================
