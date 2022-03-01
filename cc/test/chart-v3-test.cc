#include "chart/v3/chart.hh"

// ----------------------------------------------------------------------

int main(int /*argc*/, const char* const* /*argv*/)
{
    try {
        ae::antigen_index num{10};
        for (const auto ind : num)
            fmt::print(">>>> {}\n", ind);

        ae::chart::v3::Chart chart;
    }
    catch (std::exception& err) {
        fmt::print("> {}\n", err.what());
    }
    return 0;
}

// ----------------------------------------------------------------------
