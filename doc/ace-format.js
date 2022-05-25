// -*- js-indent-level: 4 -*-
{
    "  version": "acmacs-ace-v1",
    "?created": "",
    "c": {                               // single chart data
        "i" : {                          //  chart meta information
            "v": "INFLUENZA",            // virus, e.g. INFLUENZA (default, if omitted), HPV, generic, DENGE
            "V": "A(H3N2)",              // virus type and subtype, e.g. B or A(H3N2) or serotype
            "A": "HINT",                 // assay: HI, HINT, FRA, FOCUST REDUCTION, PRNT
            "D": "20160602.002",         // table/assay date and number (if multiple on that day), e.g. 20160602.002
            "N": "",                     // user supplied name
            "l": "CDC",                  // lab
            "r": "turkey",               // RBCs species of HI assay, e.g. "turkey"
            "s": "UNUSED: 2009PDM",      // UNUSED subset/lineage, e.g. "2009PDM"
            "T": "A",                    // table type "A[NTIGENIC]" - default, "G[ENETIC]"
            "S": {                       // source table info list, each entry is like "i"
                "D": "20160602",
            }
        },
        "a": [
            {                           //  Antigen list
                "N": "A(H3N2)/CAMBODIA/E0826360/2020", //  name: TYPE(SUBTYPE)/[HOST/]LOCATION/ISOLATION/YEAR or CDC_ABBR NAME or UNRECOGNIZED NAME
                "a": ["DISTINCT", "TC"], // annotations that distinguish antigens (prevent from merging): ["DISTINCT"], mutation information, unrecognized extra data
                "D": "2021-04-23", //  YYYY[-]MM[-]DD | isolation date
                "L": "V", //  lineage: "Y[AMAGATA]" or "V[ICTORIA]"
                "P": "MDCK2/SIAT1 (2016-05-12)", //  passage
                "R": "NYMC-51C", //  reassortant
                "l": ["CDC#2013706008"], //  lab ids ([lab#id])
                "A": "QKIPGNDNSTATLCLG...", //  aligned amino-acid sequence
                "B": "CAAAAAATTCCTGGAAAT...", //  aligned nucleotide sequence
                "Ai": [[pos1, "aas"]] //  insertions at the aa level
                "Bi": [[pos1, "nucs"]] // insertions at the nucleotide level
                "T": { //  semantic attributes by group
                },
                "C": "DEPRECATED: use s, ASIA", //  (DEPRECATED, use "s") continent: "ASIA", "AUSTRALIA-OCEANIA", "NORTH-AMERICA", "EUROPE", "RUSSIA", "AFRICA", "MIDDLE-EAST", "SOUTH-AMERICA", "CENTRAL-AMERICA"
                "c": ["DEPRECATED: use s"], //  (DEPRECATED, use "s") clades, e.g. ["5.2.1"]
                "S": "DEPRECATED: use s" //  (DEPRECATED, use "s") single letter semantic boolean attributes: R - reference, E - egg, V - current vaccine, v - previous vaccine, S - vaccine surrogate
            }
        ],
        "s": [
            {                           //  Serum list
                "N": "A(H3N2)/CAMBODIA/E0826360/2020", //  name: TYPE(SUBTYPE)/[HOST/]LOCATION/ISOLATION/YEAR or CDC_ABBR NAME or UNRECOGNIZED NAME
                "a": ["DISTINCT", "TC"], // annotations that distinguish antigens (prevent from merging): ["DISTINCT"], mutation information, unrecognized extra data
                "s": "FERRET", // serum species
                "L": "V", //  lineage: "Y[AMAGATA]" or "V[ICTORIA]"
                "P": "MDCK2/SIAT1 (2016-05-12)", //  passage
                "R": "NYMC-51C", //  reassortant
                "I": "CDC 2016-045", // serum id, e.g "CDC 2016-045"                                                                                                                                              |
                "A": "QKIPGNDNSTATLCLG...", //  aligned amino-acid sequence
                "B": "CAAAAAATTCCTGGAAAT...", //  aligned nucleotide sequence
                "Ai": [[pos1, "aas"]] //  insertions at the aa level
                "Bi": [[pos1, "nucs"]] // insertions at the nucleotide level
                "T": { //  semantic attributes by group
                },
                "C": "DEPRECATED: use s, ASIA", //  (DEPRECATED, use "s") continent: "ASIA", "AUSTRALIA-OCEANIA", "NORTH-AMERICA", "EUROPE", "RUSSIA", "AFRICA", "MIDDLE-EAST", "SOUTH-AMERICA", "CENTRAL-AMERICA"
                "c": ["DEPRECATED: use s"], //  (DEPRECATED, use "s") clades, e.g. ["5.2.1"]
                "S": "DEPRECATED: use s", //  (DEPRECATED, use "s") single letter semantic boolean attributes: R - reference, E - egg, V - current vaccine, v - previous vaccine, S - vaccine surrogate
                "h": [] // DEPRECATED homologous antigen indices, e.g. [0]
            }
        ],
        "t": { // Titers
            "l": [["80", "160"],["80", "160"]], //         dense matrix of titers
            "d": [ {str: str} ],    //  sparse matrix, entry for each antigen present, key is serum index, value is titer, dont-care titers omitted
            "L": [ {str: str} ]    //  layers of titers, each top level array element as in "d" or "l"
        },
        "C": [7.0, 8.0], // forced column bases for a new projections
        "P": [ // Projections
            {
                "c": "",         // comment
                "l": [ [1.3, 0.7], [], [1.5, 0.5] ] // layout, if point is disconnected: empty list or ?[NaN, NaN]
                "i": 1,     // UNUSED number of iterations?
                "s": 1979.121,       // stress
                "m": "none",         // minimum column basis, "none" (default), "1280"
                "C": [7.0, 8.0],     // forced column bases
                "t": [1.0, 0.0, 0.0, 1.0],     // transformation matrix
                "g": [1.0, 1.0],     // antigens_sera_gradient_multipliers, float for each point
                "f": [1.0, 1.0],     // avidity adjusts (antigens_sera_titers_multipliers), float for each point
                "d": false,     // dodgy_titer_is_regular, false is default
                "e": 0.0001,       // stress_diff_to_stop
                "U": [194,195],       // list of indices of unmovable points (antigen/serum attribute for stress evaluation)
                "D": [196,197],       // list of indices of disconnected points (antigen/serum attribute for stress evaluation)
                "u": [198, 199] // list of indices of points unmovable in the last dimension (antigen/serum attribute for stress evaluation)
            }
        ],
    }
}
