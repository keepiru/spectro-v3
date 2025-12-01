#include <catch2/catch_test_macros.hpp>

TEST_CASE("Build system verification", "[infrastructure]")
{
  REQUIRE(true);
}

TEST_CASE("Basic arithmetic works", "[sanity]")
{
  REQUIRE(2 + 2 == 4);
}
