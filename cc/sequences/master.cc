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
        MasterSequence{TS{"B"}, "B/BRISBANE/60/2008"sv,                  SA{"DRICTGITSSNSPHVVKTATQGEVNVTGVIPLTTTPTKSHFANLKGTETRGKLCPKCLNCTDLDVALGRPKCTGKIPSARVSILHEVRPVTSGCFPIMHDRTKIRQLPNLLRGYEHIRLSTHNVINAENAPGGPYKIGTSGSCPNITNGNGFFATMAWAVPKNDKNKTATNPLTIEVPYICTEGEDQITVWGFHSDNETQMAKLYGDSKPQKFTSSANGVTTHYVSQIGGFPNQTEDGGLPQSGRIVVDYMVQKSGKTGTITYQRGILLPQKVWCASGRSKVIKGSLPLIGEADCLHEKYGGLNKSKPYYTGEHAKAIGNCPIWVKTPLKLANGTKYRPPAKLLKERGFFGAIAGFLEGGWEGMIAGWHGYTSHGAHGVAVAADLKSTQEAINKITKNLNSLSELEVKNLQRLSGAMDELHNEILELDEKVDDLRADTISSQIELAVLLSNEGIINSEDEHLLALERKLKKMLGPSAVEIGNGCFETKHKCNQTCLDRIAAGTFDAGEFSLPTFDSLNITAASLNDDGLDNHTILLYYSTAASSLAVTLMIAIFVVYMVSRDNVSCSICL"}},

        MasterSequence{TS{"A(H1N1)"}, "A(H1N1)/CALIFORNIA/7/2009"sv,     SA{"DTLCIGYHANNSTDTVDTVLEKNVTVTHSVNLLEDKHNGKLCKLRGVAPLHLGKCNIAGWILGNPECESLSTASSWSYIVETPSSDNGTCYPGDFIDYEELREQLSSVSSFERFEIFPKTSSWPNHDSNKGVTAACPHAGAKSFYKNLIWLVKKGNSYPKLSKSYINDKGKEVLVLWGIHHPSTSADQQSLYQNADAYVFVGSSRYSKKFKPEIAIRPKVRDQEGRMNYYWTLVEPGDKITFEATGNLVVPRYAFAMERNAGSGIIISDTPVHDCNTTCQTPKGAINTSLPFQNIHPITIGKCPKYVKSTKLRLATGLRNIPSIQSRGLFGAIAGFIEGGWTGMVDGWYGYHHQNEQGSGYAADLKSTQNAIDEITNKVNSVIEKMNTQFTAVGKEFNHLEKRIENLNKKVDDGFLDIWTYNAELLVLLENERTLDYHDSNVKNLYEKVRSQLKNNAKEIGNGCFEFYHKCDNTCMESVKNGTYDYPKYSEEAKLNREEIDGVKLESTRIYQILAIYSTVASSLVLVVSLGAISFWMCSNGSLQCRICI"}},
        MasterSequence{TS{"A(H1N1)"}, "A/Brevig Mission/1/1918"sv,       SA{"DTICIGYHANNSTDTVDTVLEKNVTVTHSVNLLEDSHNGKLCKLKGIAPLQLGKCNIAGWLLGNPECDLLLTASSWSYIVETSNSENGTCYPGDFIDYEELREQLSSVSSFEKFEIFPKTSSWPNHETTKGVTAACSYAGASSFYRNLLWLTKKGSSYPKLSKSYVNNKGKEVLVLWGVHHPPTGTDQQSLYQNADAYVSVGSSKYNRRFTPEIAARPKVRDQAGRMNYYWTLLEPGDTITFEATGNLIAPWYAFALNRGSGSGIITSDAPVHDCNTKCQTPHGAINSSLPFQNIHPVTIGECPKYVRSTKLRMATGLRNIPSIQSRGLFGAIAGFIEGGWTGMIDGWYGYHHQNEQGSGYAADQKSTQNAIDGITNKVNSVIEKMNTQFTAVGKEFNNLERRIENLNKKVDDGFLDIWTYNAELLVLLENERTLDFHDSNVRNLYEKVKSQLKNNAKEIGNGCFEFYHKCDDACMESVRNGTYDYPKYSEESKLNREEIDGVKLESMGVYQILAIYSTVASSLVLLVSLGAISFWMCSNGSLQCRICI"}},

        MasterSequence{TS{"A(H3N2)"}, "A(H3N2)/HONG_KONG/1/1968"sv,      SA{"QDLPGNDNSTATLCLGHHAVPNGTLVKTITDDQIEVTNATELVQSSSTGKICNNPHRILDGIDCTLIDALLGDPHCDVFQNETWDLFVERSKAFSNCYPYDVPDYASLRSLVASSGTLEFITEGFTWTGVTQNGGSNACKRGPGSGFFSRLNWLTKSGSTYPVLNVTMPNNDNFDKLYIWGVHHPSTNQEQTSLYVQASGRVTVSTRRSQQTIIPNIGSRPWVRGLSSRISIYWTIVKPGDVLVINSNGNLIAPRGYFKMRTGKSSIMRSDAPIDTCISECITPNGSIPNDKPFQNVNKITYGACPKYVKQNTLKLATGMRNVPEKQTRGLFGAIAGFIENGWEGMIDGWYGFRHQNSEGTGQAADLKSTQAAIDQINGKLNRVIEKTNEKFHQIEKEFSEVEGRIQDLEKYVEDTKIDLWSYNAELLVALENQHTIDLTDSEMNKLFEKTRRQLRENAEDMGNGCFKIYHKCDNACIESIRNGTYDHDVYRDEALNNRFQIKGVELKSGYKDWILWISFAISCFLLCVVLLGFIMWACQRGNIRCNICI"}},
        MasterSequence{TS{"A(H3N2)"}, "A/Memphis/110/1976"sv,            SA{"QDLPGNDNSTATLCLGHHAVPNGTLVKTITNDQIEVTNATELVQSSSTGKICDNPHRILDGINCTLIDALLGDPHCDGFQNEKWDLFVERSKAFSNCYPYDVPDYASLRSLVASSGTLEFINEGFNWTGVTQNGGSRACKRGPDNGFFSRLNWLYKSGSTYPVQNVTMPNNDNSDKLYIWGVHHPSTDKEQTDLYVQASGKVTVSTKRSQQTVIPNVGSRPWVRGLSSRVSIYWTIVKPGDILVINSNGNLIAPRGYFKMRTGKSSIMRSDAPIGTCSSECITPNGSIPNDKPFQNVNKITYGACPKYVKQNTLKLATGMRNVPEKQTRGIFGAIAGFIENGWEGMIDGWYGFRHQNSEGTGQAADLKSTQAAIDQINGKLNRVIEKTNEKFHQIEKEFSEVEGRIQDLEKYVEDTKIDLWSYNAELLVALENQHTIDLTDSEMNKLFEKTRRQLRENAEDMGNGCFKIYHKCDNACIGSIRNGTYDHDVYRDEALNNRFQIKGVELKSGYKDWILWISFAISCFLLCVVLLGFIMWACQKGNIRCNICI"}},

        // http://www.signalpeptide.de/
        MasterSequence{TS{"A(H2N2)"}, "A/Japan/305/1957"sv,              SA{"DQICIGYHANNSTEKVDTNLERNVTVTHAKDILEKTHNGKLCKLNGIPPLELGDCSIAGWLLGNPECDRLLSVPEWSYIMEKENPRDGLCYPGSFNDYEELKHLLSSVKHFEKVKILPKDRWTQHTTTGGSRACAVSGNPSFFRNMVWLTKEGSDYPVAKGSYNNTSGEQMLIIWGVHHPIDETEQRTLYQNVGTYVSVGTSTLNKRSTPEIATRPKVNGQGGRMEFSWTLLDMWDTINFESTGNLIAPEYGFKISKRGSSGIMKTEGTLENCETKCQTPLGAINTTLPFHNVHPLTIGECPKYVKSEKLVLATGLRNVPQIESRGLFGAIAGFIEGGWQGMVDGWYGYHHSNDQGSGYAADKESTQKAFDGITNKVNSVIEKMNTQFEAVGKEFGNLERRLENLNKRMEDGFLDVWTYNAELLVLMENERTLDFHDSNVKNLYDKVRMQLRDNVKELGNGCFEFYHKCDDECMNSVKNGTYDYPKYEEESKLNRNEIKGVKLSSMGVYQILAIYATVAGSLSLAIMMAGISFWMCSNGSLQCRICI"}},
        MasterSequence{TS{"A(H2N2)"}, "A/Korea/426/1968"sv,              SA{"DQICIGYHANNSTEKVDTILERNVTVTHAKDILEKTHNGKLCKLNGIPPLELGDCSIAGWLLGNPECDRLLSVPEWSYIMEKENPRYSLCYPGSFNDYEELKHLLSSVKHFEKVKILPKDRWTQHTTTGGSWACAVSGKPSFFRNMVWLTRKGSNYPVAKGSYNNTSGEQMLIIWGVHHPNDEAEQRALYQNVGTYVSVATSTLYKRSIPEIAARPKVNGLGRRMEFSWTLLDMWDTINFESTGNLVAPEYGFKISKRGSSGIMKTEGTLENCETKCQTPLGAINTTLPFHNVHPLTIGECPKYVKSEKLVLATGLRNVPQIESRGLFGAIAGFIEGGWQGMVDGWYGYHHSNDQGSGYAADKESTQKAFNGITNKVNSVIEKMNTQFEAVGKEFSNLEKRLENLNKKMEDGFLDVWTYNAELLVLMENERTLDFHDSNVKNLYDKVRMQLRDNVKELGNGCFEFYHKCDNECMDSVKNGTYDYPKYEEESKLNRNEIKGVKLSSMGVYQILAIYATVAGSLSLAIMMAGISFWMCSNGSLQCRICI"}},
        MasterSequence{TS{"A(H2N2)"}, "A/Mallard/New York/6750/1978 "sv, SA{"DQICIGYHANNSTEKVDTILERNVTVTHAKDILEKTHNGKLCRLSGIPPLELGDCSIAGWLLGNPECDRLLSVPEWSYIVEKENPANGLCYPGNFNDYEELKHLLTRVTHFEKIKILPRDQWTQHTTTGGSRACAVSGNPSFFRNMVWLTKKGSNYPVAKGSYNNTSGEQMLVIWGIHHPNDDTEQRTLYQNVGTYVSVGTSTLNKRSIPEIATRPKVNGQGGRMEFSWTLLETWDVINFESTGNLIAPEYGFKISKRGSSGIMKTEKTLENCETKCQTPLGAINTTLPFHNIHPLTIGECPKYVKSDRLVLATGLRNVPQIESRGLFGAIAGFIEGGWQGMIDGWYGYHHSNDQGSGYAADKESTQKAIDGITNKVNSVIEKMNTQFEAVGKEFNNLERRLENLNKKMEDGFLDVWTYNAELLVLMENERTLDFHDSNVKNLYDKVRMQLRDNAKEIGNGCFEFYHKCDDECMNSVRNGTYDYPKYEEESKLNRNEIKGVKLSNMGVYQILAIYATVAGSLSLAIMIAGISFWMCSNGSLQCRICI"}},

        MasterSequence{TS{"A(H4N4)"}, "A/Grey teal/Australia/2/1979"sv,  SA{"QNYTGNPVICMGHHAVANGTMVKTLTDDQVEVVTAQELVESQILPELCPSPLRLVDGQTCDIVNGALGSPGCDHLNGAEWDVFIERPSAVDTCYPFDVPDYQSLRSILANNGKFEFIAEEFQWNTVKQNGKSGACKRANVNDFFNRLNWLVKSDGNAYPLQNLTKINNGDYARLYIWGVHHPSTDTEQTNLYKNNPGRVTVSTKTSQTSVVPNIGSRPLVRGQSGRISFYWTIVEPGDLIVFNTIGNLIAPRGHYKLDSQKKSTILNTAVPIGSCVSKCHTDKGSLSTTKPFQNISRIAIGDCPKYVKQGSLKLATGMRNIPEKASRGLFGAIAGFIENGWQGLIDGWYGFRHQNAEGTGTAADLKSTQAAIDQINGKLNRLIEKTNEKYHQIEKEFEQVEGRIQDLEKYVEDTKIDLWSYNAELLVALENQHTIDVTDSEMNKLFERVRRQLRENAEDKGNGCFEIFHKCDNNCIESIRNGTYDHDIYRDEAINNRFQIQGVKLTQGYKDIILWISFSISCFLLVALLLAFILWACQNGNIRCQICI"}},
        MasterSequence{TS{"A(H4N6)"}, "A/Budgerigar/Hokkaido/1/1977"sv,  SA{"QSYTGNPVICMGHHSVANGTMVKTLTDDQVEVVTARELVESQTLPELCPSPLRLVDGQTCDIINGALGSPGCDHLNGAEWDVFIERPNAVDTCYPFDVPDYQSLRSILANNGKFEFIAEEFQWNTVIQNGKSSACKRANVNDFFNRLNWLVKSTGNAYPLQNLTKVNNGDYARLYIWGVHHPSTDTEQTNLYKNNPGRVTVSTKTSQTSVVPNIGSRPLVRGQSGRISFYWTIVEPGDLIVFNTIGNLIAPRGHYKLNNQKKSTILNTPIPIGSCVSKCHTDKGSVSTTNPFQNISRIAIGECPKYVKQGSLKLATGMRNVPEKASRGLFGAIAGFIENGWQGLIDGWYGFRHQNAEGTGTAADLKSTQAAIDKINGKLNRLIEKTNEKYHQIEKEFNKIEGRVQDLEKYVEDTKIDLWSYNAELLVALENQHTIDVTDSEMNKLFERVRRQLRENAEDQGNGCFEIFHKCDNNCIESIRNGTYDHDIYRDEAINNRFQIQGVKLIQGYKDIILWISFSISCFLLVALLLAFILWACQNGNIRCQICI"}},

        MasterSequence{TS{"A(H5N1)"}, "A/Chicken/Hong Kong/220/1997"sv,  SA{"DQICIGYHANNSTEQVDTIMEKNVTVTHAQDILERTHNGKLCDLNGVKPLILRDCSVAGWLLGNPMCDEFINVPEWSYIVEKASPANDLCYPGNFNDYEELKHLLSRINHFEKIQIIPKSSWSNHDASSGVSSACPYLGRSSFFRNVVWLIKKNSTYPTIKRSYNNTNQEDLLVLWGIHHPNDAAEQTKLYQNPTTYISVGTSTLNQRLVPEIATRPKVNGQSGRMEFFWTILKPNDAINFESNGNFIAPEYAYKIVKKGDSTIMKSELEYGNCNTKCQTPMGAINSSMPFHNIHPLTIGECPKYVKSNRLVLATGLRNTPQRERRRKKRGLFGAIAGFIEGGWQGMVDGWYGYHHSNEQGSGYAADQESTQKAIDGVTNKVNSIINKMNTQFEAVGREFNNLERRIENLNKKMEDGFLDVWTYNTELLVLMENERTLDFHDSNVKNLYDKVRLQLRDNAKELGNGCFEFYHKCDNECMESVKNGTYDYPQYSEEARLNREEISGVKLESMGTYQILSIYSTVASSLALAIMVAGLSLWMCSNGSLQCRICI"}},
        MasterSequence{TS{"A(H5N1)"}, "A/Chicken/Hong Kong/37.4/2002"sv, SA{"DQICIGYHANNSTEQVDTIMEKNVTVTHAQDILEKTHNGKLCDLDGVKPLILRDCSVAGWLLGNPMCDEFINVPEWSYIVEKANPANDLCYPGDFNDYEELKHLLSRINHFEKIQIIPKSSWSNHEASSGVSSACPYNGKSSFFRNVVWLIKKDSAYPTIKRSYNNTNQEDLLILWGIHHPNDAAEQTKLYQNPTTYISVGTSTLNQRLVPKISTRSKVNGQSGRMEFFWTILKPSDAINLESNGNFIAPEYAYKIVKKGDSAIMKSELEYGNCNTKCQTPMGAINSSMPFHNIHPLTIGECPKYVKSNRLVLATGLRNTPQRERRRKKRGLFGAIAGFIEGGWQGMVDGWYGYHHSNEQGSGYAADKESTQKAIDGVTNKVNSIINKMNTQFEAVGREFNNLERRIENLNKKMEDGFLDVWTYNAELLVLMENERTLDFHDSNVKNLYDKVRLQLRDNAKELGNGCFEFYHKCDNECMESVKNGTYDYPQYSEEARLNREEISGVKLESMGTYQILSIYSTVASSLALAIMVAGLSLWMCSNGSLQCRIC"}},

        MasterSequence{TS{"A(H6N5)"}, "A/Shearwater/Australia/1972"sv,   SA{"DKICIGYHANNSTTQIDTILEKNVTVTHSVELLENQKEERFCKILKKAPLDLKGCTIEGWILGNPQCDLLLGDQSWSYIVERPTAQNGICYPGVLNEVEELKALIGSGERVERFEMFPKSTWTGVDTSSGVTRACPYNSGSSFYRNLLWIIKTKSAAYSVIKGAYNNTGNQPILYFWGVHHPPDTNEQNTLYGSGDRYVRMGTESMNFAKSPEIAARPAVNGQRGRIDYYWSILKPGETLNVESNGNLIAPWYAFRFVSTSNKGAVFKSNLPIENCDATCQTVAGVLRTNKTFQNVSPLWIGECPKYVKSESLRLATGLRNVPQIETRGLFGAIAGFIEGGWTGMIDGWYGYHHENSQGSGYAADRESTQKAVDGITNKVNSIIDKMNTQFEAVDHEFSNLERRIDNLNKRMEDGFLDVWTYNAELLVLLENERTLDLHDANVKNLYERVKSQLRDNAMILGNGCFEFWHKCDDECMESVKNGTYDYPKYQDESKLNRQEIESVKLESLGVYQILAIYSTVSSSLVLVGLIIAVGLWMCSNGSMQCRICI"}},

        MasterSequence{TS{"A(H7N7)"}, "A/Equine/C.Detroit/1/1964"sv,     SA{"DKICLGHHAESNGTKVDTLTEKGIEVVNATETVEQKNIPKICSKGKQTIDLGQCGLLGTIIGPPQCDQFLEFSANLIIERREGNDICYPGKFDDEETLRQILRKSGGIKKENMGFTYTGVRTNGETSACRRSRSSFYAEMKWLLSNTDNEVFPQMTKSYKNTKREPALIIWGIHHSGSTAEQTRLYGSGNKLITVWSSKYQQSFAPNPGPRPQINGQSGRIDFYWLMLDPNDTVNFSFNGAFIAPDRASFLRGKSLGIQSDAQLDNNCEGECYHIGGTIISNLPFQNINSRAIGKCPRYVKQKSLMLATGMKNVPENSTHKQLTHHMRKKRGLFGAIAGFIENGWEGLIDGWYGYRHQNAQGEGTAADYKSTQSAINQITGKLNRLIEKTNQQFELIDNEFNEIEKQIGNVINWTRDSIIEIWSYNAEFLVAVENQHTIDLTDSEMNKLYEKVRRQLRENAEEDGNGCFEIFHQCDNDCMASIRNNTYDHKKYRKEAIQNRIQIDAVKLSSGYKDVILWFSFGASCFLFLAIAMGLAFICIKNGNMRCTICI"}},
        MasterSequence{TS{"A(H7N7)"}, "A/Equine/New Market/1/1977"sv,    SA{"DKICLGHHAVSNGTKVDTLTEKGIEVVNATETVEQKNIPKICSKGKQTIDLGQCGLLGTTIGPPQCDQFLEFSANLIIERREGDDICYPGKFDNEETLRQILRKSGGIKKENMGFTYTGVRTNGETSACRRSRSSFYAEMKWLLSNTDNGVFPQMTKSYKNTKKEPALIIWGIHHSGSTAEQTRLYGSGNKLITVWSSKYQQSFAPNPGPRPQMNGQSGRIDFYWLMLDPNDTVNFSFNGAFIAPDRASFLRGKSLGIQSDAQLDNNCEGECYHIGGTIISNLPFQNINSRAIGKCPRYVKQKSLMLATGMKNVPENSTHKQLTHHMRKKRGLFGAIAGFIENGWEGLIDGWYGYRHQNAQGEGTAADYKSTQSAVNQITGKLNRLIEKTNQQFELIDNEFNEIEKQIGNVINWTRDSIIEIWSYNAEFLVAVENQHTIDLTDSEMNKLYEKVRRQLRENAEEDGNGCFEIFHQCDNDCMASIRNNTYDHKKYRKEAIQNRIQIDAVKLSSGYKEIILWFSFGASCFLFLAIAMVLAFICIKNGNMRCTICI"}},
        MasterSequence{TS{"A(H7N7)"}, "A/Equine/Prague/1/1956"sv,        SA{"DKICLGHHAVSNGTKVDTLTEKGIEVVNATETVEQTNIPKICSKGKQTVDLGQCGLLGTVIGPPQCDQFLEFSANLIVERREGNDICYPGKFDNEETLRKILRKSGGIKKENMGFTYTGVRTNGETSACRRSRSSFYAEMKWLLSSTDNGTFPQMTKSYKNTKKVPALIIWGIHHSGSTTEQTRLYGSGNKLITVWSSKYQQSFVPNPGPRPQMNGQSGRIDFHWLMLDPNDTVTFSFNGAFIAPDRASFLRGKSLGIQSDAQLDNNCEGECYHIGGTIISNLPFQNINSRAIGKCPRYVKQKSLMLATGMKNVPEAPAHKQLTHHMRKKRGLFGAIAGFIENGWEGLIDGWYGYKHQNAQGEGTAADYKSTQSAINQITGKLNRLIEKTNQQFELIDNEFNEIEKQIGNVINWTRDSIIEVWSYNAEFLVAVENQHTIDLTDSEMNKLYEKVRRQLRENAEEDGNGCFEIFHQCDNDCMASIRNNTYDHKKYRKEAIQNRIQIDAVKLSSGYKDIILWFSFGASCFLFLAIAMGLVFICIKNGNMRCTICI"}},

        MasterSequence{TS{"A(H8N4)"}, "A/Turkey/Ontario/6118/1968"sv,    SA{"DRICIGYQSNNSTDTVNTLIEQNVPVTQTMELVETEKHPAYCNTDLGAPLELRDCKIEAVIYGNPKCDIHLKDQGWSYIVERPSAPEGMCYPGSVENLEELRFVFSSAASYKRIRLFDYSRWNVTRSGTSKACNASTGGQSFYRSINWLTKKEPDTYDFNEGAYVNNEDGDIIFLWGIHHPPDTKEQTTLYKNANTLSSVTTNTINRSFQPNIGPRPLVRGQQGRMDYYWGILKRGETLKIRTNGNLIAPEFGYLLKGESYGRIIQNEDIPIGNCNTKCQTYAGAINSSKPFQNASRHYMGECPKYVKKASLRLAVGLRNTPSVEPRGLFGAIAGFIEGGWSGMIDGWYGFHHSNSEGTGMAADQKSTQEAIDKITNKVNNIVDKMNREFEVVNHEFSEVEKRINMINDKIDDQIEDLWAYNAELLVLLENQKTLDEHDSNVKNLFDEVKRRLSANAIDAGNGCFDILHKCDNECMETIKNGTYDHKEYEEEAKLERSKINGVKLEENTTYKILSIYSTVAASLCLAILIAGGLILGMQNGSCRCMFCI"}},

        MasterSequence{TS{"A(H9N2)"}, "A/Turkey/Wisconsin/1/1966"sv,     SA{"DKICIGYQSTNSTETVDTLTESNVPVTHTKELLHTEHNGMLCATDLGHPLILDTCTIEGLIYGNPSCDILLGGKEWSYIVERSSAVNGMCYPGNVENLEELRSLFSSAKSYKRIQIFPDKTWNVTYSGTSRACSNSFYRSMRWLTHKSNSYPFQNAHYTNNERENILFMWGIHHPPTDTEQTDLYKNADTTTSVTTEDINRTFKPVIGPRPLVNGQQGRIDYYWSVLKPGQTLRIRSNGNLIAPWYGHVLTGESHGRILKTDLNNGNCVVQCQTEKGGLNTTLPFHNISKYAFGNCPKYVGVKSLKLPVGLRNVPAVSSRGLFGAIAGFIEGGWPGLVAGWYGFQHSNDQGVGMAADKGSTQKAIDKITSKVNNIIDKMNKQYEVIDHEFNELEARLNMINNKIDDQIQDIWAYNAELLVLLENQKTLDEHDANVNNLYNKVKRALGSNAVEDGNGCFELYHKCDDQCMETIRNGTYDRQKYQEESRLERQKIEGVKLESEGTYKILTIYSTVASSLVLAMGFAAFLFWAMSNGSCRCNICI"}},

        MasterSequence{TS{"A(H10N4)"}, "A/Mink/Sweden/1984"sv,           SA{"DRICLGHHAVANGTIVKTLTNEQEEVTNATETVESTNLNKLCMKGRSYKDLGNCHPVGMLIGTPVCDPHLTGTWDTLIERENAIAHCYPGATINEEALRQKIMESGGISKMSTGFTYGSSITSAGTTKACMRNGGDSFYAELKWLVSKTKGQNFPQTTNTYRNTDTAEHLIIWGIHHPSSTQEKNDLYGTQSLSISVESSTYQNNFVPVVGARPQVNGQSGRIDFHWTLVQPGDNITFSDNGGLIAPSRVSKLTGRDLGIQSEALIDNSCESKCFWRGGSINTKLPFQNLSPRTVGQCPKYVNQRSLLLATGMRNVPEVVQGRGLFGAIAGFIENGWEGMVDGWYGFRHQNAQGTGQAADYKSTQAAIDQITGKLNRLIEKTNTEFESIESEFSETEHQIGNVINWTKDSITDIWTYNAELLVAMENQHTIDMADSEMLNLYERVRKQLRQNAEEDGKGCFEIYHTCDDSCMESIRNNTYDHSQYREEALLNRLNINPVKLSSGYKDIILWFSFGESCFVLLAVVMGLVFFCLKNGNMRCTICI"}},
        MasterSequence{TS{"A(H10N7)"}, "A/Duck/Germany/1949"sv,          SA{"DRICLGHHAVANGTIVKTLTNEQEEVTNATETVESTNLNKLCMKGRSYKDLGNCHPVGMLIGTPVCDPHLTGTWDTLIERENAIAHCYPGATINEEALRQKIMESGGISKMSTGFTYGSSINSAGTTKACMRNGGDSFYAELKWLVSKTKGQNFPQTTNTYRNTDTAEHLIIWGIHHPSSTQEKNDLYGTQSLSISVESSTYQNNFVPVVGARPQVNGQSGRIDFHWTLVQPGDNITFSHNGGLIAPSRVSKLTGRGLGIQSEALIDNSCESKCFWRGGSINTKLPFQNLSPRTVGQCPKYVNQRSLLLATGMRNVPEVVQGRGLFGAIAGFIENGWEGMVDGWYGFRHQNAQGTGQAADYKSTQAAIDQITGKLNRLIEKTNTEFESIESEFSETEHQIGNVINWTKDSITDIWTYQAELLVAMENQHTIDMADSEMLNLYERVRKQLRQNAEEDGKGCFEIYHTCDDSCMESIRNNTYDHSQYREEALLNRLNINSVKLSSGYKDIILWFSFGASCFVLLAVVMGLVFFCLKNGNMRCTICI"}},

        MasterSequence{TS{"A(H11N6)"}, "A/Duck/England/1/1956"sv,        SA{"DEICIGYLSNNSTDKVDTIIENNVTVTSSVELVETEHTGSFCSINGKQPISLGDCSFAGWILGNPMCDELIGKTSWSYIVEKPNPTNGICYPGTLESEEELRLKFSGVLEFNKFEVFTSNGWGAVNSGVGVTAACKFGGSNSFFRNMVWLIHQSGTYPVIKRTFNNTKGRDVLIVWGIHHPATLTEHQDLYKKDSSYVAVGSETYNRRFTPEINTRPRVNGQAGRMTFYWKIVKPGESITFESNGAFLAPRYAFEIVSVGNGKLFRSELNIESCSTKCQTEIGGINTNKSFHNVHRNTIGDCPKYVNVKSLKLATGPRNVPAIASRGLFGAIAGFIEGGWPGLINGWYGFQHRNEEGTGIAADKESTQKAIDQITSKVNNIVDRMNTNFESVQHEFSEIEERINQLSKHVDDSVVDIWSYNAQLLVLLENEKTLDLHDSNVRNLHEKVRRMLKDNAKDEGNGCFTFYHKCDNKCIERVRNGTYDHKEFEEESKINRQEIEGVKLDSSGNVYKILSIYSCIASSLVLAALIMGFMFWACSNGSCRCTICI"}},

        MasterSequence{TS{"A(H12N5)"}, "A/Duck/Alberta/60/1976"sv,       SA{"DKICIGYQTNNSTETVNTLSEQNVPVTQVEELVHRGIDPILCGTELGSPLVLDDCSLEGLILGNPKCDLYLNGREWSYIVERPKEMEGVCYPGSIENQEELRSLFSSIKKYERVKMFDFTKWNVTYTGTSKACNNTSNQGSFYRSMRWLTLKSGQFPVQTDEYKNTRDSDIVFTWAIHHPPTSDEQVKLYKNPDTLSSVTTVEINRSFKPNIGPRPLVRGQQGRMDYYWAVLKPGQTVKIQTNGNLIAPEYGHLITGKSHGRILKNNLPMGQCVTECQLNEGVMNTSKPFQNTSKHYIGKCPKYIPSGSLKLAIGLRNVPQVQDRGLFGAIAGFIEGGWPGLVAGWYGFQHQNAEGTGIAADRDSTQRAIDNMQNKLNNVIDKMNKQFEVVNHEFSEVESRINMINSKIDDQITDIWAYNAELLVLLENQKTLDEHDANVRNLHDRVRRVLRENAIDTGDGCFEILHKCDNNCMDTIRNGTYNHKEYEEESKIERQKVNGVKLEENSTYKILSIYSSVASSLVLLLMIIGGFIFGCQNGNVRCTFCI"}},

        MasterSequence{TS{"A(H13N2)"}, "A/Whale/Maine/328/1984"sv,       SA{"DRICVGYLSTNSTEKVDTLLENDVPVTSSIDLVETNHTGTYCSLDGISPVHLGDCSFEGWIVGNPACTSNFGIREWSYLIEDPSAPHGLCYPGELDNNGELRHLFSGIRSFSRTELIAPTSWGEVNDGATSACRDNTGTNSFYRNLVWFVKKGNSYPVISRTYNNTTGRDVLVMWGLHHPVSTDETKSLYVNSDPYTLVSTSSWSKKYKLETGVRPGYNGQRSWMKIYWVLMHPGESITFESNGGLLAPRYGYIIEEYGKGRIFQSPIRIARCNTRCQTSVGGINTNKTFQNIERNALGNCPKYIKSGQLKLATGLRNVPAISNRGLFGAIAGFIEGGWPGLINGWYGFQHQNEQGVGIAADKESTQKAIDQITTKINNIIDKMNGNYDSIRGEFSQVEQRINMLADRIDDAVTDVWSYNAKLLVLLENDKTLDMHDANVRNLHEQVRRTLKANAIDEGNGCFELLHKCNDSCMDTIRNGTYNHAEYAEESKLKRQEIEGIKLKSEDNVYKALSIYSCIASSVVLVGLILAFIMWACSSGNCRFNVCI"}},

        MasterSequence{TS{"A(H14N5)"}, "A/Mallard/Astrakhan/263/1982"sv, SA{"QITNGTTGNPIICLGHHAVENGTSVKTLTDNHVEVVSAKELVETNHTDELCPSPLKLVDGQDCHLINGALGSPGCDRLQDTTWDVFIERPTAVDTCYPFDVPDYQSLRSILASSGSLEFIAEQFTWNGVKVDGSSSACLRGGRNSFFSRLNWLTKATNGNYGPINVTKENTGSYVRLYLWGVHHPSSDNEQTDLYKVATGRVTVSTRSDQISIVPNIGSRPRVRNQSGRISIYWTLVNPGDSIIFNSIGNLIAPRGHYKISKSTKSTVLKSDKRIGSCTSPCLTDKGSIQSDKPFQNVSRIAIGNCPKYVKQGSLMLATGMRNIPGKQAKGLFGAIAGFIENGWQGLIDGWYGFRHQNAEGTGTAADLKSTQAAIDQINGKLNRLIEKTNEKYHQIEKEFEQVEGRIQDLEKYVEDTKIDLWSYNAELLVALENQHTIDVTDSEMNKLFERVRRQLRENAEDQGNGCFEIFHQCDNNCIESIRNGTYDHNIYRDEAINNRIKINPVTLTMGYKDIILWISFSMSCFVFVALILGFVLWACQNGNIRCQICI"}},
    };

//        MasterSequence{TS{"A()"}, ""sv, SA{""}},

#pragma GCC diagnostic pop
}

// ======================================================================

std::vector<const ae::sequences::MasterSequence*> ae::sequences::master_sequences(const type_subtype_t& ts)
{
    std::vector<const MasterSequence*> result;
    for (const auto& master_seq : master_sequence_data) {
        if (master_seq.type_subtype == ts)
            result.push_back(&master_seq);
    }
    return result;

} // ae::sequences::master_sequences

// ----------------------------------------------------------------------

size_t ae::sequences::min_hamming_distance_to_master(const type_subtype_t& ts, const sequence_aa_t& source)
{
    size_t min_dist{source.size()};
    for (const auto& master_seq : master_sequence_data) {
        if (master_seq.type_subtype == ts)
            min_dist = std::min(min_dist, hamming_distance(master_seq.aa, source, hamming_distance_by_shortest::yes));
    }
    return min_dist;

} // ae::sequences::min_hamming_distance_to_master

// ----------------------------------------------------------------------

std::pair<const ae::sequences::MasterSequence*, size_t> ae::sequences::closest_subtype_by_min_hamming_distance_to_master(const sequence_aa_t& source)
{
    std::pair<const ae::sequences::MasterSequence*, size_t> result{nullptr, source.size()};
    for (const auto& master_seq : master_sequence_data) {
        const auto hd = hamming_distance(master_seq.aa, source, hamming_distance_by_shortest::yes);
        if (hd < result.second) {
            result.first = &master_seq;
            result.second = hd;
        }
    }
    return result;

} // ae::sequences::closest_subtype_by_min_hamming_distance_to_master

// ----------------------------------------------------------------------

const ae::sequences::MasterSequence* ae::sequences::master_sequence_for(const ae::virus::type_subtype_t& ts)
{
    const auto hb = ts.h_or_b();
    for (const auto& master_seq : master_sequence_data) {
        if (hb == master_seq.type_subtype.h_or_b())
            return &master_seq;
    }
    return nullptr;

} // ae::sequences::master_sequence_for

// ----------------------------------------------------------------------