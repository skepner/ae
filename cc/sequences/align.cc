#include "utils/messages.hh"
#include "sequences/align.hh"
#include "sequences/raw-sequence.hh"
#include "sequences/detect.hh"
#include "sequences/master.hh"
#include "sequences/deletions.hh"

// ======================================================================

namespace ae::sequences
{
    constexpr size_t hamming_distance_report_threshold{100};

    // ======================================================================

    inline bool subtype_to_report(const ae::virus::type_subtype_t& s1, const ae::virus::type_subtype_t& s2)
    {
        using namespace std::string_view_literals;
        const auto our = [](std::string_view hb) { return hb == "B"sv || hb == "H1"sv || hb == "H3"sv; };
        return our(s1.h_or_b()) || our(s2.h_or_b());
    }

    // ======================================================================

    inline void update_type_subtype(RawSequence& sequence, const detect::aligned_data_t& aligned_data, Messages& messages)
    {
        if (sequence.type_subtype.empty())
            sequence.type_subtype = aligned_data.type_subtype;
        else if (sequence.type_subtype.h_or_b() != aligned_data.type_subtype.h_or_b()) {
            if (const auto [master, hamming] = closest_subtype_by_min_hamming_distance_to_master(sequence.aa); hamming < hamming_distance_report_threshold) {
                const auto master_h_b = master->type_subtype.h_or_b();
                if (master_h_b != aligned_data.type_subtype.h_or_b()) {
                    if (subtype_to_report(aligned_data.type_subtype, sequence.type_subtype))
                        messages.add(Message::subtype_mismatch, fmt::format("detected: {}", aligned_data.type_subtype),
                                     fmt::format("detected subtype {} does not correspond to the closest by hamming distance to master subtype {} {} for \"{}\"\nM: {}\nS: {}",
                                                 aligned_data.type_subtype, master->type_subtype, hamming, sequence.name, master->aa, sequence.aa));
                }
                if (sequence.type_subtype.empty() || sequence.type_subtype.h_or_b() == "H0") {
                    sequence.type_subtype = aligned_data.type_subtype;
                }
                else if (master_h_b == aligned_data.type_subtype.h_or_b()) {
                    // if (subtype_to_report(aligned_data.type_subtype, sequence.type_subtype) && hamming > 40)
                    //     fmt::print(">>> detected subtype {} is used (gisaid: {}), confirmed by closest hamming distance subtype {} {}, for \"{}\"\nS:  {}\nMD: {}\nMP: {}\n",
                    //     aligned_data.type_subtype,
                    //                sequence.type_subtype, master->type_subtype, hamming, sequence.name, sequence.aa, master->aa, master_sequence_for(sequence.type_subtype)->aa);
                    sequence.type_subtype = aligned_data.type_subtype;
                }
                else {
                    if (subtype_to_report(aligned_data.type_subtype, sequence.type_subtype))
                        messages.add(Message::subtype_mismatch, fmt::format("detected: {} provided: {}", aligned_data.type_subtype, sequence.type_subtype),
                                     fmt::format("provided subtype {} is used (detected: {}), confirmed by closest hamming distance subtype {} {}, for \"{}\"\nM: {}\nS: {}", sequence.type_subtype,
                                                 aligned_data.type_subtype, master->type_subtype, hamming, sequence.name, master->aa, sequence.aa));
                }
            }
            else {
                if (sequence.type_subtype.empty() || sequence.type_subtype.h_or_b() == "H0") {
                    // if (sequence.aa[pos0_t{0}] != 'X' && sequence.aa.size() > 500 && subtype_to_report(aligned_data.type_subtype, sequence.type_subtype))
                    //     fmt::print(">> detected subtype {} is used, provided {}, no reasonable master found\nS:  {}\nMD: {}\n", aligned_data.type_subtype, sequence.type_subtype, sequence.aa,
                    //     master_sequence_for(aligned_data.type_subtype)->aa);
                    sequence.type_subtype = aligned_data.type_subtype;
                }
                else {
                    if (sequence.aa[pos0_t{0}] != 'X' && sequence.aa.size() > 500 && subtype_to_report(aligned_data.type_subtype, sequence.type_subtype))
                        messages.add(Message::subtype_mismatch, fmt::format("detected: {} provided: {}", aligned_data.type_subtype, sequence.type_subtype),
                                     fmt::format("provided subtype {} is used, detected {}, no reasonable master found\nS:  {}\nMP: {}\nMD: {}", sequence.type_subtype, aligned_data.type_subtype,
                                                 sequence.aa, master_sequence_for(sequence.type_subtype)->aa, master_sequence_for(aligned_data.type_subtype)->aa));
                }
            }
        }
    }

} // namespace ae::sequences

// ======================================================================

bool ae::sequences::align(RawSequence& sequence, Messages& messages)
{
    const std::string_view not_aligned_aa{sequence.aa};
    std::optional<detect::aligned_data_t> aligned_data;
    detect::h3::mktii(not_aligned_aa, aligned_data)               //
        || detect::h1::mkv(not_aligned_aa, aligned_data)          //
        || detect::b::b(not_aligned_aa, aligned_data)             //
        || detect::hx::second_stage(not_aligned_aa, aligned_data) //
        || detect::h3::third_stage(not_aligned_aa, aligned_data)  //
        || detect::h1::third_stage(not_aligned_aa, aligned_data)  //
        || detect::hx::third_stage(not_aligned_aa, aligned_data)  //
        || detect::h1::end_ici(not_aligned_aa, aligned_data)      //
        || detect::h3::end_ici(not_aligned_aa, aligned_data)      //
        || detect::b::end_icl(not_aligned_aa, aligned_data)       //
        ;

    if (aligned_data.has_value()) {
        if (aligned_data->aa_shift < 0) {
            sequence.aa.add_prefix(-aligned_data->aa_shift);
            sequence.nuc.add_prefix(-aligned_data->aa_shift * 3);
            sequence.issues.set(issue::prefix_x);
        }
        else {
            sequence.aa.remove_prefix(aligned_data->aa_shift);
            sequence.nuc.remove_prefix(aligned_data->aa_shift * 3);
        }
        update_type_subtype(sequence, *aligned_data, messages); // after adjusting sequence.aa!
        find_deletions_insertions_set_lineage(sequence);
        if (const auto s_length = sequence.aa.size(), m_length = ha_sequence_length_for(sequence.type_subtype); s_length < m_length) {
            sequence.issues.set(issue::too_short);
        }
        else if (s_length > m_length) {
            // TODO truncate
            sequence.issues.set(issue::too_long);
        }
        if (sequence.aa.number_of_x() > 10)
            sequence.issues.set(issue::too_many_x);
        if (sequence.aa.number_of_deletions() > 6)
            sequence.issues.set(issue::too_many_deletions);
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
