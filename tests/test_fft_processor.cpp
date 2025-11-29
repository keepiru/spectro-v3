#include <catch2/catch_test_macros.hpp>
#include <fft_processor.h>

TEST_CASE("Constructor succeeds")
{
  const uint32_t bins = 1024;
  FFTProcessor processor(bins);

  REQUIRE(processor.getNumBins() == bins);

  SECTION("Power of 2 bins are accepted")
  {
    REQUIRE_NOTHROW(FFTProcessor(256));
    REQUIRE_NOTHROW(FFTProcessor(512));
    REQUIRE_NOTHROW(FFTProcessor(2048));
  }

  SECTION("Non-power of 2 bins are rejected")
  {
    REQUIRE_THROWS_AS(FFTProcessor(300), std::invalid_argument);
    REQUIRE_THROWS_AS(FFTProcessor(1000), std::invalid_argument);
    REQUIRE_THROWS_AS(FFTProcessor(1500), std::invalid_argument);
  }
}