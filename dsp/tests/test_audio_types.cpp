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

        CHECK(kFC * ChannelCount(1) == SampleCount(1000));        // mono
        CHECK(kFC * ChannelCount(2) == SampleCount(2000));        // stereo
        CHECK(kFC * ChannelCount(6) == SampleCount(6000));        // multichannel
        CHECK(FrameCount(0) * ChannelCount(2) == SampleCount(0)); // zero frames
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

    SECTION("AsPosition conversion")
    {
        const FrameCount kFC(750);
        const FramePosition kHave = kFC.AsPosition();
        const FramePosition kWant = 750;
        CHECK(kHave == kWant);
    }

    SECTION("AsPtrDiffT conversion")
    {
        const FrameCount kFC(1234);
        const std::ptrdiff_t kHave = kFC.AsPtrDiffT();
        const std::ptrdiff_t kWant = 1234;
        CHECK(kHave == kWant);
    }
}

TEST_CASE("SampleCount", "[audio_types]")
{
    SECTION("Construction and Get()")
    {
        CHECK(SampleCount().Get() == 0); // Default constructor
        CHECK(SampleCount(0).Get() == 0);
        CHECK(SampleCount(100).Get() == 100);
        CHECK(SampleCount(1000000).Get() == 1000000);
    }

    SECTION("Equality operator")
    {
        const SampleCount kSC1(500);
        const SampleCount kSC2(500);
        const SampleCount kSC3(1000);

        CHECK(kSC1 == kSC2);
        CHECK_FALSE(kSC1 == kSC3);
    }

    SECTION("AsPtrDiffT conversion")
    {
        const SampleCount kSC(5678);
        const std::ptrdiff_t kHave = kSC.AsPtrDiffT();
        const std::ptrdiff_t kWant = 5678;
        CHECK(kHave == kWant);
    }

    SECTION("Large values")
    {
        const size_t kLargeValue = 1uLL << 32;
        const SampleCount kSC(kLargeValue);
        CHECK(kSC.Get() == kLargeValue);
    }
}

TEST_CASE("SampleIndex", "[audio_types]")
{
    SECTION("Construction and Get()")
    {
        CHECK(SampleIndex().Get() == 0); // Default constructor
        CHECK(SampleIndex(0).Get() == 0);
        CHECK(SampleIndex(250).Get() == 250);
        CHECK(SampleIndex(500000).Get() == 500000);
    }

    SECTION("Addition with SampleCount")
    {
        const SampleIndex kStart(100);
        const SampleCount kOffset(50);
        const auto kEnd = kStart + kOffset;
        CHECK(kEnd.Get() == 150);
    }

    SECTION("Addition with FFTSize")
    {
        const SampleIndex kStart(1000);
        const FFTSize kFFTSize = 512;
        const auto kEnd = kStart + kFFTSize;
        CHECK(kEnd.Get() == 1512);
    }

    SECTION("Comparison operators")
    {
        const SampleIndex kIdx1(100);
        const SampleIndex kIdx2(200);
        const SampleIndex kIdx3(200);

        CHECK(kIdx1 < kIdx2);
        CHECK(kIdx2 > kIdx1);
        CHECK_FALSE(kIdx2 < kIdx1);
        CHECK_FALSE(kIdx1 > kIdx2);
        CHECK_FALSE(kIdx2 < kIdx3);
        CHECK_FALSE(kIdx2 > kIdx3);
    }

    SECTION("AsPtrDiffT conversion")
    {
        const SampleIndex kIdx(9876);
        const std::ptrdiff_t kHave = kIdx.AsPtrDiffT();
        const std::ptrdiff_t kWant = 9876;
        CHECK(kHave == kWant);
    }

    SECTION("Chained addition")
    {
        const SampleIndex kStart(100);
        const SampleCount kCount(50);
        const FFTSize kFFTSize = 256;
        const auto kEnd = kStart + kCount + kFFTSize;
        CHECK(kEnd.Get() == 406);
    }
}