// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once

#include "adapters/audio_sink.h"
#include <QIODevice>
#include <memory>

/// @brief Stub implementation of IAudioSink for testing
class StubAudioSink : public IAudioSink
{
  public:
    void Start(std::unique_ptr<QIODevice> /*aSourceQIODevice*/) noexcept override {}
    void Stop() noexcept override {}
};
