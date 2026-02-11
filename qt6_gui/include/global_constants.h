// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once

#include <audio_types.h>
#include <cstdint>

// Maximum number of audio channels supported by the application
inline constexpr ChannelCount GKMaxChannels = 6;

/// Window scale factor for FFT overlap (1, 2, 4, 8, or 16)
/// uint8_t is sufficient for the small range of valid values.
using WindowScale = uint8_t;

/// Number of bytes per audio frame (samples for all channels)
using BytesPerFrame = uint32_t;