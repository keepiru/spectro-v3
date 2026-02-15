// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#include "controllers/spectrogram_controller.h"
#include "models/audio_buffer.h"
#include "models/settings.h"
#include "tests/spectrogram_controller_test_fixture.h"
#include <audio_types.h>
#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstddef>
#include <cstdint>
#include <fft_processor.h>
#include <fft_window.h>
#include <format>
#include <memory>
#include <mock_fft_processor.h>
#include <stdexcept>
#include <utility>
#include <vector>

TEST_CASE("SpectrogramController constructor", "[spectrogram_controller]")
{
    const SpectrogramControllerTestFixture fixture;
}

TEST_CASE("SpectrogramController::SetFFTSettings", "[spectrogram_controller]")
{
    std::vector<FFTSize> procCalls;
    const IFFTProcessor::Factory procSpy = [&procCalls](FFTSize size) {
        procCalls.emplace_back(size);
        return std::make_unique<MockFFTProcessor>(size);
    };

    std::vector<std::pair<FFTSize, FFTWindow::Type>> windowCalls;
    const FFTWindowFactory windowSpy = [&windowCalls](FFTSize size, FFTWindow::Type type) {
        windowCalls.emplace_back(size, type);
        return std::make_unique<FFTWindow>(size, type);
    };

    SpectrogramControllerTestFixture fixture{
        .controller = SpectrogramController(
          fixture.settings, fixture.audio_buffer, fixture.audio_player, procSpy, windowSpy)
    };

    // Constructor calls with defaults
    REQUIRE(procCalls == (std::vector<FFTSize>{ SpectrogramController::KDefaultFftSize,
                                                SpectrogramController::KDefaultFftSize }));
    REQUIRE(windowCalls == (std::vector<std::pair<FFTSize, FFTWindow::Type>>{
                             std::make_pair(SpectrogramController::KDefaultFftSize,
                                            SpectrogramController::KDefaultWindowType),
                             std::make_pair(SpectrogramController::KDefaultFftSize,
                                            SpectrogramController::KDefaultWindowType),
                           }));

    fixture.settings.SetFFTSettings(1024, FFTWindow::Type::Rectangular);

    // SetFFTSettings calls again with new settings
    REQUIRE(procCalls == (std::vector<FFTSize>{ SpectrogramController::KDefaultFftSize,
                                                SpectrogramController::KDefaultFftSize,
                                                1024,
                                                1024 }));
    REQUIRE(windowCalls == (std::vector<std::pair<FFTSize, FFTWindow::Type>>{
                             std::make_pair(SpectrogramController::KDefaultFftSize,
                                            SpectrogramController::KDefaultWindowType),
                             std::make_pair(SpectrogramController::KDefaultFftSize,
                                            SpectrogramController::KDefaultWindowType),
                             std::make_pair(1024, FFTWindow::Type::Rectangular),
                             std::make_pair(1024, FFTWindow::Type::Rectangular) }));
}

TEST_CASE("SpectrogramController::GetRows, GetRow, ComputeFFT", "[spectrogram_controller]")
{
    SpectrogramControllerTestFixture fixture;
    fixture.settings.SetFFTSettings(8, FFTWindow::Type::Rectangular);
    fixture.audio_buffer.Reset(1, 44100);
    fixture.audio_buffer.AddSamples({ 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13,
                                      14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26 });

    SECTION("single window computation")
    {
        const std::vector<std::vector<float>> kWant = {
            { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },
        };
        REQUIRE(fixture.controller.GetRows(0, FramePosition{ 0 }, 1) == kWant);
        REQUIRE(fixture.controller.GetRow(0, FramePosition{ 0 }) == kWant[0]);
        REQUIRE(fixture.controller.ComputeFFT(0, FrameIndex{ 0 }) == kWant[0]);
    }

    SECTION("multiple non-overlapping windows")
    {
        fixture.settings.SetWindowScale(1);

        const std::vector<std::vector<float>> kWant = {
            { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },
            { 9.0f, 10.0f, 11.0f, 12.0f, 13.0f },
            { 17.0f, 18.0f, 19.0f, 20.0f, 21.0f },
        };
        const auto kGot = fixture.controller.GetRows(0, FramePosition{ 0 }, 3);
        REQUIRE(kGot == kWant);
    }

    SECTION("50% overlap")
    {
        fixture.settings.SetWindowScale(2); // 50% overlap

        const std::vector<std::vector<float>> kWant = {
            { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },      { 5.0f, 6.0f, 7.0f, 8.0f, 9.0f },
            { 9.0f, 10.0f, 11.0f, 12.0f, 13.0f },  { 13.0f, 14.0f, 15.0f, 16.0f, 17.0f },
            { 17.0f, 18.0f, 19.0f, 20.0f, 21.0f },
        };
        const auto kGot = fixture.controller.GetRows(0, FramePosition{ 0 }, 5);
        REQUIRE(kGot == kWant);
    }

    SECTION("75% overlap")
    {
        fixture.settings.SetWindowScale(4); // 75% overlap

        const std::vector<std::vector<float>> kWant = {
            { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },      { 3.0f, 4.0f, 5.0f, 6.0f, 7.0f },
            { 5.0f, 6.0f, 7.0f, 8.0f, 9.0f },      { 7.0f, 8.0f, 9.0f, 10.0f, 11.0f },
            { 9.0f, 10.0f, 11.0f, 12.0f, 13.0f },  { 11.0f, 12.0f, 13.0f, 14.0f, 15.0f },
            { 13.0f, 14.0f, 15.0f, 16.0f, 17.0f }, { 15.0f, 16.0f, 17.0f, 18.0f, 19.0f },
            { 17.0f, 18.0f, 19.0f, 20.0f, 21.0f }, { 19.0f, 20.0f, 21.0f, 22.0f, 23.0f },
        };
        const auto kGot = fixture.controller.GetRows(0, FramePosition{ 0 }, 10);
        REQUIRE(kGot == kWant);
    }

    SECTION("edge case semantics differ between GetRows, GetRow, and ComputeFFT")
    {
        fixture.settings.SetWindowScale(1);
        fixture.audio_buffer.Reset(1, 44100);
        fixture.audio_buffer.AddSamples({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });

        SECTION("GetRows with start sample beyond buffer end returns zeroed rows")
        {
            const std::vector<std::vector<float>> kWant = {
                { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },
                { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
            };

            REQUIRE(fixture.controller.GetRows(0, FramePosition{ 0 }, 2) == kWant);

            // Do the same thing with GetRow
            REQUIRE(fixture.controller.GetRow(0, FramePosition{ 0 }) == kWant[0]);
            REQUIRE(fixture.controller.GetRow(0, FramePosition{ 8 }) == kWant[1]);

            // And with ComputeFFT
            REQUIRE(fixture.controller.ComputeFFT(0, FrameIndex(0)) == kWant[0]);
            REQUIRE_THROWS_AS((void)fixture.controller.ComputeFFT(0, FrameIndex(8)),
                              std::out_of_range);
        }

        SECTION("GetRows with negative start sample returns zeroed rows")
        {
            const std::vector<std::vector<float>> kWant = {
                { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
                { 7.0f, 8.0f, 9.0f, 10.0f, 11.0f },
            };
            REQUIRE(fixture.controller.GetRows(0, FramePosition{ -2 }, 2) == kWant);

            // Do the same thing with GetRow
            REQUIRE(fixture.controller.GetRow(0, FramePosition{ -2 }) == kWant[0]);
            REQUIRE(fixture.controller.GetRow(0, FramePosition{ 6 }) == kWant[1]);
            // ComputeFFT takes FrameIndex (unsigned), so negative values cannot be passed
            // The validation happens in GetRow which calls ToFrameIndex before ComputeFFT
            REQUIRE(fixture.controller.ComputeFFT(0, FrameIndex{ 6 }) == kWant[1]);
        }
    }

    SECTION("Hann window integration")
    {
        fixture.settings.SetFFTSettings(8, FFTWindow::Type::Hann);
        fixture.settings.SetWindowScale(1);
        fixture.audio_buffer.Reset(1, 44100);
        fixture.audio_buffer.AddSamples({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 });

        // Hann window attenuates edges, so we'll see lower magnitudes at the
        // edges.  Keep in mind our MockFFTProcessor just returns the input
        // samples as magnitudes so the only transformation is from the
        // windowing.
        const std::vector<std::vector<float>> kWant = {
            { 0.0f, 0.146446615f, 0.5f, 0.853553414f, 1.0f },
            { 0.0f, 0.146446615f, 0.5f, 0.853553414f, 1.0f }
        };

        REQUIRE(fixture.controller.GetRows(0, FramePosition{ 0 }, 2) == kWant);
        REQUIRE(fixture.controller.GetRow(0, FramePosition{ 0 }) == kWant[0]);
        REQUIRE(fixture.controller.ComputeFFT(0, FrameIndex{ 0 }) == kWant[0]);
    }

    SECTION("throws on invalid channel")
    {
        REQUIRE_THROWS_AS((void)fixture.controller.GetRows(1, FramePosition{ 0 }, 1),
                          std::out_of_range);
        REQUIRE_THROWS_AS((void)fixture.controller.GetRow(1, FramePosition{ 0 }),
                          std::out_of_range);
        REQUIRE_THROWS_AS((void)fixture.controller.ComputeFFT(1, FrameIndex{ 0 }),
                          std::out_of_range);
    }
}

TEST_CASE("SpectrogramController::GetAvailableFrameCount", "[spectrogram_controller]")
{
    SpectrogramControllerTestFixture fixture;
    fixture.audio_buffer.AddSamples({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 });

    const auto kExpectedSampleCount = 5; // 10 samples / 2 channels
    REQUIRE(fixture.controller.GetAvailableFrameCount() == FrameCount(kExpectedSampleCount));
}

TEST_CASE("SpectrogramController::GetChannelCount", "[spectrogram_controller]")
{
    SpectrogramControllerTestFixture fixture;
    fixture.audio_buffer.Reset(3, 44100);

    REQUIRE(fixture.controller.GetChannelCount() == 3);
}

TEST_CASE("SpectrogramController::CalculateTopOfWindow", "[spectrogram_controller]")
{
    SpectrogramControllerTestFixture fixture;
    fixture.settings.SetFFTSettings(8, FFTWindow::Type::Rectangular);

    auto check = [&fixture](int64_t index, size_t scale, int64_t want) {
        fixture.settings.SetWindowScale(scale);

        const FramePosition have = fixture.controller.CalculateTopOfWindow(FramePosition{ index });
        INFO(std::format("CalculateTopOfWindow: sample={} scale={} => topSample={} (want {})",
                         index,
                         scale,
                         have.Get(),
                         want));
        REQUIRE(have == FramePosition{ want });
    };

    check(6, 1, -8); //[-8 -7 -6 -5 -4 -3 -2 -1] 0  1  2  3  4  5
    check(6, 2, -4); // -8 -7 -6 -5[-4 -3 -2 -1  0  1  2  3] 4  5
    check(6, 4, -2); // -8 -7 -6 -5 -4 -3[-2 -1  0  1  2  3  4  5]
    check(6, 8, -2); // -8 -7 -6 -5 -4 -3[-2 -1  0  1  2  3  4  5]

    check(7, 1, -8); //[-8 -7 -6 -5 -4 -3 -2 -1] 0  1  2  3  4  5  6
    check(7, 2, -4); // -8 -7 -6 -5[-4 -3 -2 -1  0  1  2  3] 4  5  6
    check(7, 4, -2); // -8 -7 -6 -5 -4 -3[-2 -1  0  1  2  3  4  5] 6
    check(7, 8, -1); // -8 -7 -6 -5 -4 -3 -2[-1  0  1  2  3  4  5  6]

    check(8, 1, 0); // -8 -7 -6 -5 -4 -3 -2 -1 [0  1  2  3  4  5  6  7]
    check(8, 2, 0); // -8 -7 -6 -5 -4 -3 -2 -1 [0  1  2  3  4  5  6  7]
    check(8, 4, 0); // -8 -7 -6 -5 -4 -3 -2 -1 [0  1  2  3  4  5  6  7]
    check(8, 8, 0); // -8 -7 -6 -5 -4 -3 -2 -1 [0  1  2  3  4  5  6  7]

    check(12, 1, 0); // [0  1  2  3  4  5  6  7] 8  9  10  11
    check(12, 2, 4); //  0  1  2  3 [4  5  6  7  8  9  10  11]
    check(12, 4, 4); //  0  1  2  3 [4  5  6  7  8  9  10  11]
    check(12, 8, 4); //  0  1  2  3 [4  5  6  7  8  9  10  11]

    check(13, 1, 0); // [0  1  2  3  4  5  6  7] 8  9  10  11  12
    check(13, 2, 4); //  0  1  2  3 [4  5  6  7  8  9  10  11] 12
    check(13, 4, 4); //  0  1  2  3 [4  5  6  7  8  9  10  11] 12
    check(13, 8, 5); //  0  1  2  3  4 [5  6  7  8  9  10  11  12]

    check(14, 1, 0); // [0  1  2  3  4  5  6  7] 8  9  10  11  12  13
    check(14, 2, 4); //  0  1  2  3 [4  5  6  7  8  9  10  11] 12  13
    check(14, 4, 6); //  0  1  2  3  4  5 [6  7  8  9  10  11  12  13]
    check(14, 8, 6); //  0  1  2  3  4  5 [6  7  8  9  10  11  12  13]

    check(15, 1, 0); // [0  1  2  3  4  5  6  7] 8  9  10  11  12  13  14
    check(15, 2, 4); //  0  1  2  3 [4  5  6  7  8  9  10  11] 12  13  14
    check(15, 4, 6); //  0  1  2  3  4  5 [6  7  8  9  10  11  12  13] 14
    check(15, 8, 7); //  0  1  2  3  4  5  6 [7  8  9  10  11  12  13  14]

    check(16, 1, 8); //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15]
    check(16, 2, 8); //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15]
    check(16, 4, 8); //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15]
    check(16, 8, 8); //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15]

    check(17, 1, 8); //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15] 16
    check(17, 2, 8); //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15] 16
    check(17, 4, 8); //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15] 16
    check(17, 8, 9); //  0  1  2  3  4  5  6  7  8 [9  10  11  12  13  14  15  16]

    check(18, 1, 8);  //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15] 16  17
    check(18, 2, 8);  //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15] 16  17
    check(18, 4, 10); //  0  1  2  3  4  5  6  7  8  9 [10  11  12  13  14  15  16  17]
    check(18, 8, 10); //  0  1  2  3  4  5  6  7  8  9 [10  11  12  13  14  15  16  17]
}

TEST_CASE("SpectrogramController::RoundToStride", "[spectrogram_controller]")
{
    SpectrogramControllerTestFixture fixture;
    fixture.settings.SetFFTSettings(8, FFTWindow::Type::Rectangular);

    auto check = [&](size_t stride, int64_t sample, int64_t want) {
        if (fixture.settings.GetFFTSize() % stride != 0) {
            throw std::invalid_argument("Stride must divide FFT size evenly");
        }
        fixture.settings.SetWindowScale(fixture.settings.GetFFTSize() / stride);

        const FramePosition have = fixture.controller.RoundToStride(FramePosition{ sample });
        INFO(std::format(
          "SpectrogramController::RoundToStride: stride={}, sample={}, got={} (want {})",
          stride,
          sample,
          have.Get(),
          want));
        REQUIRE(have == FramePosition{ want });
    };

    check(1, -2, -2);
    check(1, -1, -1);
    check(1, 0, 0);
    check(1, 1, 1);
    check(1, 2, 2);

    check(2, -3, -4);
    check(2, -2, -2);
    check(2, -1, -2);
    check(2, 0, 0);
    check(2, 1, 0);
    check(2, 2, 2);

    check(4, -5, -8);
    check(4, -4, -4);
    check(4, -3, -4);
    check(4, -2, -4);
    check(4, -1, -4);
    check(4, 0, 0);
    check(4, 1, 0);
    check(4, 2, 0);
    check(4, 3, 0);
    check(4, 4, 4);
    check(4, 5, 4);

    check(8, -10, -16);
    check(8, -9, -16);
    check(8, -8, -8);
    check(8, -1, -8);
    check(8, 0, 0);
    check(8, 7, 0);
    check(8, 8, 8);
    check(8, 15, 8);
    check(8, 16, 16);
    check(8, 17, 16);
}

TEST_CASE("SpectrogramController::GetHzPerBin", "[spectrogram_controller]")
{
    using Catch::Matchers::WithinAbs;
    SpectrogramControllerTestFixture fixture;
    fixture.audio_buffer.Reset(2, 48000); // 48 kHz sample rate

    fixture.settings.SetFFTSettings(1024, FFTWindow::Type::Rectangular);
    CHECK_THAT(fixture.controller.GetHzPerBin(), WithinAbs(46.875f, 0.0001f)); // 48000 / 1024

    fixture.settings.SetFFTSettings(2048, FFTWindow::Type::Rectangular);
    CHECK_THAT(fixture.controller.GetHzPerBin(), WithinAbs(23.4375f, 0.0001f)); // 48000 / 2048

    fixture.audio_buffer.Reset(1, 44100); // 44.1 kHz sample rate
    fixture.settings.SetFFTSettings(1024, FFTWindow::Type::Rectangular);
    CHECK_THAT(fixture.controller.GetHzPerBin(), WithinAbs(43.06640625f, 0.0001f)); // 44100 / 1024
}

TEST_CASE("SpectrogramController::GetPlaybackFrame", "[spectrogram_controller]")
{
    // We're just going to do a basic smoke test here.  The AudioPlayer tests
    // cover the actual calculation logic.

    SpectrogramControllerTestFixture fixture;

    SECTION("returns std::nullopt when audio player is not playing")
    {
        CHECK_FALSE(fixture.controller.GetPlaybackFrame());
    }

    SECTION("returns correct frame index based on audio player position")
    {
        fixture.audio_buffer.AddSamples(std::vector<float>(500, 0)); // 250 frames for stereo
        REQUIRE(fixture.audio_player.Start(FrameIndex{ 123 }));
        CHECK(fixture.controller.GetPlaybackFrame() == FrameIndex{ 123 });
    }
}
