#include <audio_types.h>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <limits>
#include <sndfile.h>
#include <stdexcept>

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

TEST_CASE("FrameCount", "[audio_types]")
{
    SECTION("Construction and Get()")
    {
        CHECK(FrameCount().Get() == 0); // Default constructor
        CHECK(FrameCount(0).Get() == 0);
        CHECK(FrameCount(100).Get() == 100);
        CHECK(FrameCount(1000000).Get() == 1000000);
    }

    SECTION("Equality operator")
    {
        const FrameCount kFC1(100);
        const FrameCount kFC2(100);
        const FrameCount kFC3(200);

        CHECK(kFC1 == kFC2);
        CHECK_FALSE(kFC1 == kFC3);
    }

    SECTION("Multiplication with ChannelCount")
    {
        const FrameCount kFC(1000);

        CHECK(kFC * ChannelCount(1) == 1000);        // mono
        CHECK(kFC * ChannelCount(2) == 2000);        // stereo
        CHECK(kFC * ChannelCount(6) == 6000);        // multichannel
        CHECK(FrameCount(0) * ChannelCount(2) == 0); // zero frames
    }

    SECTION("ToIntChecked - valid values")
    {
        CHECK(FrameCount(100).ToIntChecked() == 100);
        CHECK(FrameCount(0).ToIntChecked() == 0);

        // Max int value
        const FrameCount kFC(std::numeric_limits<int>::max());
        CHECK(kFC.ToIntChecked() == std::numeric_limits<int>::max());
    }

    SECTION("ToIntChecked - overflow detection")
    {
        // Value exceeding int max
        const size_t kOverflowValue = static_cast<size_t>(std::numeric_limits<int>::max()) + 1;
        CHECK_THROWS_AS(FrameCount(kOverflowValue).ToIntChecked(), std::overflow_error);
    }

    SECTION("ToSfCountT conversion")
    {
        const FrameCount kFC(500);
        const sf_count_t kHave = kFC.ToSfCountT();
        const sf_count_t kWant = 500;
        CHECK(kHave == kWant);
    }

    SECTION("AsOffset conversion")
    {
        const FrameCount kFC(750);
        const FrameOffset kHave = kFC.AsOffset();
        const FrameOffset kWant = 750;
        CHECK(kHave == kWant);
    }
}