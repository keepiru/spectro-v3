// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once

#include "controllers/spectrogram_controller.h"
#include "models/audio_buffer.h"
#include "models/settings.h"
#include <mock_fft_processor.h>

/// @brief Base fixture for tests that need a SpectrogramController
/// This reduces boilerplate when testing the views that depend on
/// SpectrogramController.
struct SpectrogramControllerTestFixture
{
    Settings settings;
    AudioBuffer audio_buffer;
    SpectrogramController controller{ settings, audio_buffer, MockFFTProcessor::GetFactory() };
};