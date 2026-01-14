#include <audio_types.h>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <limits>
#include <map>
#include <sndfile.h>
#include <stdexcept>
#include <type_traits>
#include <utility>

TEST_CASE("FFTSize", "[audio_types]")
{

    SECTION("Constructor accepts valid power of 2 values")
    {
        CHECK_NOTHROW(FFTSize(1));
        CHECK_NOTHROW(FFTSize(2));
        CHECK_NOTHROW(FFTSize(32768));
        CHECK_NOTHROW(FFTSize(1048576));
        CHECK_NOTHROW(FFTSize(1 << 30));
    }

    SECTION("Constructor throws on invalid non-power of 2 values")
    {
        CHECK_THROWS_AS(FFTSize(0), std::invalid_argument);
        CHECK_THROWS_AS(FFTSize(3), std::invalid_argument);
        CHECK_THROWS_AS(FFTSize(5), std::invalid_argument);
        CHECK_THROWS_AS(FFTSize(255), std::invalid_argument);
        CHECK_THROWS_AS(FFTSize(1023), std::invalid_argument);
        CHECK_THROWS_AS(FFTSize(1025), std::invalid_argument);
    }

    SECTION("Constructor throws when value exceeds int max")
    {
        // INT_MAX is 2147483647, so 1 << 31 = 2147483648 exceeds it
        CHECK_THROWS_AS(FFTSize(1uLL << 31), std::invalid_argument);
        CHECK_THROWS_AS(FFTSize(1uLL << 32), std::invalid_argument);
        CHECK_THROWS_AS(FFTSize(std::numeric_limits<size_t>::max()), std::invalid_argument);
    }

    SECTION("Get() returns size_t")
    {
        const FFTSize kSize(512);
        size_t const value = kSize.Get();
        CHECK(value == 512);
        CHECK(std::is_same_v<decltype(kSize.Get()), size_t>);
    }

    SECTION("Constexpr evaluation")
    {
        constexpr bool test1 = FFTSize(16).Get() == 16;
        static_assert(test1, "16 should be a power of 2");
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
        const FramePosition kWant{ 750 };
        CHECK(kHave == kWant);
    }

    SECTION("AsPosition overflow detection")
    {
        const size_t kOverflowValue =
          static_cast<size_t>(std::numeric_limits<std::ptrdiff_t>::max()) + 1;
        CHECK_THROWS_AS(FrameCount(kOverflowValue).AsPosition(), std::overflow_error);
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
}

TEST_CASE("FrameIndex", "[audio_types]")
{
    SECTION("Construction and Get()")
    {
        CHECK(FrameIndex().Get() == 0); // Default constructor
        CHECK(FrameIndex(0).Get() == 0);
        CHECK(FrameIndex(100).Get() == 100);
        CHECK(FrameIndex(500000).Get() == 500000);
    }

    SECTION("Addition with FrameCount")
    {
        const FrameIndex kStart(100);
        const FrameCount kOffset(50);
        const auto kEnd = kStart + kOffset;
        CHECK(kEnd.Get() == 150);
    }

    SECTION("Comparison operators - greater than")
    {
        const FrameIndex kIdx1(100);
        const FrameIndex kIdx2(200);
        const FrameIndex kIdx3(200);

        CHECK(kIdx2 > kIdx1);
        CHECK_FALSE(kIdx1 > kIdx2);
        CHECK_FALSE(kIdx2 > kIdx3);
    }

    SECTION("Comparison operators - less than")
    {
        const FrameIndex kIdx1(100);
        const FrameIndex kIdx2(200);
        const FrameIndex kIdx3(200);

        CHECK(kIdx1 < kIdx2);
        CHECK_FALSE(kIdx2 < kIdx1);
        CHECK_FALSE(kIdx2 < kIdx3);
    }

    SECTION("Use with zero offset")
    {
        const FrameIndex kIdx(1000);
        const auto kSame = kIdx + FrameCount(0);
        CHECK(kSame.Get() == 1000);
    }

    SECTION("Large values")
    {
        const size_t kLargeValue = 1uLL << 40;
        const FrameIndex kIdx(kLargeValue);
        CHECK(kIdx.Get() == kLargeValue);
    }

    SECTION("Used as cache key")
    {
        // Test that FrameIndex can be used in std::pair and std::map (like in
        // SpectrogramController) Note: std::map uses operator< for ordering, not operator==
        const FrameIndex kIdx1(100);
        const FrameIndex kIdx2(200);
        const FrameIndex kIdx3(100);
        const ChannelCount kChannel0(0);

        std::map<std::pair<ChannelCount, FrameIndex>, int> cache;
        cache[{ kChannel0, kIdx1 }] = 1;
        cache[{ kChannel0, kIdx2 }] = 2;
        cache[{ kChannel0, kIdx3 }] = 3; // overwrites the first entry

        CHECK(cache.size() == 2);
        CHECK(cache[{ kChannel0, kIdx1 }] == 3);
        CHECK(cache[{ kChannel0, kIdx2 }] == 2);
    }
}

TEST_CASE("FramePosition", "[audio_types]")
{

    SECTION("Addition with FrameCount")
    {
        const FramePosition kStart(500);
        const FrameCount kOffset(250);
        const auto kEnd = kStart + kOffset;
        CHECK(kEnd.Get() == 750);
    }

    SECTION("Subtraction with FrameCount")
    {
        const FramePosition kStart(800);
        const FrameCount kOffset(300);
        const auto kEnd = kStart - kOffset;
        CHECK(kEnd.Get() == 500);
    }

    SECTION("Addition with FFTSize")
    {
        const FramePosition kStart(1000);
        const FFTSize kFFTSize = 512;
        const auto kEnd = kStart + kFFTSize;
        CHECK(kEnd.Get() == 1512);
    }

    SECTION("Subtraction with FFTSize")
    {
        const FramePosition kStart(2000);
        const FFTSize kFFTSize = 512;
        const auto kEnd = kStart - kFFTSize;
        CHECK(kEnd.Get() == 1488);
    }

    SECTION("Chained addition")
    {
        const FramePosition kStart(100);
        const FrameCount kCount(50);
        const FFTSize kFFTSize = 256;
        const auto kEnd = kStart + kCount + kFFTSize;
        CHECK(kEnd.Get() == 406);
    }
}