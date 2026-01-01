#include <audio_types.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("IsPowerOf2", "[audio_types]")
{

    SECTION("Valid power of 2 values")
    {
        CHECK(IsPowerOf2(1));
        CHECK(IsPowerOf2(2));
        CHECK(IsPowerOf2(32768));
        CHECK(IsPowerOf2(1048576));
        CHECK(IsPowerOf2(1 << 30));
    }

    SECTION("Invalid non-power of 2 values")
    {
        CHECK_FALSE(IsPowerOf2(0));
        CHECK_FALSE(IsPowerOf2(3));
        CHECK_FALSE(IsPowerOf2(5));
        CHECK_FALSE(IsPowerOf2(255));
        CHECK_FALSE(IsPowerOf2(1023));
        CHECK_FALSE(IsPowerOf2(1025));
    }

    SECTION("Negative values are not powers of 2")
    {
        CHECK_FALSE(IsPowerOf2(-1));
        CHECK_FALSE(IsPowerOf2(-2));
        CHECK_FALSE(IsPowerOf2(-1024));
    }

    SECTION("Constexpr evaluation")
    {
        constexpr bool test1 = IsPowerOf2(16);
        constexpr bool test2 = IsPowerOf2(18);
        static_assert(test1, "16 should be a power of 2");
        static_assert(!test2, "18 should not be a power of 2");
    }
}