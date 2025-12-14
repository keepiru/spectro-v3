#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <dsp_utils.h>

TEST_CASE("DSPUtils::MagnitudeToDecibels", "[dsp_utils]")
{
    SECTION("Simple magnitude calculations are correct")
    {
        REQUIRE(DSPUtils::MagnitudeToDecibels(0.0001) == -80.0);
        REQUIRE(DSPUtils::MagnitudeToDecibels(0.1) == -20.0);
        REQUIRE(DSPUtils::MagnitudeToDecibels(1.0) == 0.0);
        REQUIRE(DSPUtils::MagnitudeToDecibels(10) == 20.0);
        REQUIRE_THAT(DSPUtils::MagnitudeToDecibels(42),
                     Catch::Matchers::WithinAbs(32.4576F, 0.01F));
        REQUIRE(DSPUtils::MagnitudeToDecibels(100) == 40.0);
        REQUIRE(DSPUtils::MagnitudeToDecibels(1000) == 60.0);
    }

    SECTION("Zero magnitude returns very negative value")
    {
        REQUIRE(DSPUtils::MagnitudeToDecibels(0.0F) == -1000.0F);
    }

    SECTION("Negative magnitude returns very negative value")
    {
        REQUIRE(DSPUtils::MagnitudeToDecibels(-1.0F) == -1000.0F);
    }
}
