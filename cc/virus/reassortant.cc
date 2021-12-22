#include <array>

#include "utils/log.hh"
#include "utils/regex.hh"
#include "utils/string.hh"
#include "virus/reassortant.hh"

// ----------------------------------------------------------------------

std::tuple<ae::virus::Reassortant, std::string> ae::virus::reassortant::parse(std::string_view source)
{
    using namespace ae::regex;

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif

#define PR_BOL "^"
#define PR_LOOKAHEAD_NOT_PAREN_SPACE_DASH "(?=[^\\(\\s\\-])"
#define PR_LOOKAHEAD_NOT_PAREN_SPACE "(?=[^\\(\\s])"
#define PR_AB_REASSORTANT "(?:A(?:\\(H\\d+(?:N\\d+)?\\))?|B)/RES?ASSORTANT/"
#define PR_HG_REASSORTANT "HIGHGROWTH\\s+RES?ASSORTANT\\s+"
#define PR_AB "[AB]/"

#define PR_PREFIX_1 "(?:" PR_BOL "|" PR_BOL PR_AB_REASSORTANT "|" PR_BOL PR_AB "|" PR_HG_REASSORTANT "|" PR_LOOKAHEAD_NOT_PAREN_SPACE_DASH ")"
// #define PREFIX_2 "(?:" BOL "|" BOL AB_REASSORTANT "|" LOOKAHEAD_NOT_PAREN_SPACE ")"

#define PR_NUMBER     "[\\-\\s]?(\\d+[A-Z\\d\\-]*)\\b"
#define PR_NYMC_X     "(?:NYMC[\\s\\-]*)?X"
#define PR_NYMC_X_CL  PR_AB_REASSORTANT "NYMC\\s+X[\\-\\s]+(\\d+)\\s+CL[\\-\\s]+(\\d+)" // A/reassortant/NYMC X-157 CL-3(New York/55/2004 x Puerto Rico/8/1934)(H3N2)
#define PR_NYMC       PR_PREFIX_1 "(?:NYMC[\\s\\-]B?X|BX|NYMC)" PR_NUMBER
#define PR_NYMCX_0    "X" PR_NUMBER
#define PR_NYMCX_1    "^" PR_NYMCX_0
#define PR_NYMCX_2_1  PR_AB_REASSORTANT PR_NYMC_X PR_NUMBER "\\s*\\((?:A/)?\\s*" PR_NYMC_X PR_NUMBER "\\s+X\\s+" // double reassortant A/reassortant/NYMC X-179(NYMC X-157 x A/California/07/2009)(H1N1)
#define PR_NYMCX_2_2  PR_AB_REASSORTANT PR_NYMC_X PR_NUMBER "(.+)" "\\s+X\\s+" PR_NYMC_X PR_NUMBER // double reassortant A(H1N1)/REASSORTANT/X-83(CHILE/1/1983 X X-31)
#define PR_NYMCX_3    PR_AB_REASSORTANT "X" PR_NUMBER
#define PR_NYMCX_4    "([\\s_])" PR_NYMCX_0
#define PR_NYMCX_5    PR_PREFIX_1 PR_NYMCX_0
#define PR_CBER       "(?:CBER|BVR)" PR_NUMBER // Center for Biologics Evaluation and Research https://www.fda.gov/about-fda/fda-organization/center-biologics-evaluation-and-research-cber
#define PR_IDCDC      "(?:PR8[\\- ]*IDCDC[\\- _]*|I[DB]CDC-)?RG[\\- ]*([\\dA-Z\\.]+)"
#define PR_NIB        "NIB(?:SC|RG)?" PR_NUMBER
#define PR_IVR        "(IVR|CVR)" PR_NUMBER // IVR-153 (A(H1N1)/California/7/2009) is by CSL, CVR - by CSL/Seqirus
#define PR_IVR_2      PR_AB_REASSORTANT "(IVR|CVR)" PR_NUMBER "(.+)\\s+X\\s+" "(IVR|CVR)" PR_NUMBER // A/resassortant/IVR-153(A/California/07/2009 x IVR-6)(H1N1)
#define PR_MELB       "(PR8)[-_\\s]*(?:HY)?"             // MELB (Malet) reassortant spec, e.g. "A/DRY VALLEYS/1/2020_PR8-HY"
#define PR_CNIC        "(CNIC)" PR_NUMBER // CNIC-2006(B/Sichuan-Jingyang/12048/2019) in CDC B/Vic
#define PR_IGY        "(IGYRP\\d+(?:\\.C\\d+)?)" // A/reassortant/IgYRP13.c1(California/07/2004 x Puerto Rico/8/1934)
#define PR_CDC        PR_AB_REASSORTANT "(CDC\\d+)"
#define PR_BS         PR_AB_REASSORTANT "(BS)" // A/reassortant/BS(Philippines/2/1982 x Puerto Rico/8/1934)(H3N2)
#define PR_SAN        PR_PREFIX_1 "SAN" PR_NUMBER // VIDRL H3: SAN-007 (A/Tasmania/503/2020)

    static const std::array normalize_data{
        look_replace_t{std::regex(PR_NYMCX_2_1,         std::regex::icase), {"NYMC-$1 NYMC-$2", "$` ($'"}}, // must be before PR_NYMC
        look_replace_t{std::regex(PR_NYMCX_2_2,         std::regex::icase), {"NYMC-$1 NYMC-$3", "$` $2 $'"}}, // must be before PR_NYMC
        look_replace_t{std::regex(PR_NYMC_X_CL,         std::regex::icase), {"NYMC-$1 CL-$2", "$` $'"}}, // before PR_NYMC
        look_replace_t{std::regex(PR_NYMC,              std::regex::icase), {"NYMC-$1", "$` $'"}},
        look_replace_t{std::regex(PR_NYMCX_1,           std::regex::icase), {"NYMC-$1", "$` $'"}},
        look_replace_t{std::regex(PR_NYMCX_3,           std::regex::icase), {"NYMC-$1", "$` $'"}}, // must be before PR_NYMCX_4
        look_replace_t{std::regex(PR_NYMCX_4,           std::regex::icase), {"NYMC-$2", "$`$1 $'"}},
        look_replace_t{std::regex(PR_NYMCX_5,           std::regex::icase), {"NYMC-$1", "$` $'"}},
        look_replace_t{std::regex(PR_PREFIX_1 PR_NIB,   std::regex::icase), {"NIB-$1", "$` $'"}},
        look_replace_t{std::regex(PR_PREFIX_1 PR_IDCDC, std::regex::icase), {"RG-$1", "$` $'"}},
        look_replace_t{std::regex(PR_PREFIX_1 PR_CBER,  std::regex::icase), {"CBER-$1", "$` $'"}},
        look_replace_t{std::regex(PR_PREFIX_1 PR_IVR_2, std::regex::icase), {"$1-$2 $4-$5", "$` $3 $'"}}, // before PR_IVR
        look_replace_t{std::regex(PR_PREFIX_1 PR_IVR,   std::regex::icase), {"$1-$2", "$` $'"}},
        look_replace_t{std::regex(PR_PREFIX_1 PR_MELB,  std::regex::icase), {"$1", "$` $'"}},
        look_replace_t{std::regex(PR_PREFIX_1 PR_CNIC,  std::regex::icase), {"$1-$2", "$` $'"}},
        look_replace_t{std::regex(PR_PREFIX_1 PR_IGY,   std::regex::icase), {"$1", "$` $'"}},
        look_replace_t{std::regex(PR_PREFIX_1 PR_CDC,   std::regex::icase), {"$1", "$` $'"}},
        look_replace_t{std::regex(PR_PREFIX_1 PR_BS,    std::regex::icase), {"$1", "$` $'"}},
        look_replace_t{std::regex(PR_SAN,               std::regex::icase), {"SAN-$1", "$` $'"}}, // VIDRL H3: SAN-007 (A/Tasmania/503/2020)

        // CDC-LV is annotation, it is extra in the c2 excel parser // look_replace_t{std::regex("\\b(CDC)-?(LV\\d+[AB]?)\\b", std::regex::icase), "$1-$2"}, "$` $'",
        look_replace_t{std::regex(PR_LOOKAHEAD_NOT_PAREN_SPACE "X[\\s\\-]+PR8", std::regex::icase), {"REASSORTANT-PR8", "$` $'"}},
        look_replace_t{std::regex(PR_LOOKAHEAD_NOT_PAREN_SPACE "REASSORTANT-([A-Z0-9\\-\\(\\)_/:]+)", std::regex::icase), {"REASSORTANT-$1", "$` $'"}}, // manually fixed gisaid stuff
    };

#pragma GCC diagnostic pop

    AD_LOG(ae::log::name_parsing, "reassortant source: \"{}\"", source);
    if (const auto reassortant_rest = scan_replace(source, normalize_data); reassortant_rest.has_value()) {
        AD_LOG(ae::log::name_parsing, "reassortant: \"{}\" extra:\"{}\" <-- \"{}\"", reassortant_rest->front(), reassortant_rest->back(), source);
        return {Reassortant{ae::string::uppercase(reassortant_rest->front())}, ae::string::collapse_spaces(ae::string::strip(reassortant_rest->back()))};
    }
    else {
        // AD_DEBUG("no reassortant in \"{}\"", source);
        return {Reassortant{}, std::string{source}};
    }

} // acmacs::virus::parse_reassortant

// ----------------------------------------------------------------------
