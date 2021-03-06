#pragma once

#include <string_view>
#include <optional>

#include "utils/string.hh"
#include "utils/log.hh"
#include "virus/type-subtype.hh"

// ======================================================================

// template <typename T> constexpr std::optional<T>&& operator|(std::optional<T>&& lhs, std::optional<T>&& rhs)
// {
//     if (lhs.has_value())
//         return std::move(lhs);
//     else
//         return std::move(rhs);
// }

// ======================================================================

namespace ae::sequences::detect
{
    using namespace std::string_view_literals;

    struct aligned_data_t
    {
        int aa_shift{0};
        ae::virus::type_subtype_t type_subtype;
        std::string_view detector;

        aligned_data_t(size_t offset, int add, std::string_view subtype, std::string_view detectr) : aa_shift{static_cast<int>(offset) + add}, type_subtype{subtype}, detector{detectr} {}
    };

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

    inline bool find_in_sequence(std::string_view sequence, size_t limit, std::initializer_list<std::string_view> look_for, int add, std::string_view subtype,
                                 std::optional<aligned_data_t>& aligned_data)
    {
        if (const auto pos = find_in_sequence(sequence, limit, look_for); pos != std::string::npos) {
            aligned_data = aligned_data_t{pos, add, subtype, *look_for.begin()};
            return true;
        }
        else
            return false;
    }

    inline bool has_infix(std::string_view source, size_t pos, std::string_view match) { return source.substr(pos, match.size()) == match; }

    // ----------------------------------------------------------------------

    constexpr const std::string_view H1{"A(H1)"};
    constexpr const std::string_view H3{"A(H3)"};
    constexpr const std::string_view B{"B"};

    constexpr int H1_sequence_aa_size = 549;
    constexpr int H3_sequence_aa_size = 550;
    constexpr int B_sequence_aa_size = 570;

    // ----------------------------------------------------------------------
    // H1
    // ----------------------------------------------------------------------

    namespace h1
    {
        inline bool mkv(std::string_view not_aligned_aa, std::optional<aligned_data_t>& aligned_data)
        {
            if (const auto pos = find_in_sequence(not_aligned_aa, 20, {"MKV"sv, "MKA"sv, "MEA"sv, "MEV"sv});
                pos != std::string::npos && (has_infix(not_aligned_aa, pos + 17, "DTLC"sv) || has_infix(not_aligned_aa, pos + 17, "DTIC"sv))) {
                aligned_data = aligned_data_t{pos, 17, H1, "MKV-MKA-MEA-MEV"sv};
                return true;
            }
            else
                return false;
        }

        inline bool third_stage(std::string_view not_aligned_aa, std::optional<aligned_data_t>& aligned_data)
        {
            find_in_sequence(not_aligned_aa, 50, {"VLEKN"sv}, -18, H1, aligned_data)                      // VLEKN is H1 specific (whole AA sequence)
                || find_in_sequence(not_aligned_aa, 150, {"SSWSYI"sv, "ESWSYI"sv}, -73, H1, aligned_data) // SSWSYI and ESWSYI are H1 specific (whole AA sequence)
                || find_in_sequence(not_aligned_aa, 150, {"FERFEI"sv}, -110, H1, aligned_data)            //
                || find_in_sequence(not_aligned_aa, 200, {"IWLVKKG"sv}, -148, H1, aligned_data)           //
                || find_in_sequence(not_aligned_aa, 200, {"SSVSSF"sv}, -105, H1, aligned_data)            //
                ;
            return aligned_data.has_value();
        }

        inline bool end_ici(std::string_view not_aligned_aa, std::optional<aligned_data_t>& aligned_data)
        {
            const std::string_view LQCRICI{"LQCRICI"};
            if (ae::string::endswith(not_aligned_aa, LQCRICI) && (not_aligned_aa.size() < H1_sequence_aa_size || not_aligned_aa.substr(not_aligned_aa.size() - H1_sequence_aa_size, 2) == "DT"sv)) {
                aligned_data = aligned_data_t{not_aligned_aa.size(), -H1_sequence_aa_size, H1, LQCRICI};
                return true;
            }
            else
                return false;
        }

    } // namespace h1

    // ----------------------------------------------------------------------
    // H3
    // ----------------------------------------------------------------------

    namespace h3
    {
        inline bool mktii(std::string_view not_aligned_aa, std::optional<aligned_data_t>& aligned_data)
        {
            if (const auto pos2 = find_in_sequence(not_aligned_aa, 30, {"MKTIIALSNILCLVFA"sv}); pos2 != std::string::npos && not_aligned_aa[pos2 + 17] == 'Q' && not_aligned_aa[pos2 + 16] != 'Q') {
                aligned_data = aligned_data_t{pos2, 17, H3, "MKTIIALSNILCLVFA-17"sv};
                // AD_DEBUG("{}", not_aligned_aa);
                return true;
            }
            else if (const auto pos = find_in_sequence(not_aligned_aa, 20, {"MKTII"sv}); pos != std::string::npos && (not_aligned_aa[pos + 16] == 'Q' || not_aligned_aa[pos + 15] == 'A')) {
                // not_aligned_aa.substr(pos + 15, 2) != "DR") { // DR[ISV]C - start of the B sequence (signal peptide is 15 aas!)
                aligned_data = aligned_data_t{pos, 16, H3, "MKTII"sv};
                return true;
            }
            else
                return false;
        }

        inline bool third_stage(std::string_view not_aligned_aa, std::optional<aligned_data_t>& aligned_data)
        {
            find_in_sequence(not_aligned_aa, 150, {"CTLID"sv, "CTLMDALL"sv, "CTLVD"sv}, -63, H3, aligned_data) // Only H3 (and H0N0) has CTLID in the whole AA sequence
                || find_in_sequence(not_aligned_aa, 100, {"PNGTIVKTI"sv}, -20, H3, aligned_data)               // Only H3 (and H0N0) has PNGTIVKTI in the whole AA sequence
                || find_in_sequence(not_aligned_aa, 200, {"DKLYIWG"sv}, -174, H3, aligned_data)                // Only H3 (and H0N0) has DKLYIWG in the whole AA sequence
                || find_in_sequence(not_aligned_aa, 150, {"SNCYPYDV"sv}, -94, H3, aligned_data)                //
                ;
            return aligned_data.has_value();
        }

        inline bool end_ici(std::string_view not_aligned_aa, std::optional<aligned_data_t>& aligned_data)
        {
            const std::string_view IRCNICI{"IRCNICI"};
            if (ae::string::endswith(not_aligned_aa, IRCNICI)) {
                aligned_data = aligned_data_t{not_aligned_aa.size(), -H3_sequence_aa_size, H3, IRCNICI};
                return true;
            }
            else
                return false;
        }

    } // namespace h3

    // ----------------------------------------------------------------------
    // B
    // ----------------------------------------------------------------------

    namespace b
    {
        inline bool b(std::string_view not_aligned_aa, std::optional<aligned_data_t>& aligned_data)
        {
            find_in_sequence(not_aligned_aa, 20, {"MKAIIVLLM"sv}, 15, B, aligned_data)        //
                || find_in_sequence(not_aligned_aa, 100, {"CTDL"sv}, -59, B, aligned_data)    // Only B has CTDL at first 100 AAs
                || find_in_sequence(not_aligned_aa, 100, {"NSPHVV"sv}, -10, B, aligned_data)  // Only B has NSPHVV at first 100 AAs
                || find_in_sequence(not_aligned_aa, 150, {"EHIRL"sv}, -114, B, aligned_data)  //
                || find_in_sequence(not_aligned_aa, 250, {"CPNATS"sv}, -142, B, aligned_data) // Only B (YAMAGATA?) has CPNATS in whole AA sequence
                || find_in_sequence(not_aligned_aa, 250, {"PNATSK"sv}, -143, B, aligned_data) //
                || find_in_sequence(not_aligned_aa, 150, {"NVTNG"sv}, -144, B, aligned_data)  // VICTORIA?
                ;
            return aligned_data.has_value();
        }

        inline bool end_icl(std::string_view not_aligned_aa, std::optional<aligned_data_t>& aligned_data)
        {
            const std::string_view SCSICL{"SCSICL"};
            if (ae::string::endswith(not_aligned_aa, SCSICL)) {
                aligned_data = aligned_data_t{not_aligned_aa.size(), -B_sequence_aa_size, B, SCSICL};
                return true;
            }
            else
                return false;
        }

    } // namespace b

    // ----------------------------------------------------------------------
    // Hx
    // ----------------------------------------------------------------------

    namespace hx
    {
        inline bool h2_MTIT(std::string_view not_aligned_aa, std::optional<aligned_data_t>& aligned_data)
        {
            if (const auto pos = find_in_sequence(not_aligned_aa, 20, {"MTIT"sv, "MAII"sv, "MTII"sv}); pos != std::string::npos && has_infix(not_aligned_aa, pos + 14, "GDQIC"sv)) {
                aligned_data = aligned_data_t{pos, 15, "A(H2)"sv, "MTIT"sv};
                return true;
            }
            else
                return false;
        }

        inline bool h4_MLS(std::string_view not_aligned_aa, std::optional<aligned_data_t>& aligned_data)
        {
            if (const auto pos = find_in_sequence(not_aligned_aa, 20, {"MLS"sv}); pos != std::string::npos && (not_aligned_aa[pos + 16] == 'Q' || has_infix(not_aligned_aa, pos + 16, "SQNY"sv))) {
                aligned_data = aligned_data_t{pos, 16, "A(H4)"sv, "MLS"sv};
                return true;
            }
            else
                return false;
        }

        inline bool h7_MNIQ(std::string_view not_aligned_aa, std::optional<aligned_data_t>& aligned_data)
        {
            if (const auto pos = find_in_sequence(not_aligned_aa, 20, {"MNIQ"sv, "MNNQ"sv, "MNTQ"sv});
                pos != std::string::npos && not_aligned_aa[pos + 17] != 'S' && has_infix(not_aligned_aa, pos + 18, "DKIC"sv)) { // SDKIC is H15 most probably
                aligned_data = aligned_data_t{pos, 18, "A(H7)"sv, "MNIQ"sv};
                return true;
            }
            else
                return false;
        }

        inline bool h8_MEKFIA(std::string_view not_aligned_aa, std::optional<aligned_data_t>& aligned_data)
        {
            if (const auto pos = find_in_sequence(not_aligned_aa, 20, {"MEKFIA"sv}); pos != std::string::npos && not_aligned_aa[pos + 17] == 'D') {
                aligned_data = aligned_data_t{pos, 17, "A(H8)"sv, "MEKFIA"sv};
                return true;
            }
            else
                return false;
        }

        inline bool find_in_sequence_infix(std::string_view sequence, size_t limit, std::initializer_list<std::string_view> look_for, int add, std::string_view subtype, size_t infix_offset,
                                           std::string_view infix, std::optional<aligned_data_t>& aligned_data)
        {
            if (const auto pos = find_in_sequence(sequence, limit, look_for); pos != std::string::npos && has_infix(sequence, pos + infix_offset, infix)) {
                aligned_data = aligned_data_t{pos, add, subtype, *look_for.begin()};
                return true;
            }
            else
                return false;
        }

        inline bool second_stage(std::string_view not_aligned_aa, std::optional<aligned_data_t>& aligned_data)
        {
            h2_MTIT(not_aligned_aa, aligned_data)                                                                                        //
                || h4_MLS(not_aligned_aa, aligned_data)                                                                                  //
                || find_in_sequence(not_aligned_aa, 20, {"MEKIV", "MERIV"sv, "MKKIV"sv}, 16, "A(H5)"sv, aligned_data)                    //
                || find_in_sequence(not_aligned_aa, 20, {"MIAIIV"sv, "MIAIII"sv}, 16, "A(H6)"sv, aligned_data)                           //
                || h7_MNIQ(not_aligned_aa, aligned_data)                                                                                 //
                || h8_MEKFIA(not_aligned_aa, aligned_data)                                                                               //
                || find_in_sequence_infix(not_aligned_aa, 20, {"MET"sv, "MEIIS"sv, "MEV"sv}, 18, "A(H9)"sv, 17, "ADKIC"sv, aligned_data) //
                || find_in_sequence(not_aligned_aa, 20, {"MYK"sv}, 17, "A(H10)"sv, aligned_data)                                         //
                || find_in_sequence_infix(not_aligned_aa, 20, {"MK"sv, "MEKTLL"sv}, 16, "A(H11)"sv, 16, "DEIC"sv, aligned_data)          //
                || find_in_sequence_infix(not_aligned_aa, 20, {"MEKFIIL"sv}, 17, "A(H12)"sv, 15, "AYDKIC"sv, aligned_data)               //
                || find_in_sequence_infix(not_aligned_aa, 20, {"MDI"sv, "MAL"sv, "MEV"sv}, 18, "A(H13)"sv, 17, "ADRIC"sv, aligned_data)  //
                || find_in_sequence_infix(not_aligned_aa, 20, {"MIA"sv}, 17, "A(H14)"sv, 14, "AYSQITN"sv, aligned_data)                  //
                || find_in_sequence_infix(not_aligned_aa, 20, {"MMVK"sv, "MMIK"sv}, 19, "A(H16)"sv, 19, "DKIC"sv, aligned_data)          //
                || find_in_sequence_infix(not_aligned_aa, 20, {"MEL"sv}, 18, "A(H17)"sv, 17, "GDRICI"sv, aligned_data)                   //
                || find_in_sequence_infix(not_aligned_aa, 100, {"QNYT"sv}, 0, "A(H4)"sv, 11, "GHHA"sv, aligned_data)                     //
                || find_in_sequence(not_aligned_aa, 50, {"DEICIGYL"sv}, 0, "A(H11)"sv, aligned_data)                                     // H11 (DEICIGYL is specific)
                || find_in_sequence(not_aligned_aa, 100, {"KSDKICLGHHA"sv}, 2, "A(H15)"sv, aligned_data)                                 //
                ;
            return aligned_data.has_value();
        }

        inline bool third_stage(std::string_view not_aligned_aa, std::optional<aligned_data_t>& aligned_data)
        {
            find_in_sequence(not_aligned_aa, 100, {"GVKPLIL"sv, "GVRPLIL"sv}, -45, "A(H5)"sv, aligned_data)            //
                || find_in_sequence(not_aligned_aa, 100, {"GWLLGNPMCDE"sv}, -58, "A(H5)"sv, aligned_data)              //
                || find_in_sequence(not_aligned_aa, 150, {"NHFE"sv}, -108, "A(H5)"sv, aligned_data)                    // specific at first 150
                || find_in_sequence(not_aligned_aa, 100, {"QKEER"sv}, -35, "A(H6)"sv, aligned_data)                    // QKEER is H6 specific
                || find_in_sequence(not_aligned_aa, 150, {"EELKA"sv}, -98, "A(H6)"sv, aligned_data)                    // EELKA is H6 specific
                || find_in_sequence(not_aligned_aa, 100, {"GQCGL"sv}, -51, "A(H7)"sv, aligned_data)                    //
                || find_in_sequence(not_aligned_aa, 200, {"FYRSINWL"sv}, -141, "A(H8)"sv, aligned_data)                //
                || find_in_sequence(not_aligned_aa, 50, {"QSTN"sv}, -7, "A(H9)"sv, aligned_data)                       // QSTN is H9 specific
                || find_in_sequence(not_aligned_aa, 150, {"CDLLLGG"sv, "CDLLLEG"sv}, -66, "A(H9)"sv, aligned_data)     // CDLLLGG, CDLLLEG are H9 specific
                || find_in_sequence(not_aligned_aa, 150, {"LEELRS"sv}, -97, "A(H9)"sv, aligned_data)                   // LEELRS is H9 specific
                || find_in_sequence(not_aligned_aa, 150, {"SARSYQ"sv}, -106, "A(H9)"sv, aligned_data)                  // SARSYQ is H9 specific
                || find_in_sequence(not_aligned_aa, 150, {"SSYQRIQ"sv}, -108, "A(H9)"sv, aligned_data)                 // SSYQRIQ is H9 specific
                || find_in_sequence(not_aligned_aa, 50, {"NGTIVKTLTNE"sv}, -11, "A(H10)"sv, aligned_data)              //
                || find_in_sequence(not_aligned_aa, 150, {"QKIMESG"sv}, -99, "A(H10)"sv, aligned_data)                 //
                || find_in_sequence(not_aligned_aa, 100, {"SSVEL"sv}, -27, "A(H11)"sv, aligned_data)                   // H11 (SSVEL is specific)
                || find_in_sequence(not_aligned_aa, 50, {"VGYLSTN"sv}, -4, "A(H13)"sv, aligned_data)                   // H13 (specific)
                || find_in_sequence(not_aligned_aa, 70, {"DTLTENGVP"sv, "DTLIENGVP"sv}, -16, "A(H16)"sv, aligned_data) // H16 (specific)
                ;
            return aligned_data.has_value();
        }

    } // namespace hx

} // namespace ae::sequences::detect

// ======================================================================
