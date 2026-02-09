// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "audio_types.h"
#include "controllers/audio_recorder.h"
#include "controllers/settings_controller.h"
#include "mock_media_devices.h"
#include "models/audio_buffer.h"
#include "models/settings.h"
#include <QAudioFormat>
#include <QSignalSpy>
#include <catch2/catch_test_macros.hpp>
#include <vector>

// NOLINTBEGIN(misc-const-correctness) // False positives with non-const members of the fixture
// NOLINTBEGIN(bugprone-unchecked-optional-access) // It doesn't understand REQUIRE before access

struct SettingsControllerFixture
{
    Settings settings;
    MockMediaDevices provider;
    AudioBuffer audio_buffer;
    AudioRecorder recorder{ audio_buffer };
    SettingsController controller{ settings, provider, recorder };
};

TEST_CASE("SettingsController constructor", "[settings_controller]")
{
    const SettingsControllerFixture fixture;
}

TEST_CASE("SettingsController::GetAudioDevices", "[settings_controller]")
{
    SettingsControllerFixture fixture;

    SECTION("returns empty list when provider has no devices")
    {
        REQUIRE(fixture.controller.GetAudioInputs().empty());
    }

    SECTION("returns list of devices from provider")
    {
        fixture.provider.AddDevice(MockAudioDevice("device-1", "Test Microphone"));
        fixture.provider.AddDevice(MockAudioDevice("device-2", "USB Audio Interface"));
        const auto deviceList = fixture.controller.GetAudioInputs();

        REQUIRE(deviceList.size() == 2);
        REQUIRE(deviceList[0]->Id() == "device-1");
        REQUIRE(deviceList[0]->Description() == "Test Microphone");
        REQUIRE(deviceList[1]->Id() == "device-2");
        REQUIRE(deviceList[1]->Description() == "USB Audio Interface");
    }
}

TEST_CASE("SettingsController::GetDefaultAudioInput returns provider default",
          "[settings_controller]")
{
    const SettingsControllerFixture fixture;
    REQUIRE(fixture.controller.GetDefaultAudioInput()->Id() == "mock-device");
}

TEST_CASE("SettingsController::GetSupportedSampleRates", "[settings_controller]")
{
    SettingsControllerFixture fixture;

    SECTION("returns nullopt for invalid device ID")
    {
        const auto rates = fixture.controller.GetSupportedSampleRates("nonexistent-device");
        REQUIRE(!rates.has_value());
    }

    SECTION("returns empty list when no formats supported")
    {
        fixture.provider.AddDevice(MockAudioDevice(
          "device-1", "Test Device", [](const QAudioFormat&) noexcept { return false; }));

        const auto ratesOpt = fixture.controller.GetSupportedSampleRates("device-1");
        REQUIRE(ratesOpt.has_value());
        REQUIRE(ratesOpt.value().empty());
    }

    SECTION("only returns supported sample rates")
    {
        fixture.provider.AddDevice(
          MockAudioDevice("device-1", "Test Device", [](const QAudioFormat& format) noexcept {
              return format.sampleRate() == 44100 || format.sampleRate() == 48000;
          }));

        const auto ratesOpt = fixture.controller.GetSupportedSampleRates("device-1");

        REQUIRE(ratesOpt.has_value());
        REQUIRE(ratesOpt.value() == std::vector<SampleRate>{ 44100, 48000 });
    }
}

TEST_CASE("SettingsController::GetSupportedChannels", "[settings_controller]")
{
    SettingsControllerFixture fixture;

    SECTION("returns nullopt for invalid device ID")
    {
        const auto channels = fixture.controller.GetSupportedChannels("nonexistent-device");
        REQUIRE(!channels.has_value());
    }

    SECTION("returns empty list when no channel counts supported")
    {
        fixture.provider.AddDevice(MockAudioDevice(
          "device-1", "Test Device", [](const QAudioFormat&) noexcept { return false; }));

        const auto channelsOpt = fixture.controller.GetSupportedChannels("device-1");

        REQUIRE(channelsOpt.has_value());
        REQUIRE(channelsOpt.value().empty());
    }

    SECTION("only returns supported channel counts")
    {
        fixture.provider.AddDevice(
          MockAudioDevice("device-1", "Test Device", [](const QAudioFormat& format) noexcept {
              return format.channelCount() == 1 || format.channelCount() == 4;
          }));

        const auto channelsOpt = fixture.controller.GetSupportedChannels("device-1");

        REQUIRE(channelsOpt.has_value());
        REQUIRE(channelsOpt.value() == std::vector<ChannelCount>{ 1, 4 });
    }
}

TEST_CASE("SettingsController::GetAudioInputById", "[settings_controller]")
{
    SettingsControllerFixture fixture;
    fixture.provider.AddDevice(MockAudioDevice("device-1", "Test Microphone"));
    fixture.provider.AddDevice(MockAudioDevice("device-2", "USB Audio Interface"));

    SECTION("returns nullopt for invalid device ID")
    {
        const auto deviceOpt = fixture.controller.GetAudioInputById("invalid-device");
        REQUIRE(!deviceOpt.has_value());
    }

    SECTION("returns device for valid ID")
    {
        const auto deviceOpt = fixture.controller.GetAudioInputById("device-2");

        REQUIRE(deviceOpt.has_value());
        REQUIRE(deviceOpt.value()->Id() == "device-2");
        REQUIRE(deviceOpt.value()->Description() == "USB Audio Interface");
    }
}

TEST_CASE("SettingsController::IsRecording returns false initially", "[settings_controller]")
{
    const SettingsControllerFixture fixture;
    REQUIRE(!fixture.controller.IsRecording());
}

TEST_CASE("SettingsController::StartRecording", "[settings_controller]")
{
    SettingsControllerFixture fixture;
    fixture.provider.AddDevice(MockAudioDevice("device-1", "Test Microphone"));

    SECTION("returns false for invalid device ID")
    {
        REQUIRE(!fixture.controller.StartRecording("nonexistent-device", 2, 44100));
        REQUIRE(!fixture.controller.IsRecording());
    }

    SECTION("starts recording and emits channel count signal")
    {
        QSignalSpy channelSpy(&fixture.audio_buffer, &AudioBuffer::BufferReset);

        REQUIRE(fixture.controller.StartRecording("device-1", 2, 44100));
        REQUIRE(fixture.controller.IsRecording());

        REQUIRE(channelSpy.count() == 1);
        REQUIRE(channelSpy.takeFirst().at(0).toInt() == 2);
    }
}

TEST_CASE("SettingsController::StopRecording", "[settings_controller]")
{
    SettingsControllerFixture fixture;
    fixture.provider.AddDevice(MockAudioDevice("device-1", "Test Microphone"));

    SECTION("stops recording after start")
    {
        const bool kRecordingStarted = fixture.controller.StartRecording("device-1", 2, 44100);
        REQUIRE(kRecordingStarted);
        REQUIRE(fixture.controller.IsRecording());

        fixture.controller.StopRecording();
        REQUIRE(!fixture.controller.IsRecording());
    }

    SECTION("is no-op when not recording")
    {
        fixture.controller.StopRecording(); // Should not crash
        REQUIRE(!fixture.controller.IsRecording());
    }
}

// NOLINTEND(bugprone-unchecked-optional-access)
// NOLINTEND(misc-const-correctness)