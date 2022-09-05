#include <cstdlib>
#include <array>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_session.hpp>

#include "utils/float.hh"
#include "chart/v3/chart.hh"

// ----------------------------------------------------------------------

TEST_CASE("best stress", "{stress]") {
    const char* ae_root = std::getenv("AE_ROOT");
    REQUIRE(ae_root != nullptr);

    ae::chart::v3::Chart chart{std::filesystem::path{ae_root} / "test" / "chart1.ace"};
    REQUIRE(chart.antigens().size() == ae::antigen_index{22});
    REQUIRE(chart.sera().size() == ae::serum_index{10});
    REQUIRE(chart.number_of_points() == ae::point_index{32});
    REQUIRE(chart.projections().empty());
    REQUIRE(chart.titers().number_of_antigens() == chart.antigens().size());
    REQUIRE(chart.titers().number_of_sera() == chart.sera().size());
    REQUIRE(chart.titers().number_of_non_dont_cares() == 220);
    const std::array expected_column_bases{3.0, 5.0, 4.0, 4.0, 5.0, 6.0, 5.0, 5.0, 6.0, 5.0};
    for (const auto sr_no : chart.sera().size())
        REQUIRE(float_equal(chart.titers().raw_column_basis(sr_no), expected_column_bases[*sr_no]));

    chart.relax(ae::chart::v3::number_of_optimizations_t{1000}, ae::chart::v3::minimum_column_basis{"none"}, ae::number_of_dimensions_t{2}, ae::chart::v3::optimization_options{});
    chart.projections().sort(chart);
    REQUIRE(std::abs(chart.projections().best().stress() - 66.12473) < 10e-4);
}

int main(int argc, const char* const* argv)
{
    return Catch::Session().run( argc, argv );
}

// ----------------------------------------------------------------------
