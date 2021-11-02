#include <array>

#include "ext/fmt.hh"
#include "ext/range-v3.hh"
#include "virus/name-parse.hh"
#include "utils/messages.hh"

static void virus_name_parsing_test(bool verbose);

// ----------------------------------------------------------------------

int main(int argc, const char* const* argv)
{
    const bool verbose{argc > 1 && std::string_view{argv[1]} == "-v"};
    try {
        virus_name_parsing_test(verbose);
    }
    catch (std::exception& err) {
        fmt::print("> {}\n", err.what());
    }
    return 0;
}

// ----------------------------------------------------------------------

using CT = ae::virus::name::Parts;

struct TestData
{
    std::string_view raw_name;
    CT expected;
};

void virus_name_parsing_test(bool verbose)
{
    using TD = TestData;

    const std::array data{
        TD{"A/SINGAPORE/INFIMH-16-0019/2016", CT{.subtype = "A", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016"}},
        TD{"A(H3N2)/SINGAPORE/INFIMH-16-0019/2016", CT{.subtype = "A(H3N2)", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016"}},
        TD{"a(h3n2)/singapore/infimh-16-0019/2016", CT{.subtype = "A(H3N2)", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016"}},
        TD{"A(H3N2) / SINGAPORE /INFIMH-16-0019/2016", CT{.subtype = "A(H3N2)", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016"}},
        TD{"A(H3)/SINGAPORE/INFIMH-16-0019/2016", CT{.subtype = "A(H3)", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016"}},
        TD{"AH3N2/SINGAPORE/INFIMH-16-0019/2016", CT{.subtype = "A(H3N2)", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016"}},
        // TD{"A/H3N2/SINGAPORE/INFIMH-16-0019/2016", CT{.subtype = "A(H3N2)", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016"}},
        TD{"A/HUMAN/SINGAPORE/INFIMH-16-0019/2016", CT{.subtype = "A", .host = "HUMAN", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016"}},
        TD{"A/SINGAPORE/INFIMH-16-0019/16", CT{.subtype = "A", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016"}},
        TD{"A/ SINGAPORE/INFIMH-16-0019/16", CT{.subtype = "A", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016"}},
        TD{"A/SINGAPORE /INFIMH-16-0019/16", CT{.subtype = "A", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016"}},

        TD{"A/SINGAPORE/INFIMH-16-0019/2016 NYMC-307A", CT{.subtype = "A", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016", .reassortant = "NYMC-307A"}},
        TD{"A/SINGAPORE/INFIMH-16-0019/2016 X-307A", CT{.subtype = "A", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016", .reassortant = "NYMC-307A"}},
        TD{"A/SINGAPORE/INFIMH-16-0019/2016 NYMC-X-307A", CT{.subtype = "A", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016", .reassortant = "NYMC-307A"}},
        TD{"A/SINGAPORE/INFIMH-16-0019/2016 NYMC X-307A", CT{.subtype = "A", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016", .reassortant = "NYMC-307A"}},
        TD{"A/SINGAPORE/INFIMH-16-0019/2016 NYMC-X307A", CT{.subtype = "A", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016", .reassortant = "NYMC-307A"}},
        TD{"A/SINGAPORE/INFIMH-16-0019/2016 NYMC X307A", CT{.subtype = "A", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016", .reassortant = "NYMC-307A"}},
        TD{"A/SINGAPORE/INFIMH-16-0019/2016 NYMC X307A-CL", CT{.subtype = "A", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016", .reassortant = "NYMC-307A-CL"}},
        TD{"NYMC-307A(A/SINGAPORE/INFIMH-16-0019/2016)", CT{.subtype = "A", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016", .reassortant = "NYMC-307A"}},
        TD{"NYMC-307A (A/SINGAPORE/INFIMH-16-0019/2016)", CT{.subtype = "A", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016", .reassortant = "NYMC-307A"}},
        TD{"BVR-25(B/Victoria/2110/2019)", CT{.subtype = "B", .location = "VICTORIA", .isolation = "2110", .year = "2019", .reassortant = "CBER-25"}},
        TD{"NIB-121 (A/Hong Kong/2671/2019", CT{.subtype = "A", .location = "HONG KONG", .isolation = "2671", .year = "2019", .reassortant = "NIB-121"}},
        TD{"NYMC X-181", CT{.reassortant = "NYMC-181"}},
        TD{"A/Kansas/14/2017 CBER-22B CDC19A", CT{.subtype = "A", .location = "KANSAS", .isolation="14", .year="2017", .reassortant="CBER-22B", .extra="CDC19A"}},

        // TD{"A/SINGAPORE/INFIMH-16-0019/2016 CL2  X-307A", CT{.subtype = "A", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016", .reassortant = "NYMC-307A", .extra =
        // "CL2"}}, TD{"A/SINGAPORE/INFIMH-16-0019/2016-06-14", CT{.subtype = "A", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016"}}, TD{"A/SINGAPORE/INFIMH-16-0019/2016
        // NEW CL2  X-307A", CT{.subtype = "A", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016", .reassortant = "NYMC-307A", .extra = "CL2"}},
        // TD{"A/SINGAPORE/INFIMH-16-0019/2016 CL2 NEW X-307A", CT{.subtype = "A", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016", .reassortant = "NYMC-307A", .extra =
        // "CL2"}}, TD{"A/SINGAPORE/INFIMH-16-0019/2016 CL2  X-307A NEW", CT{.subtype = "A", .location = "SINGAPORE", .isolation = "INFIMH-16-0019", .year = "2016", .reassortant = "NYMC-307A",
        // .extra = "CL2"}},

        // TD{"AH3N2/SINGAPORE/INFIMH-16-0019/2016 MDCK1", CT{}},

        TD{"A/SOUTH AFRICA/19/16", CT{.subtype = "A", .location = "SOUTH AFRICA", .isolation = "19", .year = "2016"}},
        TD{"A / SOUTH AFRICA/19/16", CT{.subtype = "A", .location = "SOUTH AFRICA", .isolation = "19", .year = "2016"}},
        TD{"A / SOUTH AFRICA/019/16", CT{.subtype = "A", .location = "SOUTH AFRICA", .isolation = "19", .year = "2016"}},
        TD{"A / SOUTH AFRICA /019/16", CT{.subtype = "A", .location = "SOUTH AFRICA", .isolation = "19", .year = "2016"}},
        TD{"A / SOUTH AFRICA/ 019/16", CT{.subtype = "A", .location = "SOUTH AFRICA", .isolation = "19", .year = "2016"}},
        TD{"A / SOUTH AFRICA / 019/16", CT{.subtype = "A", .location = "SOUTH AFRICA", .isolation = "19", .year = "2016"}},
        TD{"A / SOUTH AFRICA / 019 /16", CT{.subtype = "A", .location = "SOUTH AFRICA", .isolation = "19", .year = "2016"}},
        TD{"A / SOUTH AFRICA / 019 / 16", CT{.subtype = "A", .location = "SOUTH AFRICA", .isolation = "19", .year = "2016"}},
        TD{"A / SOUTH AFRICA / 19 /16", CT{.subtype = "A", .location = "SOUTH AFRICA", .isolation = "19", .year = "2016"}},
        TD{"A/AINWAZEIN/19/16", CT{.subtype = "A", .location = "AIN W ZAIN", .isolation = "19", .year = "2016"}},
        TD{"A/广西南宁/19/16", CT{.subtype = "A", .location = "GUANGXI NANNING", .isolation = "19", .year = "2016"}},
        // TD{"A/BRISBANE/01/2018  NYMC-X-311 (18/160)",                            CT{}}, // CT{A,              H,            "BRISBANE", "1", "2018", Reassortant{"NYMC-311"}, P, E}}, //
        // "(18/160)" removed by check_nibsc_extra TD{"A/Snowy Sheathbill/Antarctica/2899/2014",                            CT{}}, // CT{A,              hst{"SNOWY SHEATHBILL"}, "ANTARCTICA",
        // "2899", "2014", R, P, E}}, TD{"A/wigeon/Italy/6127-23/2007",                                        CT{}}, // CT{A,              hst{"WIGEON"}, "ITALY", "6127-23", "2007", R, P,
        // E}}, TD{"B/Via?A Del Mar/73490/2017",                                         CT{}}, // CT{B,              H,            "VINA DEL MAR", "73490", "2017", R, P, E}},
        // TD{"B/Cameroon11V-12080 GVFI/2011", CT{}}, // CT{B,              H,            "CAMEROON", "11V-12080 GVFI", "2011", R, P, E}}, TD{"A/Mali 071 Ci/2015", CT{}},
        // // CT{A,              H, "MALI", "71 CI", "2015", R, P, E}}, TD{"A/Zambia/13/174/2013",                                               CT{}}, // CT{A,              H, "ZAMBIA",
        // "13-174", "2013", R, P, E}}, TD{"A/Lyon/CHU18.54.48/2018",                                            CT{}}, // CT{A,              H,            "LYON CHU", "18.54.48", "2018", R,
        // P, E}},
        TD{"A/Lyon/CHU/R18.54.48/2018", CT{.subtype = "A", .location = "LYON CHU", .isolation = "R18.54.48", .year = "2018"}},
        // TD{"A/Algeria/G0281/16/2016",                                            CT{}}, // CT{A,              H,            "ALGERIA", "G0281-16", "2016", R, P, E}},
        // TD{"A/chicken/Ghana/7/2015",                                             CT{}}, // CT{A,              hst{"CHICKEN"}, "GHANA", "7", "2015", R, P, E}},
        // TD{"IVR-153 (A/CALIFORNIA/07/2009)",                                     CT{}}, // CT{A,              H,            "CALIFORNIA", "7", "2009", Reassortant{"IVR-153"}, P, E}},
        // TD{"A/Antananarivo/1067/2016 CBER-11B C1.3",                             CT{}}, // CT{A,              H,            "ANTANANARIVO", "1067", "2016", Reassortant{"CBER-11B"}, P,
        // "C1.3"}},
        // // CDC TD{"A/Montana/50/2016 CBER-07 D2.3",                                     CT{}}, // CT{A,              H,            "MONTANA", "50", "2016", Reassortant{"CBER-07"}, P,
        // "D2.3"}},
        // // CDC
        TD{"A/duck/Guangdong/4.30 DGCPLB014-O/2017", CT{.subtype = "A", .host = "DUCK", .location = "GUANGDONG", .isolation = "4.30 DGCPLB014-O", .year = "2017"}},
        TD{"A/duck/Guangdong/4.30 DGCPLB014-O/2017 XXX", CT{.subtype = "A", .host = "DUCK", .location = "GUANGDONG", .isolation = "4.30 DGCPLB014-O", .year = "2017", .extra = "XXX"}},
        TD{"A/duck/Guangdong/4.30.DGCPLB014-O/2017", CT{.subtype = "A", .host = "DUCK", .location = "GUANGDONG", .isolation = "4.30.DGCPLB014-O", .year = "2017"}},
        // TD{"A/duck/Guangdong/02.11 DGQTXC195-P/2015(Mixed)",                     CT{}}, // CT{A,              hst{"DUCK"},  "GUANGDONG", "2.11 DGQTXC195-P", "2015", R, P, E}}, // (MIXED)
        // removed TD{"A/duck/Guangdong/02.11 DGQTXC195-P/2015(H5N1)",                      CT{}}, // CT{typ{"A(H5N1)"}, hst{"DUCK"},  "GUANGDONG", "2.11 DGQTXC195-P", "2015", R, P, E}},
        // TD{"A/swine/Chachoengsao/2003",                                          CT{}}, // CT{A,              hst{"SWINE"}, "CHACHOENGSAO", "UNKNOWN", "2003", R, P, E}},
        TD{"A/duck/BODENSEE/#500/2019", CT{.subtype = "A", .host = "DUCK", .location = "BODENSEE", .isolation = "#500", .year = "2019"}},

        TD{"A/HAWAII/66/2019 CDC-LV30A", CT{.subtype = "A", .location = "HAWAII", .isolation = "66", .year = "2019", .extra = "CDC-LV30A"}},

        // // nbci -- genbank
        TD{"A/Anas platyrhynchos/Belgium/17330 2/2013", CT{.subtype = "A", .host = "ANAS PLATYRHYNCHOS", .location = "BELGIUM", .isolation = "17330 2", .year = "2013"}},
        // // TD{"A/mallard/Balkhash/6304_HA/2014",                                 CT{}}, //    CT{A, hst{"MALLARD"}, "BALKHASH", "6304", "2014"}, R, P, E}},
        // TD{"A/mallard/Balkhash/6304_HA/2014",                                    CT{}}, // CT{A, hst{"MALLARD"}, "BALKHASH", "6304", "2014", R, P, E}}, // _HA is seqgment reference in ncbi
        // // TD{"A/SWINE/NE/55024/2018",                                           CT{}}, //    CT{A, hst{"SWINE"},   "NE", "55024", "2018", R, P, E}},
        // TD{"A/chicken/Iran221/2001",                                             CT{}}, // CT{A, hst{"CHICKEN"}, "IRAN", "221", "2001", R, P, E}},
        // TD{"A/BiliranTB5/0423/2015",                                             CT{}}, // CT{A, H,                 "BILIRAN",  "TB5-0423", "2015", R, P, E}},
        // TD{"A/chicken/Yunnan/Kunming/2007",                                      CT{}}, // CT{A, hst{"CHICKEN"}, "YUNNAN KUNMING", "UNKNOWN", "2007", R, P, E}},

        // // gisaid
        // TD{"A/Flu-Bangkok/24/19",                                                CT{}}, // CT{A,               H, "BANGKOK", "24", "2019", R, P, E}},
        // TD{"A(H1)//ARGENTINA/FLE0116/2009",                                      CT{}}, // CT{typ{"A(H1)"},    H, "ARGENTINA", "FLE0116", "2009", R, P, E}},
        // TD{"A/FriuliVeneziaGiuliaPN/230/2019",                                   CT{}}, // CT{A,               H, "FRIULI-VENEZIA GIULIA PN", "230", "2019", R, P, E}},
        // TD{"A/turkey/Netherlands/03010496/03 clone-C12",                         CT{}}, // CT{A,               hst{"TURKEY"}, "NETHERLANDS", "3010496", "2003", R, P, "CLONE-C12"}},
        // TD{"A/reassortant/IDCDC-RG22(New York/18/2009 x Puerto Rico/8/1934)",    CT{}}, // CT{A, H, "NEW YORK", "18", "2009", Reassortant{"RG-22"}, P, E}},
        // TD{"A/reassortant/IgYRP13.c1(California/07/2004 x Puerto Rico/8/1934)",  CT{}}, // CT{A, H, "CALIFORNIA", "7", "2004", Reassortant{"IGYRP13.C1"}, P, E}},
        // TD{"A/REASSORTANT/X-83(CHILE/1/1983 X X-31)(H1N1)",                      CT{}}, // CT{typ{"A(H1N1)"},  H, "CHILE", "1", "1983", Reassortant{"NYMC-83 NYMC-31"}, P, E}},
        // TD{"A/X-53A(Puerto Rico/8/1934-New Jersey/11/1976)",                     CT{}}, // CT{A,               H, "NEW JERSEY", "11", "1976", Reassortant{"NYMC-53A"}, P, E}},
        // TD{"A/Anas platyrhynchos/bonn/7/03(H2N?)",                               CT{}}, // CT{typ{"A(H2)"},    hst{"MALLARD"}, "BONN", "7", "2003", R, P, E}},
        // TD{"A/California/7/2004 (cell-passaged)(H3)",                            CT{}}, // CT{typ{"A(H3)"},    H, "CALIFORNIA", "7", "2004", R, Passage{"MDCK?"}, E}},
        // TD{"A/California/7/2004 (egg-passaged)(H3)",                             CT{}}, // CT{typ{"A(H3)"},    H, "CALIFORNIA", "7", "2004", R, Passage{"E?"}, E}},
        // TD{"A/QUAIL/some unknown location/0025/2016(H5N1)",                      CT{}}, // CT{typ{"A(H5N1)"},  hst{"QUAIL"}, "SOME UNKNOWN LOCATION", "25", "2016", R, P, E}}, // DELISERDANG
        // is unknown location TD{"A/Medellin/FLU8292/2007(H3)",                                        CT{}}, // CT{typ{"A(H3)"},    H, "MEDELLIN", "FLU8292", "2007", R, P, E}}, // Medellin
        // is unknown location TD{"A/turkey/Italy12rs206-2/1999(H7N1)",                                 CT{}}, // CT{typ{"A(H7N1)"},  hst{"TURKEY"}, "ITALY", "12RS206-2", "1999", R, P, E}},
        // TD{"A/Michigan/382/2018(H1N2v)",                                         CT{}}, // CT{typ{"A(H1N2V)"}, H, "MICHIGAN", "382", "2018", R, P, E}},
        // TD{"A/mallard/Washington/454202-15/2006(H5N2?)",                         CT{}}, // CT{A,               hst{"MALLARD"}, "WASHINGTON", "454202-15", "2006", R, P, E}},
        // TD{"A/quail/Bangladesh/32270/2017(mixed,H9)",                            CT{}}, // CT{A,               hst{"QUAIL"}, "BANGLADESH", "32270", "2017", R, P, E}},
        // TD{"A/ruddy turnstone/Delaware/1016389/2003(HxN3)",                      CT{}}, // CT{typ{"A(N3)"},    hst{"RUDDY TURNSTONE"}, "DELAWARE", "1016389", "2003", R, P, E}},
        // TD{"A/ruddy turnstone/New Jersey/1321401/2005(HxNx)",                    CT{}}, // CT{A,               hst{"RUDDY TURNSTONE"}, "NEW JERSEY", "1321401", "2005", R, P, E}},
        // TD{"A/duck/ukraine/1/1960(H11N)",                                        CT{}}, // CT{typ{"A(H11)"},   hst{"DUCK"}, "UKRAINE", "1", "1960", R, P, E}},
        // TD{"A/ruddy turnstone/Delaware Bay/85/2017(mixed.H5)",                   CT{}}, // CT{A,               hst{"RUDDY TURNSTONE"}, "DELAWARE BAY", "85", "2017", R, P, E}},
        // TD{"A/Michigan/1/2010(HON1)",                                            CT{}}, // CT{typ{"A(N1)"},    H, "MICHIGAN", "1", "2010", R, P, E}},
        // TD{"A/some location/57(H2N2)",                                           CT{}}, // CT{typ{"A(H2N2)"},  H, "SOME LOCATION", "UNKNOWN", "1957", R, P, E}},
        // TD{"A/California/07/2009 NIBRG-121xp (09/268)",                          CT{}}, // CT{A,               H, "CALIFORNIA", "7", "2009", Reassortant{"NIB-121XP"}, P, E}},
        // TD{"A/turkey/Bulgaria/Haskovo/336/2018",                                 CT{}}, // CT{A,               hst{"TURKEY"}, "HASKOVO", "336", "2018", R, P, E}},

        // TD{"A/BONN/2/2020_PR8-HY-HA-R142G-HA-K92R/Y159F/K189N",                  CT{}}, // CT{A,               H, "BONN", "2", "2020", Reassortant{"PR8"}, P, E}},

        // TD{"",                   CT{typ{""}, hst{""}, "", R, P, E}},
    };

    size_t errors = 0;
    ae::virus::name::parse_settings settings;
    for (const auto [no, entry] : ranges::views::enumerate(data)) {
        try {
            ae::Messages messages;
            const auto result = ae::virus::name::parse(entry.raw_name, settings, messages, ae::MessageLocation{"test", no});
            // if (verbose)
            //     fmt::print(">>>  \"{}\"\n", entry.raw_name);
            if (!messages.empty()) {
                fmt::print("> \"{}\"\n{}", entry.raw_name, messages.report());
                ++errors;
            }
            else if (result != entry.expected) {
                fmt::print("> {:60s} <-- \"{}\"  expected: \"{}\"\n", fmt::format("\"{}\"", result), entry.raw_name, entry.expected);
                ++errors;
            }
            else if (verbose)
                fmt::print("  {:60s} <-- \"{}\"\n", fmt::format("\"{}\"", result), entry.raw_name);
        }
        catch (std::exception& err) {
            fmt::print("> {}: {}\n", entry.raw_name, err.what());
            throw;
        }
    }
    // fmt::print("{}\n", messages.report());
    if (errors)
        throw std::runtime_error{fmt::format("{} errors found", errors)};
}

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
