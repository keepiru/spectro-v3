// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "audio_types.h"
#include "controllers/settings_controller.h"
#include "mock_media_devices.h"
#include "models/settings.h"
#include <QAudioFormat>
#include <catch2/catch_test_macros.hpp>
#include <vector>

// NOLINTBEGIN(misc-const-correctness) // False positives with non-const members of the fixture
// NOLINTBEGIN(bugprone-unchecked-optional-access) // It doesn't understand REQUIRE before access

struct SettingsControllerFixture
{
    Settings settings;
    MockMediaDevices provider;
    SettingsController controller{ settings, provider };
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
        fixture.provider.AddDevice(
          MockAudioDevice("device-1", "Test Device", [](const QAudioFormat&) { return false; }));

        const auto ratesOpt = fixture.controller.GetSupportedSampleRates("device-1");
        REQUIRE(ratesOpt.has_value());
        REQUIRE(ratesOpt.value().empty());
    }

    SECTION("only returns supported sample rates")
    {
        fixture.provider.AddDevice(
          MockAudioDevice("device-1", "Test Device", [](const QAudioFormat& format) {
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
        fixture.provider.AddDevice(
          MockAudioDevice("device-1", "Test Device", [](const QAudioFormat&) { return false; }));

        const auto channelsOpt = fixture.controller.GetSupportedChannels("device-1");

        REQUIRE(channelsOpt.has_value());
        REQUIRE(channelsOpt.value().empty());
    }

    SECTION("only returns supported channel counts")
    {
        fixture.provider.AddDevice(
          MockAudioDevice("device-1", "Test Device", [](const QAudioFormat& format) {
              return format.channelCount() == 1 || format.channelCount() == 4;
          }));

        const auto channelsOpt = fixture.controller.GetSupportedChannels("device-1");

        REQUIRE(channelsOpt.has_value());
        REQUIRE(channelsOpt.value() == std::vector<ChannelCount>{ 1, 4 });
    }
}

TEST_CASE("SettingsController::GetAudioDeviceById", "[settings_controller]")
{
    SettingsControllerFixture fixture;
    fixture.provider.AddDevice(MockAudioDevice("device-1", "Test Microphone"));
    fixture.provider.AddDevice(MockAudioDevice("device-2", "USB Audio Interface"));

    SECTION("returns nullopt for invalid device ID")
    {
        const auto deviceOpt = fixture.controller.GetAudioDeviceById("invalid-device");
        REQUIRE(!deviceOpt.has_value());
    }

    SECTION("returns device for valid ID")
    {
        const auto deviceOpt = fixture.controller.GetAudioDeviceById("device-2");

        REQUIRE(deviceOpt.has_value());
        REQUIRE(deviceOpt.value()->Id() == "device-2");
        REQUIRE(deviceOpt.value()->Description() == "USB Audio Interface");
    }
}

// NOLINTEND(bugprone-unchecked-optional-access)
// NOLINTEND(misc-const-correctness)