#include <catch2/catch_test_macros.hpp>
#include <fft_processor.h>

TEST_CASE("Constructor succeeds")
{
  const uint32_t bins = 1024;
  FFTProcessor processor(bins);

  REQUIRE(processor.getNumBins() == bins);
}