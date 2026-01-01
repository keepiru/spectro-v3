#include "include/global_constants.h"
#include "models/audio_buffer.h"
#include <QSignalSpy>
#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include <vector>

TEST_CASE("AudioBuffer constructor", "[audio_buffer]")
{
    AudioBuffer const buffer;
    REQUIRE(buffer.GetChannelCount() == 2);
    REQUIRE(buffer.GetSampleRate() == 44100);
}

TEST_CASE("AudioBuffer::Reset throws on invalid arguments", "[audio_buffer]")
{
    AudioBuffer buffer;

    REQUIRE_THROWS_AS(buffer.Reset(-1, 44100), std::invalid_argument);
    REQUIRE_THROWS_AS(buffer.Reset(0, 44100), std::invalid_argument);
    REQUIRE_THROWS_AS(buffer.Reset(GKMaxChannels + 1, 44100), std::invalid_argument);
    REQUIRE_THROWS_AS(buffer.Reset(2, 0), std::invalid_argument);
}

TEST_CASE("AudioBuffer::AddSamples succeeds with valid size", "[audio_buffer]")
{
    AudioBuffer buffer;
    buffer.AddSamples({ 1, 2, 3, 4 });
}

TEST_CASE("AudioBuffer::AddSamples throws on invalid size", "[audio_buffer]")
{
    AudioBuffer buffer;
    REQUIRE_THROWS_AS(buffer.AddSamples({ 1, 2, 3, 4, 5 }), std::invalid_argument);
}

TEST_CASE("AudioBuffer::GetSamples throws if insufficient samples", "[audio_buffer]")
{
    AudioBuffer buffer;
    buffer.AddSamples({ 1, 2, 3, 4 });
    REQUIRE_THROWS_AS((void)buffer.GetSamples(1, 1, 4), std::out_of_range);
}

TEST_CASE("AudioBuffer::GetSamples throws on invalid channel index", "[audio_buffer]")
{
    AudioBuffer const buffer;
    (void)buffer.GetSamples(1, 0, 0); // No exception
    REQUIRE_THROWS_AS((void)buffer.GetSamples(2, 0, 0), std::out_of_range);
}

TEST_CASE("AudioBuffer::AddSamples deinterleaves to channel buffers", "[audio_buffer]")
{
    AudioBuffer buffer;
    buffer.AddSamples({ 1, 2, 3, 4 });

    REQUIRE(buffer.GetSamples(0, 0, 2) == std::vector<float>({ 1, 3 }));
    REQUIRE(buffer.GetSamples(1, 0, 2) == std::vector<float>({ 2, 4 }));
}

TEST_CASE("AudioBuffer::AddSamples emits signal", "[audio_buffer]")
{
    AudioBuffer buffer;
    QSignalSpy spy(&buffer, &AudioBuffer::DataAvailable);
    buffer.AddSamples({ 1, 2, 3, 4 });

    REQUIRE(spy.count() == 1);
    auto firstCallArgs = spy.takeFirst();
    auto have = firstCallArgs.takeFirst().value<int64_t>();
    REQUIRE(have == 2);
}

TEST_CASE("AudioBuffer::Reset clears samples", "[audio_buffer]")
{
    AudioBuffer buffer;
    buffer.AddSamples({ 1, 2, 3, 4 });

    REQUIRE(buffer.GetFrameCount() == 2);

    buffer.Reset(2, 44100);
    REQUIRE(buffer.GetFrameCount() == 0);
    REQUIRE_THROWS_AS((void)buffer.GetSamples(0, 0, 1), std::out_of_range);

    // Also test after changing channel count
    buffer.AddSamples({ 5, 6, 7, 8 });
    buffer.Reset(1, 44100);

    REQUIRE(buffer.GetFrameCount() == 0);
}

TEST_CASE("AudioBuffer::Reset changes channel count and sample rate", "[audio_buffer]")
{
    AudioBuffer buffer;

    REQUIRE(buffer.GetChannelCount() == 2);
    REQUIRE(buffer.GetSampleRate() == 44100);

    buffer.Reset(1, 22050);

    REQUIRE(buffer.GetChannelCount() == 1);
    REQUIRE(buffer.GetSampleRate() == 22050);
}

TEST_CASE("AudioBuffer::Reset emits buffer reset signal", "[audio_buffer]")
{
    AudioBuffer buffer;
    const QSignalSpy spy(&buffer, &AudioBuffer::BufferReset);

    REQUIRE(spy.count() == 0);

    buffer.Reset(2, 44100);
    REQUIRE(spy.count() == 1);

    buffer.AddSamples({ 1, 2, 3, 4 });
    REQUIRE(spy.count() == 1);

    buffer.Reset(2, 44100);
    REQUIRE(spy.count() == 2);
}
