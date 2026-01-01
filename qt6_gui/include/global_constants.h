#pragma once

#include <audio_types.h>
#include <cstdint>

using ChannelCount = uint8_t;

// Maximum number of audio channels supported by the application
inline constexpr ChannelCount GKMaxChannels = 6;

/// Window scale factor for FFT overlap (1, 2, 4, 8, or 16)
/// uint8_t is sufficient for the small range of valid values.
using WindowScale = uint8_t;