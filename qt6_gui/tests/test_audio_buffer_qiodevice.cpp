// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "audio_types.h"
#include "include/global_constants.h"
#include "models/audio_buffer.h"
#include "models/audio_buffer_qiodevice.h"
#include <QBuffer>
#include <QIODevice>
#include <QtTypes>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_range_equals.hpp>
#include <span>
#include <vector>

namespace {
using Catch::Matchers::RangeEquals;

std::span<const float>
QByteArrayToSpanFloat(const QByteArray& byteArray)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto* asFloat = reinterpret_cast<const float*>(byteArray.data());
    return { asFloat, byteArray.size() / sizeof(float) };
};

struct AudioBufferQIODeviceTestFixture
{
    // NOLINTBEGIN(misc-non-private-member-variables-in-classes)
    AudioBuffer audio_buffer;

    // This is the device we are testing
    AudioBufferQIODevice dev{ audio_buffer };

    // And this is a reference QBuffer that we will use to verify the output of
    // our device.  We will initialize it with the same data as the audio
    // buffer, so that we can compare the output of our device to the output of
    // a known-good implementation (QBuffer).
    QBuffer ref;
    BytesPerFrame bytes_per_frame = 0;
    // NOLINTEND(misc-non-private-member-variables-in-classes)

    AudioBufferQIODeviceTestFixture()
    {
        std::vector<float> samples = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f };
        audio_buffer.Reset(2, 44100); // 2 channels, 44100Hz sample rate
        audio_buffer.AddSamples(samples);
        // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
        bytes_per_frame = audio_buffer.GetBytesPerFrame();

        // Set up the reference buffer with the same data for comparison
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        const char* byteData = reinterpret_cast<const char*>(samples.data());
        ref.setData(byteData, static_cast<int>(samples.size() * sizeof(float)));
    }
};

} // namespace

TEST_CASE("AudioBufferQIODevice::open", "[audio_buffer_qiodevice]")
{
    AudioBufferQIODeviceTestFixture fixture;

    SECTION("Open in read-only mode succeeds")
    {
        REQUIRE(fixture.dev.open(QIODevice::ReadOnly));
    }

    SECTION("Open in write-only mode fails")
    {
        REQUIRE_FALSE(fixture.dev.open(QIODevice::WriteOnly));
    }
}

TEST_CASE("AudioBufferQIODevice::readData", "[audio_buffer_qiodevice]")
{
    AudioBufferQIODeviceTestFixture fixture;
    REQUIRE(fixture.dev.open(QIODevice::ReadOnly));
    REQUIRE(fixture.ref.open(QIODevice::ReadOnly));

    SECTION("Read 0 frames")
    {
        const QByteArray kGot = fixture.dev.read(0);
        const QByteArray kRef = fixture.ref.read(0);

        REQUIRE(kGot.isEmpty());
        REQUIRE(kRef.isEmpty());
    }

    SECTION("Read 2 frames")
    {
        const QByteArray kGot = fixture.dev.read(2LL * fixture.bytes_per_frame);
        const QByteArray kRef = fixture.ref.read(2LL * fixture.bytes_per_frame);

        const std::vector<float> kWant = { 0.1f, 0.2f, 0.3f, 0.4f };

        REQUIRE_THAT(QByteArrayToSpanFloat(kGot), RangeEquals(kWant));
        REQUIRE_THAT(QByteArrayToSpanFloat(kRef), RangeEquals(kWant));
    }

    SECTION("Read all frames")
    {
        const QByteArray kGot = fixture.dev.read(3LL * fixture.bytes_per_frame);
        const QByteArray kRef = fixture.ref.read(3LL * fixture.bytes_per_frame);

        const std::vector<float> kWant = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f };

        REQUIRE_THAT(QByteArrayToSpanFloat(kGot), RangeEquals(kWant));
        REQUIRE_THAT(QByteArrayToSpanFloat(kRef), RangeEquals(kWant));
    }

    SECTION("Read more frames than available")
    {
        // Request 5 frames, but only 3 are available
        const QByteArray kGot = fixture.dev.read(5LL * fixture.bytes_per_frame);
        const QByteArray kRef = fixture.ref.read(5LL * fixture.bytes_per_frame);

        const std::vector<float> kWant = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f };

        REQUIRE_THAT(QByteArrayToSpanFloat(kGot), RangeEquals(kWant));
        REQUIRE_THAT(QByteArrayToSpanFloat(kRef), RangeEquals(kWant));
    }

    SECTION("Read with non-multiple of frame size")
    {
        // Request 10 bytes = 1 frame + 2 extra bytes.  The extra bytes should
        // be ignored and only 1 frame should be read.
        const QByteArray kGot = fixture.dev.read(fixture.bytes_per_frame + 2);
        const QByteArray kRef = fixture.ref.read(fixture.bytes_per_frame + 2);

        const std::vector<float> kWant = { 0.1f, 0.2f };

        REQUIRE_THAT(QByteArrayToSpanFloat(kGot), RangeEquals(kWant));
        REQUIRE_THAT(QByteArrayToSpanFloat(kRef), RangeEquals(kWant));
    }

    SECTION("Read progressively")
    {
        const qint64 kBytesToRead = 2LL * fixture.bytes_per_frame;

        // Read the first batch of 2 frames
        const QByteArray kFirstGot = fixture.dev.read(kBytesToRead);
        const QByteArray kFirstRef = fixture.ref.read(kBytesToRead);

        const std::vector<float> kFirstWant = { 0.1f, 0.2f, 0.3f, 0.4f };

        REQUIRE_THAT(QByteArrayToSpanFloat(kFirstGot), RangeEquals(kFirstWant));
        REQUIRE_THAT(QByteArrayToSpanFloat(kFirstRef), RangeEquals(kFirstWant));

        // Read the second batch of 2 frames.  Only 1 frame is available, so it
        // should read that and then stop.
        const QByteArray kSecondGot = fixture.dev.read(kBytesToRead);
        const QByteArray kSecondRef = fixture.ref.read(kBytesToRead);

        const std::vector<float> kSecondWant = { 0.5f, 0.6f };

        REQUIRE_THAT(QByteArrayToSpanFloat(kSecondGot), RangeEquals(kSecondWant));
        REQUIRE_THAT(QByteArrayToSpanFloat(kSecondRef), RangeEquals(kSecondWant));
    }

    SECTION("Read from empty buffer")
    {
        fixture.audio_buffer.Reset(2, 44100); // Clear the buffer

        // Clear the reference buffer as well
        fixture.ref.close();
        fixture.ref.setData("", 0);
        fixture.ref.open(QIODevice::ReadOnly);

        const QByteArray kGot = fixture.dev.read(fixture.bytes_per_frame);
        const QByteArray kRef = fixture.ref.read(fixture.bytes_per_frame);

        REQUIRE(kGot.isEmpty());
        REQUIRE(kRef.isEmpty());
    }

    SECTION("Read after close/reopen")
    {
        const QByteArray kFirstGot = fixture.dev.read(1LL * fixture.bytes_per_frame);
        const QByteArray kFirstRef = fixture.ref.read(1LL * fixture.bytes_per_frame);

        const std::vector<float> kFirstWant = { 0.1f, 0.2f };

        REQUIRE_THAT(QByteArrayToSpanFloat(kFirstGot), RangeEquals(kFirstWant));
        REQUIRE_THAT(QByteArrayToSpanFloat(kFirstRef), RangeEquals(kFirstWant));

        fixture.dev.close();
        fixture.ref.close();

        REQUIRE(fixture.dev.open(QIODevice::ReadOnly));
        REQUIRE(fixture.ref.open(QIODevice::ReadOnly));

        const std::vector<float> kSecondWant = { 0.1f, 0.2f };

        const QByteArray kSecondGot = fixture.dev.read(1LL * fixture.bytes_per_frame);
        const QByteArray kSecondRef = fixture.ref.read(1LL * fixture.bytes_per_frame);

        REQUIRE_THAT(QByteArrayToSpanFloat(kSecondGot), RangeEquals(kSecondWant));
        REQUIRE_THAT(QByteArrayToSpanFloat(kSecondRef), RangeEquals(kSecondWant));
    }
}

TEST_CASE("AudioBufferQIODevice::SeekFrame", "[audio_buffer_qiodevice]")
{
    AudioBufferQIODeviceTestFixture fixture;
    REQUIRE(fixture.dev.open(QIODevice::ReadOnly));

    SECTION("Seek when closed fails")
    {
        fixture.dev.close();
        REQUIRE_FALSE(fixture.dev.SeekFrame(FrameIndex{ 0 }));
    }

    SECTION("Seek before open fails")
    {
        AudioBufferQIODeviceTestFixture newFixture;
        REQUIRE_FALSE(newFixture.dev.SeekFrame(FrameIndex{ 0 }));
    }

    SECTION("Seek beyond end of buffer fails")
    {
        // There are 3 frames, so seeking to the 4th frame should fail
        REQUIRE_FALSE(fixture.dev.SeekFrame(FrameIndex{ 4 }));
    }

    SECTION("Seek to start")
    {
        // Seek to the beginning
        REQUIRE(fixture.dev.SeekFrame(FrameIndex{ 0 }));

        const QByteArray kGot = fixture.dev.read(2LL * fixture.bytes_per_frame);
        const std::vector<float> kWant = { 0.1f, 0.2f, 0.3f, 0.4f };
        REQUIRE_THAT(QByteArrayToSpanFloat(kGot), RangeEquals(kWant));
    }

    SECTION("Seek to middle")
    {
        // Seek to the second frame
        REQUIRE(fixture.dev.SeekFrame(FrameIndex{ 1 }));

        const QByteArray kGot = fixture.dev.read(2LL * fixture.bytes_per_frame);
        const std::vector<float> kWant = { 0.3f, 0.4f, 0.5f, 0.6f };
        REQUIRE_THAT(QByteArrayToSpanFloat(kGot), RangeEquals(kWant));
    }

    SECTION("Seek to end")
    {
        // Seek to the third/last frame
        REQUIRE(fixture.dev.SeekFrame(FrameIndex{ 3 }));

        // We should get no data when we read past the end.
        const QByteArray kGot = fixture.dev.read(3LL * fixture.bytes_per_frame);
        const std::vector<float> kWant = {};
        REQUIRE_THAT(QByteArrayToSpanFloat(kGot), RangeEquals(kWant));
    }

    SECTION("Seek after reading some data")
    {
        // Read the first frame
        const QByteArray kFirstHave = fixture.dev.read(1LL * fixture.bytes_per_frame);
        const std::vector<float> kFirstWant = { 0.1f, 0.2f };
        REQUIRE_THAT(QByteArrayToSpanFloat(kFirstHave), RangeEquals(kFirstWant));

        // Now seek back to the beginning and read again
        REQUIRE(fixture.dev.SeekFrame(FrameIndex{ 0 }));

        const QByteArray kSecondHave = fixture.dev.read(1LL * fixture.bytes_per_frame);
        const std::vector<float> kSecondWant = { 0.1f, 0.2f };
        REQUIRE_THAT(QByteArrayToSpanFloat(kSecondHave), RangeEquals(kSecondWant));
    }
}

TEST_CASE("AudioBufferQIODevice::seek", "[audio_buffer_qiodevice]")
{
    AudioBufferQIODeviceTestFixture fixture;
    REQUIRE(fixture.dev.open(QIODevice::ReadOnly));

    SECTION("Seek to start succeeds")
    {
        REQUIRE(fixture.dev.seek(0));

        const QByteArray kGot = fixture.dev.read(2LL * fixture.bytes_per_frame);
        const std::vector<float> kWant = { 0.1f, 0.2f, 0.3f, 0.4f };
        REQUIRE_THAT(QByteArrayToSpanFloat(kGot), RangeEquals(kWant));
    }

    SECTION("Seek to middle fails")
    {
        REQUIRE_THROWS_WITH(fixture.dev.seek(1LL * fixture.bytes_per_frame),
                            "AudioBufferQIODevice only supports seeking to the beginning of the "
                            "stream.  Use SeekFrame() to seek to specific frames.");
    }
}

TEST_CASE("AudioBufferQIODevice::writeData", "[audio_buffer_qiodevice]")
{
    class TestableAudioBufferQIODevice : public AudioBufferQIODevice
    {
      public:
        using AudioBufferQIODevice::AudioBufferQIODevice;
        qint64 WriteDataWrapper(const char* aData, qint64 aMaxSize)
        {
            return writeData(aData, aMaxSize);
        }
    };

    AudioBuffer buffer;
    TestableAudioBufferQIODevice dev(buffer);
    REQUIRE(dev.WriteDataWrapper("meh", 3) == -1);
}