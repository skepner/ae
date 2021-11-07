#include <array>

#include "sequences/master.hh"
#include "sequences/hamming-distance.hh"

// ======================================================================

namespace ae::sequences
{
    using namespace std::string_view_literals;;
    using namespace ae::virus;
    using SA = sequence_aa_t;
    using TS = type_subtype_t;

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

    static const std::array master_sequence_data{
        MasterSequence{TS{"B"}, "B/BRISBANE/60/2008"sv, SA{"DRICTGITSSNSPHVVKTATQGEVNVTGVIPLTTTPTKSHFANLKGTETRGKLCPKCLNCTDLDVALGRPKCTGKIPSARVSILHEVRPVTSGCFPIMHDRTKIRQLPNLLRGYEHIRLSTHNVINAENAPGGPYKIGTSGSCPNITNGNGFFATMAWAVPKNDKNKTATNPLTIEVPYICTEGEDQITVWGFHSDNETQMAKLYGDSKPQKFTSSANGVTTHYVSQIGGFPNQTEDGGLPQSGRIVVDYMVQKSGKTGTITYQRGILLPQKVWCASGRSKVIKGSLPLIGEADCLHEKYGGLNKSKPYYTGEHAKAIGNCPIWVKTPLKLANGTKYRPPAKLLKERGFFGAIAGFLEGGWEGMIAGWHGYTSHGAHGVAVAADLKSTQEAINKITKNLNSLSELEVKNLQRLSGAMDELHNEILELDEKVDDLRADTISSQIELAVLLSNEGIINSEDEHLLALERKLKKMLGPSAVEIGNGCFETKHKCNQTCLDRIAAGTFDAGEFSLPTFDSLNITAASLNDDGLDNHTILLYYSTAASSLAVTLMIAIFVVYMVSRDNVSCSICL"}},

        MasterSequence{TS{"A(H1N1)"}, "A(H1N1)/CALIFORNIA/7/2009"sv, SA{"DTLCIGYHANNSTDTVDTVLEKNVTVTHSVNLLEDKHNGKLCKLRGVAPLHLGKCNIAGWILGNPECESLSTASSWSYIVETPSSDNGTCYPGDFIDYEELREQLSSVSSFERFEIFPKTSSWPNHDSNKGVTAACPHAGAKSFYKNLIWLVKKGNSYPKLSKSYINDKGKEVLVLWGIHHPSTSADQQSLYQNADAYVFVGSSRYSKKFKPEIAIRPKVRDQEGRMNYYWTLVEPGDKITFEATGNLVVPRYAFAMERNAGSGIIISDTPVHDCNTTCQTPKGAINTSLPFQNIHPITIGKCPKYVKSTKLRLATGLRNIPSIQSRGLFGAIAGFIEGGWTGMVDGWYGYHHQNEQGSGYAADLKSTQNAIDEITNKVNSVIEKMNTQFTAVGKEFNHLEKRIENLNKKVDDGFLDIWTYNAELLVLLENERTLDYHDSNVKNLYEKVRSQLKNNAKEIGNGCFEFYHKCDNTCMESVKNGTYDYPKYSEEAKLNREEIDGVKLESTRIYQILAIYSTVASSLVLVVSLGAISFWMCSNGSLQCRICI"}},

        MasterSequence{TS{"A(H3N2)"}, "A(H3N2)/HONG_KONG/1/1968"sv, SA{"QDLPGNDNSTATLCLGHHAVPNGTLVKTITDDQIEVTNATELVQSSSTGKICNNPHRILDGIDCTLIDALLGDPHCDVFQNETWDLFVERSKAFSNCYPYDVPDYASLRSLVASSGTLEFITEGFTWTGVTQNGGSNACKRGPGSGFFSRLNWLTKSGSTYPVLNVTMPNNDNFDKLYIWGVHHPSTNQEQTSLYVQASGRVTVSTRRSQQTIIPNIGSRPWVRGLSSRISIYWTIVKPGDVLVINSNGNLIAPRGYFKMRTGKSSIMRSDAPIDTCISECITPNGSIPNDKPFQNVNKITYGACPKYVKQNTLKLATGMRNVPEKQTRGLFGAIAGFIENGWEGMIDGWYGFRHQNSEGTGQAADLKSTQAAIDQINGKLNRVIEKTNEKFHQIEKEFSEVEGRIQDLEKYVEDTKIDLWSYNAELLVALENQHTIDLTDSEMNKLFEKTRRQLRENAEDMGNGCFKIYHKCDNACIESIRNGTYDHDVYRDEALNNRFQIKGVELKSGYKDWILWISFAISCFLLCVVLLGFIMWACQRGNIRCNICI"}},
    };

#pragma GCC diagnostic pop
}

// ======================================================================

std::vector<const ae::sequences::MasterSequence*> ae::sequences::master_sequences(const type_subtype_t& ts)
{
    std::vector<const MasterSequence*> result;
    for (const auto& seq : master_sequence_data) {
        if (seq.type_subtype == ts)
            result.push_back(&seq);
    }
    return result;

} // ae::sequences::master_sequences

// ----------------------------------------------------------------------

size_t ae::sequences::min_hamming_distance_to_master(const type_subtype_t& ts, const sequence_aa_t& source)
{
    size_t min_dist{source.size()};
    for (const auto& seq : master_sequence_data) {
        if (seq.type_subtype == ts)
            min_dist = std::min(min_dist, hamming_distance(seq.aa, source, hamming_distance_by_shortest::yes));
    }
    return min_dist;

} // ae::sequences::min_hamming_distance_to_master

// ----------------------------------------------------------------------

std::pair<ae::sequences::type_subtype_t, size_t> ae::sequences::closest_subtype_by_min_hamming_distance_to_master(const sequence_aa_t& source)
{
    std::pair result{type_subtype_t{}, source.size()};
    for (const auto& seq : master_sequence_data) {
        const auto hd = hamming_distance(seq.aa, source, hamming_distance_by_shortest::yes);
        if (hd < result.second) {
            result.first = seq.type_subtype;
            result.second = hd;
        }
    }
    return result;

} // ae::sequences::closest_subtype_by_min_hamming_distance_to_master

// ----------------------------------------------------------------------
