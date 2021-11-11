#include "ext/fmt.hh"
#include "ext/range-v3.hh"
#include "utils/messages.hh"
#include "virus/passage-parse.hh"

static size_t passage_parsing_test(bool verbose);

// ======================================================================

int main(int argc, const char* const* argv)
{
    const bool verbose{argc > 1 && std::string_view{argv[1]} == "-v"};
    try {
        return static_cast<int>(passage_parsing_test(verbose));
    }
    catch (std::exception& err) {
        fmt::print("> {}\n", err.what());
        return 1967;
    }
}

// ----------------------------------------------------------------------

struct D
{
    std::string_view raw_name;
    std::string_view expected;
};

size_t passage_parsing_test(bool verbose)
{
    const std::array data{
        D{"C1", "MDCK1"},                                                                                                       //
        D{"MDCK1", "MDCK1"},                                                                                                    //
        D{"MDCK1/SIAT1", "MDCK1/SIAT1"},                                                                                        //
        D{"MDCK1,SIAT1", "MDCK1/SIAT1"},                                                                                        //
        D{"MDCK1, SIAT1", "MDCK1/SIAT1"},                                                                                        //
        D{"MDCKX,MDCK1", "MDCK?/MDCK1"},                                                                                        //
        D{"MDCKX, MDCK1", "MDCK?/MDCK1"},                                                                                       //
        D{"C2+C1", "MDCK2/MDCK1"},                                                                                              //
        D{"MDCK 2 +1", "MDCK2/MDCK1"},                                                                                          //
        D{"MDCKx\\MDCK2", "MDCK?/MDCK2"},                                                                                       //
        D{"X1", "X1"},                                                                                                          //
        D{"X", "X?"},                                                                                                           //
        D{"X/MDCK1", "X?/MDCK1"},                                                                                               //
        D{"E2 (2012-11-01)", "E2 (2012-11-01)"},                                                                                //
        D{"passage details: MDCKX, MDCK2", "MDCK?/MDCK2"},                                                                      //
        D{"Original", "OR"},                                                                                                    //
        D{"Original Specimen", "OR"},                                                                                           //
        D{"Original Sample", "OR"},                                                                                             //
        D{"passage: Original", "OR"},                                                                                           //
        D{"passage details: original specimen", "OR"},                                                                          //
        D{"Clinical Specimen", "CS"},                                                                                           //
        D{"10 passages - embryonated chicken eggs; Passage Line 5", "*10 passages - embryonated chicken eggs; Passage Line 5"}, //
    };

    size_t errors = 0;
    ae::virus::passage::parse_settings settings{ae::virus::passage::parse_settings::tracing::no};
    for (const auto [no, entry] : ranges::views::enumerate(data)) {
        try {
            ae::Messages messages;
            const auto result = ae::virus::passage::parse(entry.raw_name, settings, messages, ae::MessageLocation{"test", no});
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
        fmt::print("> {} errors found", errors);
    return errors;

} // passage_parsing_test

// ----------------------------------------------------------------------
