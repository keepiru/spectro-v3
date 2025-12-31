#pragma once

#include <cstdint>

using ChannelCount = uint8_t;
using SampleRate = int;

// Maximum number of audio channels supported by the application
inline constexpr ChannelCount GKMaxChannels = 6;