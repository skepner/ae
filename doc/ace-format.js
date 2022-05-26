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
            {                                          //  Antigen list
                "N": "A(H3N2)/CAMBODIA/E0826360/2020", //  name: TYPE(SUBTYPE)/[HOST/]LOCATION/ISOLATION/YEAR or CDC_ABBR NAME or UNRECOGNIZED NAME
                "a": ["DISTINCT", "TC"],               // annotations that distinguish antigens (prevent from merging): ["DISTINCT"], mutation information, unrecognized extra data
                "D": "2021-04-23",                     //  YYYY[-]MM[-]DD | isolation date
                "L": "V",                              //  lineage: "Y[AMAGATA]" or "V[ICTORIA]"
                "P": "MDCK2/SIAT1 (2016-05-12)",       //  passage
                "R": "NYMC-51C",                       //  reassortant
                "l": ["CDC#2013706008"],               //  lab ids ([lab#id])
                "A": "QKIPGNDNSTATLCLG...",            //  aligned amino-acid sequence
                "B": "CAAAAAATTCCTGGAAAT...",          //  aligned nucleotide sequence
                "Ai": [[pos1, "aas"]]                  //  insertions at the aa level
                "Bi": [[pos1, "nucs"]]                 // insertions at the nucleotide level
                "T": {                                 //  semantic attributes by group
                    "C": ["3C" "3C.2a" "3C.2a1b.2a"],  // clades
                    "V": "es201902",                   // vaccine: egg/cell/reassortant, surrogate, year[month]
                    "C9": "ASIA",                      // continent: "ASIA", "AUSTRALIA-OCEANIA", "NORTH-AMERICA", "EUROPE", "RUSSIA", "AFRICA", "MIDDLE-EAST", "SOUTH-AMERICA", "CENTRAL-AMERICA"
                    "c9": "USA",                       // country: "UNITED KINGDOM"
                    "P": "e|c|r",                      // passage type: egg|cell|reassortant

                    "serology": "", //
                    "new": 1, // new since last tc, 2 tc ago

                                   // "NT": total number of tables from hidb
                                   // "RT": "CDC:HI:guinea-pig:20141009" - the most recent table id from hidb
                                   // "TS": "2021-12" time series
                                   // ? "layer": 1 - table series
                                   // "SC": [] - serum coverage data, N-fold for each serum
                },
                "C": "DEPRECATED: use s, ASIA", //  (DEPRECATED, use "s") continent: "ASIA", "AUSTRALIA-OCEANIA", "NORTH-AMERICA", "EUROPE", "RUSSIA", "AFRICA", "MIDDLE-EAST", "SOUTH-AMERICA", "CENTRAL-AMERICA"
                "c": ["DEPRECATED: use s"],     //  (DEPRECATED, use "s") clades, e.g. ["5.2.1"]
                "S": "DEPRECATED: use s"        //  (DEPRECATED, use "s") single letter semantic boolean attributes: R - reference, E - egg, V - current vaccine, v - previous vaccine, S - vaccine surrogate
            }
        ],
        "s": [
            {                                          // Serum list
                "N": "A(H3N2)/CAMBODIA/E0826360/2020", //  name: TYPE(SUBTYPE)/[HOST/]LOCATION/ISOLATION/YEAR or CDC_ABBR NAME or UNRECOGNIZED NAME
                "a": ["DISTINCT", "TC"],               // annotations that distinguish antigens (prevent from merging): ["DISTINCT"], mutation information, unrecognized extra data
                "s": "FERRET",                         // serum species
                "L": "V",                              //  lineage: "Y[AMAGATA]" or "V[ICTORIA]"
                "P": "MDCK2/SIAT1 (2016-05-12)",       //  passage
                "R": "NYMC-51C",                       //  reassortant
                "I": "CDC 2016-045",                   // serum id, e.g "CDC 2016-045"
                "A": "QKIPGNDNSTATLCLG...",            //  aligned amino-acid sequence
                "B": "CAAAAAATTCCTGGAAAT...",          //  aligned nucleotide sequence
                "Ai": [[pos1, "aas"]]                  // insertions at the aa level
                "Bi": [[pos1, "nucs"]]                 // insertions at the nucleotide level
                "T": {},                               //  semantic attributes by group (see above for antigen)
                "C": "DEPRECATED: use s, ASIA",        //  (DEPRECATED, use "s") continent: "ASIA", "AUSTRALIA-OCEANIA", "NORTH-AMERICA", "EUROPE", "RUSSIA", "AFRICA", "MIDDLE-EAST", "SOUTH-AMERICA", "CENTRAL-AMERICA"
                "c": ["DEPRECATED: use s"],            //  (DEPRECATED, use "s") clades, e.g. ["5.2.1"]
                "S": "DEPRECATED: use s",              //  (DEPRECATED, use "s") single letter semantic boolean attributes: R - reference, E - egg, V - current vaccine, v - previous vaccine, S - vaccine surrogate
                "h": []                                // DEPRECATED homologous antigen indices, e.g. [0]
            }
        ],
        "t": {                                  // Titers
            "l": [["80", "160"],["80", "160"]], // dense matrix of titers
            "d": [ {str: str} ],                // sparse matrix, entry for each antigen present, key is serum index, value is titer, dont-care titers omitted
            "L": [ {str: str} ]                 // layers of titers, each top level array element as in "d" or "l"
        },
        "C": [7.0, 8.0], // forced column bases for a new projections
        "P": [                                      // Projections
            {
                "c": "",                            // comment
                "l": [ [1.3, 0.7], [], [1.5, 0.5] ] // layout, if point is disconnected: empty list or ?[NaN, NaN]
                "s": 1979.121,                      // stress
                "m": "none",                        // minimum column basis, "none" (default), "1280"
                "C": [7.0, 8.0],                    // forced column bases
                "t": [1.0, 0.0, 0.0, 1.0],          // transformation matrix
                "g": [1.0, 1.0],                    // antigens_sera_gradient_multipliers, float for each point
                "f": [1.0, 1.0],                    // avidity adjusts (antigens_sera_titers_multipliers), float for each point
                "d": false,                         // dodgy_titer_is_regular, false is default
                "e": 0.0001,                        // stress_diff_to_stop
                "U": [194,195],                     // list of indices of unmovable points (antigen/serum attribute for stress evaluation)
                "D": [196,197],                     // list of indices of disconnected points (antigen/serum attribute for stress evaluation)
                "u": [198, 199],                    // list of indices of points unmovable in the last dimension (antigen/serum attribute for stress evaluation)
                "i": 1                              // UNUSED number of iterations?
            }
        ],
        "R": {                                        // sematic attributes based plot specifications, key: name of the style, value: style object
            "z": 0,                                   // priority order when showing in GUI
            "t": "",                                  // title
            "V": [-5.0, -5.0, 10.0, 10.0],            // viewport
            "A": {                                    // modifiers to apply
                "R": "-clades",                       // name ("N") of another plot spec to use (inherited from), applied before adding other changes provided by this object
                "T": {                                // to select antigens/sera, if value is en empty string, it means ag/sr selected if they have that semantic attribute with any value
                    "<name of semantic attribute>": <value>,
                    "!D": ["date-first", "date-last"] // select antigens with isolation date in range, if antigen date is absent, it is ""
                                                      // if date-last is "", it means until now
                    "!i": 0,                          // antigen/serum index, i.e. individual selection
                },
                "A": 1,                               // true or 1: select antigens only, false or 0: select sera only, absent or -1: select antigens and sera
                "S": "C",                             // shape: "C[IRCLE]" (default), "B[OX]", "T[RIANGLE]", "E[GG]", "U[GLYEGG]"
                "F": "transparent",                   // fill color
                "O": "black",                         // outline color
                "o": 1.0,                             // outline width
                "s": 1.0,                             // size, default 1.0
                "r": 0.0,                             // rotation in radians, default 0.0
                "a": 1.0,                             // aspect ratio, default 1.0
                "-": false,                           // hide point and its label
                "D": "r",                             // drawing order: raise, lower, absent: no change
                "l": {                                // object label style -> Offset + TextData
                    "-": false,                       // if label is hidden
                    "p":  [0, 1],                      // [x, y]: label offset (2D only), list of two doubles, default is [0, 1] means under point
                    "t": "label",                     // label text if forced by user
                    "f": "helvetica",,                // font face
                    "S": "normal",                    // font slant: "normal" (default), "italic"
                    "W": "normal",                    // font weight: "normal" (default), "bold"
                    "s": 1.0,                         // label size, default 1.0
                    "c": "black",,                    // label color, default: "black"
                    "r": 0.0,                         // label rotation, default 0.0
                    "i": 0.2                          // addtional interval between lines as a fraction of line height
                },
                "L" {                                 //legend row
                    "p": 0,                           // priority
                    "t": "legend row"
                }
            },
            "T" {                                                  // Title
                "-": false,                                        // hidden
                "B": {                                             // box area
                    "o":  "tl",                                     // box relative to, two letter code
                                                                   //   first letter is for vertical position:
                                                                   //     T - origin is bottom of the box relative to the top of the plot, i.e. box is above the plot
                                                                   //     t - origin is top of the box relative to the top of the plot, i.e. box is within the plot
                                                                   //     c - box is centered vertically
                                                                   //     B - origin is bottom of the box relative to the bottom of the plot, i.e. box is within the plot
                                                                   //     b - origin is top of the box relative to the bottom of the plot, i.e. box is above the plot
                                                                   //   second letter is for horizontal position:
                                                                   //     L - origin is right of the box relative to the left of the plot, i.e. box is to the left of the plot
                                                                   //     l - origin is left of the box relative to the left of the plot, i.e. box is within the plot
                                                                   //     c - box is centered horizontally
                                                                   //     R - origin is right of the box relative to the right of the plot, i.e. box is within the plot
                                                                   //     r - origin is left of the box relative to the right of the plot, i.e. box is to the right of the plot

                    "p": {"t": 0.0, "b": 0.0, "l": 0.0, "r": 0.0}, //  padding
                    "O": [10, 10],                                 // offset relative to origin (pixels)
                    "B": "black",                                  // border color
                    "W":  0.0,                                      // border width, 0 - no border
                    "F": "transparent",                            // background
                },
                "T": {                                             // :text: style of text
                    "t": "line\nline",                             //  multi-line text, lines are separated by \n
                    "f": "helvetica",                              // font face, "monospace", "sansserif", "serif", "helvetica", "courier", "times"
                    "S": "normal",                                 // font slant: "normal" (default), "italic"
                    "W": "bold",                                   // font weight: "normal" (default), "bold"
                    "s":  16.0,                                     // font size
                    "c": "black",                                  // text color
                    "i":  0.2                                      // addtional interval between lines as a fraction of line height
                }
            },
            "L": {                                                 //  legend data
                "-": false,                                        // hidden
                "C": true,                                         // add counter
                "S": 10.0,                                         // point size
                "z": true,                                         // show rows with zero count
                "B": {                                             //  :box: box area
                    "o":  "tl",                                     // box relative to, two letter code (see above)
                    "p": {"t": 0.0, "b": 0.0, "l": 0.0, "r": 0.0}, // padding
                    "O": [10, 10],                                 // offset relative to origin (pixels)
                    "B": "black",                                  // Color border color
                    "W": 0.0,                                      // border width, 0 - no border
                    "F": "transparent",                            // Color background
                },
                "t": {                                             // object :text: style of text in a legend row
                    "f": "helvetica"                               // "monospace", "sansserif", "serif", "helvetica", "courier", "times"
                    "S": "normal"                                  // font slant: "normal" (default), "italic"
                    "W": "normal"                                  // font weight: "normal" (default), "bold"
                    "s": 1.0,                                      // label size, default 1.0
                    "c": "black",                                  //  label color, default: "black"
                    "i":  0.2                                       // addtional interval between lines as a fraction of line height
                },
                "T": {                                             // object :text: title and its style
                    "t": "title text",                             // title text, title is not shown if text is empty, text may include newlines
                    "f": "helvetica",                              // font face, "monospace", "sansserif", "serif", "helvetica", "courier", "times"
                    "S": "normal",                                 // font slant: "normal" (default), "italic"
                    "W": "bold",                                   // font weight: "normal" (default), "bold"
                    "s": 1.0,                                      // label size, default 1.0
                    "c": "black",                                  // label color, default: "black"
                    "i": 0.2                                       //addtional interval between lines as a fraction of line height
                }
            },
        },


        "p": {                             //legacy lispmds stype plot specification
            "d": [0, 1, 2, 3],             // drawing order, point indices
            "E": {"c": "blue"},            // error line positive, default: {"c": "blue"}
            "e": {"c": "red"},             // error line negative, default: {"c": "red"}
            "g": {},                       // ? grid data
            "P": [                         // list of plot styles
                {
                    "+": true,             // if point is shown, default is true, disconnected points are usually not shown and having NaN coordinates in layout
                    "F": "transparent",    // fill color: #FF0000 or T[RANSPARENT] or color name (red, green, blue, etc.), default is transparent
                    "O": "black",          // outline color: #000000 or T[RANSPARENT] or color name (red, green, blue, etc.), default is black
                    "o": 1.0,              // outline width, default 1.0
                    "S": "C",              // shape: "C[IRCLE]" (default), "B[OX]", "T[RIANGLE]", "E[GG]", "U[GLYEGG]"
                    "s": 1.0,              // size, default 1.0
                    "r": 0.0,              // rotation in radians, default 0.0
                    "a": 1.0,              // aspect ratio, default 1.0
                    "l": {                 // label style  -> Offset + TextData
                        "+": boolean,      // if label is shown
                        "p": [0.0, 1.0],   // label position (2D only), list of two doubles, default is [0, 1] means under point
                        "t": "label text", // label text if forced by user
                        "f": "helvetica",  // font face
                        "S": "normal",     // font slant: "normal" (default), "italic"
                        "W": "normal",     // font weight: "normal" (default), "bold"
                        "s": 1.0,          // label size, default 1.0
                        "c": "black",      // label color
                        "r": 0.0,          // label rotation
                        "i": 0.2,          // addtional interval between lines as a fraction of line height
                    }
                }
            ],
            "p": [0, 0, 0],                // index in "P" for each point, antigens followed by sera
            "l": [0, 0],                   // ? for each procrustes line, index in the "L" list
            "L": [],                       // ? list of procrustes lines styles
            "s": [0, 1],                   // list of point indices for point shown on all maps in the time series
            "t": {}                        // ? title style
        },
        "x": { // extensions not used by acmacs
        }
    }
}
