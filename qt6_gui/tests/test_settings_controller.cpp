// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "controllers/settings_controller.h"
#include "include/global_constants.h"
#include "models/settings.h"
#include <QAudioFormat>
#include <QByteArray>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("SettingsController constructor", "[settings_controller]")
{
    Settings settings;
    const SettingsController controller(settings);
}

TEST_CASE("SettingsController GetAudioDevices returns device list", "[settings_controller]")
{
    Settings settings;
    const SettingsController controller(settings);

    const auto deviceList = controller.GetAudioDevices();

    // Should return a valid device list (even if empty on systems without audio hardware)
    REQUIRE(deviceList.size() >= 0);

    // If there are devices, check that descriptions are not empty
    for (const auto& device : deviceList) {
        REQUIRE(!device.description().isEmpty());
        REQUIRE(!device.id().isEmpty());
    }

    // Check default device
    const auto defaultDevice = controller.GetDefaultAudioInput();
    if (!defaultDevice.isNull()) {
        REQUIRE(!defaultDevice.id().isEmpty());
    }
}

TEST_CASE("SettingsController GetSupportedSampleRates with invalid device returns empty",
          "[settings_controller]")
{
    Settings settings;
    const SettingsController controller(settings);

    const QByteArray invalidDeviceId("invalid-device-id-12345");
    const auto rates = controller.GetSupportedSampleRates(invalidDeviceId);

    REQUIRE(!rates.has_value());
}

TEST_CASE("SettingsController GetSupportedSampleRates with valid device returns rates",
          "[settings_controller]")
{
    Settings settings;
    const SettingsController controller(settings);

    const auto deviceList = controller.GetAudioDevices();

    // Skip test if no devices available
    if (deviceList.isEmpty()) {
        SKIP("No audio devices available on this system");
    }

    const auto& firstDevice = deviceList.first();
    const auto rates = controller.GetSupportedSampleRates(firstDevice.id());

    REQUIRE(rates.has_value());
    // Should return some rates (could be empty if device doesn't support Float format)
    // But if it does, they should be from the common rates list
    for (const auto rate : rates.value()) {
        const bool isCommonRate =
          (rate == 8000 || rate == 11025 || rate == 16000 || rate == 22050 || rate == 44100 ||
           rate == 48000 || rate == 88200 || rate == 96000);
        REQUIRE(isCommonRate);
    }
}

TEST_CASE("SettingsController GetSupportedChannels with invalid device returns empty",
          "[settings_controller]")
{
    Settings settings;
    const SettingsController controller(settings);

    const QByteArray invalidDeviceId("invalid-device-id-12345");
    const auto channels = controller.GetSupportedChannels(invalidDeviceId);

    REQUIRE(!channels.has_value());
}

TEST_CASE("SettingsController GetSupportedChannels with valid device returns channels",
          "[settings_controller]")
{
    Settings settings;
    const SettingsController controller(settings);

    const auto deviceList = controller.GetAudioDevices();

    // Skip test if no devices available
    if (deviceList.isEmpty()) {
        SKIP("No audio devices available on this system");
    }

    const auto& firstDevice = deviceList.first();
    const auto channels = controller.GetSupportedChannels(firstDevice.id());

    REQUIRE(channels.has_value());
    // Should return some channels (could be empty if device doesn't support Float format)
    // But if it does, they should be <= GKMaxChannels
    for (const auto channel : channels.value()) {
        REQUIRE(channel > 0);
        REQUIRE(channel <= static_cast<int>(GKMaxChannels));
    }
}

TEST_CASE("SettingsController GetSupportedChannels clamps to GKMaxChannels",
          "[settings_controller]")
{
    Settings settings;
    const SettingsController controller(settings);

    const auto deviceList = controller.GetAudioDevices();

    // Skip test if no devices available
    if (deviceList.isEmpty()) {
        SKIP("No audio devices available on this system");
    }

    for (const auto& device : deviceList) {
        const auto channels = controller.GetSupportedChannels(device.id());

        REQUIRE(channels.has_value());
        // All returned channels must be <= GKMaxChannels
        for (const auto channel : channels.value()) {
            REQUIRE(channel <= static_cast<int>(GKMaxChannels));
        }
    }
}

TEST_CASE("SettingsController FindAudioDeviceById with invalid ID returns nullopt",
          "[settings_controller]")
{
    Settings settings;
    const SettingsController controller(settings);

    const QByteArray invalidDeviceId("invalid-device-id-12345");
    const auto deviceOpt = controller.GetAudioDeviceById(invalidDeviceId);

    REQUIRE(!deviceOpt.has_value());
}

TEST_CASE("SettingsController FindAudioDeviceById with valid ID returns device",
          "[settings_controller]")
{
    Settings settings;
    const SettingsController controller(settings);

    const auto deviceList = controller.GetAudioDevices();

    // Skip test if no devices available
    if (deviceList.isEmpty()) {
        SKIP("No audio devices available on this system");
    }

    const auto& firstDevice = deviceList.first();
    const auto deviceOpt = controller.GetAudioDeviceById(firstDevice.id());

    REQUIRE(deviceOpt.has_value());
    REQUIRE(deviceOpt.value().id() == firstDevice.id());
    REQUIRE(deviceOpt.value().description() == firstDevice.description());
}

TEST_CASE("SettingsController uses isFormatSupported for validation", "[settings_controller]")
{
    Settings settings;
    const SettingsController controller(settings);

    const auto deviceList = controller.GetAudioDevices();

    // Skip test if no devices available
    if (deviceList.isEmpty()) {
        SKIP("No audio devices available on this system");
    }

    const auto& firstDevice = deviceList.first();

    // Get supported rates and channels
    const auto rates = controller.GetSupportedSampleRates(firstDevice.id());
    const auto channels = controller.GetSupportedChannels(firstDevice.id());

    REQUIRE(rates.has_value());
    REQUIRE(channels.has_value());

    // Manually verify that returned combinations are actually supported
    for (const auto rate : rates.value()) {
        QAudioFormat format;
        format.setSampleRate(rate);
        format.setChannelCount(2); // Controller tests with 2 channels
        format.setSampleFormat(QAudioFormat::Float);

        // The controller should only return rates that pass isFormatSupported
        REQUIRE(firstDevice.isFormatSupported(format));
    }

    for (const auto channel : channels.value()) {
        QAudioFormat format;
        format.setSampleRate(44100); // Controller tests with 44100 Hz
        format.setChannelCount(channel);
        format.setSampleFormat(QAudioFormat::Float);

        // The controller should only return channels that pass isFormatSupported
        REQUIRE(firstDevice.isFormatSupported(format));
    }
}
