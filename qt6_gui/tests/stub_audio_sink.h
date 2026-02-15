// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once

#include "adapters/audio_sink.h"
#include "controllers/audio_player.h"
#include <QIODevice>
#include <memory>

/// @brief Stub implementation of IAudioSink for testing
class StubAudioSink : public IAudioSink
{
  public:
    void Start(std::unique_ptr<QIODevice> /*aSourceQIODevice*/) override {}
    void Stop() override {}
    [[nodiscard]] qint64 ProcessedUSecs() const override { return mProcessedUSecs; }

    /// @brief Set the value returned by ProcessedUSecs() for testing
    void SetProcessedUSecs(qint64 aUSecs) { mProcessedUSecs = aUSecs; }

    /// @brief Get a factory function for creating instances of StubAudioSink
    /// @return A factory function that creates a new StubAudioSink instance.
    static AudioPlayer::AudioSinkFactory GetFactory()
    {
        return []() { return std::make_unique<StubAudioSink>(); };
    }

  private:
    qint64 mProcessedUSecs{ 0 };
};
