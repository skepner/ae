#include <cstdlib>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>

#include "chart/v3/chart.hh"

// ----------------------------------------------------------------------

TEST_CASE("best stress", "{stress]") {
    const char* ae_root = std::getenv("AE_ROOT");
    REQUIRE(ae_root != nullptr);

    ae::chart::v3::Chart chart{std::filesystem::path{ae_root} / "test" / "chart1.ace"};
    chart.relax(ae::chart::v3::number_of_optimizations_t{1000}, ae::chart::v3::minimum_column_basis{"none"}, ae::number_of_dimensions_t{2}, ae::chart::v3::optimization_options{});
    chart.projections().sort(chart);
    REQUIRE(std::abs(chart.projections().best().stress() - 66.12473) < 10e-4);
}

int main(int argc, const char* const* argv)
{
    return Catch::Session().run( argc, argv );
}

// ----------------------------------------------------------------------
