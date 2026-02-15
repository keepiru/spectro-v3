// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once

#include "controllers/audio_player.h"
#include "controllers/spectrogram_controller.h"
#include "models/audio_buffer.h"
#include "models/settings.h"
#include "tests/stub_audio_sink.h"
#include <mock_fft_processor.h>

/// @brief Base fixture for tests that need a SpectrogramController
/// This reduces boilerplate when testing the views that depend on
/// SpectrogramController.
struct SpectrogramControllerTestFixture
{
    // We need the default initialization here so we can construct a test
    // fixture with a custom controller that takes additional parameters (e.g.
    // spies) without having to initialize these members in the test itself.
    Settings settings{};        // NOLINT(readability-redundant-member-init)
    AudioBuffer audio_buffer{}; // NOLINT(readability-redundant-member-init)
    AudioPlayer audio_player{ audio_buffer, StubAudioSink::GetFactory() };
    SpectrogramController controller{ settings,
                                      audio_buffer,
                                      audio_player,
                                      MockFFTProcessor::GetFactory() };
};