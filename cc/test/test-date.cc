#include <array>

#include "ext/fmt.hh"
#include "ext/date.hh"

// ----------------------------------------------------------------------

static size_t date_parsing_test(bool verbose);

// ----------------------------------------------------------------------

int main(int argc, const char* const* argv)
{
    const bool verbose{argc > 1 && std::string_view{argv[1]} == "-v"};
    try {
        return static_cast<int>(date_parsing_test(verbose));
    }
    catch (std::exception& err) {
        fmt::print("> {}\n", err.what());
    }
    return 0;
}

// ----------------------------------------------------------------------

struct TestData
{
    std::string_view raw_name;
    std::string_view expected;
    ae::date::month_first month_first{ae::date::month_first::no};
};

size_t date_parsing_test(bool verbose)
{
    using TD = TestData;

    const std::array data{
        TD{"32-11-04", "1932-11-04"},
        TD{"1821-11-04", "1821-11-04"},
        TD{"1921-11-04", "1921-11-04"},
        TD{"2021-11-04", "2021-11-04"},
        TD{"2021-11-30", "2021-11-30"},
        // TD{"2021-11-31", "2021-11-31"},
        // TD{"2021-11-32", "2021-11-32"},
        TD{"2021-11", "2021-11-01"},
        TD{"2021", "2021-01-01"},
        TD{"2021/11/04", "2021-11-04"},
        TD{"4/11/2021", "2021-11-04"},
        TD{"4/11/2021", "2021-04-11", ae::date::month_first::yes},
        TD{"4/13/2021", "2021-04-13"},
        TD{"2007/03", "2007-03-01"},
    };

    size_t errors = 0;
    for (const auto [raw, expected, month_first] : data) {
        try {
            const auto result = ae::date::parse_and_format(raw, ae::date::allow_incomplete::yes, ae::date::throw_on_error::yes, month_first);
            if (result != expected) {
                fmt::print("> {:60s} <-- \"{}\"  expected: \"{}\"\n", fmt::format("\"{}\"", result), raw, expected);
                ++errors;
            }
            else if (verbose)
                fmt::print("  {:60s} <-- \"{}\"\n", fmt::format("\"{}\"", result), raw);
        }
        catch (std::exception& err) {
            fmt::print("> \"{}\": {}\n", raw, err.what());
            ++errors;
        }
    }
    if (errors)
        fmt::print("> {} errors found", errors);
    return errors;

} // datee_parsing_test

// ----------------------------------------------------------------------
