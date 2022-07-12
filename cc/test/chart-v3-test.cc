#include "chart/v3/chart.hh"

// ----------------------------------------------------------------------

int main(int argc, const char* const* argv)
{
    try {
        // ae::antigen_index num{10};
        // for (const auto ind : num)
        //     ; // fmt::print(">>>> {}\n", ind);

        if (argc > 1) {
            ae::chart::v3::Chart chart{std::filesystem::path{argv[1]}};
            chart.write("/r/a.ace");
        }
    }
    catch (std::exception& err) {
        fmt::print("> {}\n", err.what());
    }
    return 0;
}

// ----------------------------------------------------------------------
