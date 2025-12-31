#pragma once

#include <audio_types.h>
#include <cstdint>

using ChannelCount = uint8_t;

// Maximum number of audio channels supported by the application
inline constexpr ChannelCount GKMaxChannels = 6;