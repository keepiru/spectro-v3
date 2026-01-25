// Spectro-v3 -- Real-time spectrum analyzer
// SPDX-License-Identifier: GPL-3.0-only
// Copyright (C) 2025-2026 Chris "Kai" Frederick

#pragma once
#include "models/audio_buffer.h"
#include "models/settings.h"
#include <QObject>
#include <audio_types.h>
#include <fft_processor.h>
#include <fft_window.h>
#include <memory>
#include <vector>

/// @brief Controller for spectrogram data flow and view state
///
/// Coordinates the data flow between AudioBuffer (model) and SpectrogramView (view).
/// Owns FFT processing components (FFTProcessor, FFTWindow) per channel.
/// Manages view state including live/historical mode and scroll position.
class SpectrogramController : public QObject
{
    Q_OBJECT

  public:
    static constexpr FFTSize KDefaultFftSize = 2048;
    static constexpr auto KDefaultWindowType = FFTWindow::Type::Hann;

    /// @brief Constructor
    /// @param aSettings Reference to application settings model
    /// @param aAudioBuffer Reference to the audio buffer model
    /// @param aFFTProcessorFactory Factory function to create FFTProcessor instances (optional)
    /// @param aFFTWindowFactory Factory function to create FFTWindow instances (optional)
    /// @param aParent Qt parent object (optional)
    ///
    /// The factories are used for dependency injection in tests.  By default,
    /// FFTProcessor and FFTWindow instances are created, which is appropriate
    /// for production use.
    SpectrogramController(const Settings& aSettings,
                          const AudioBuffer& aAudioBuffer,
                          IFFTProcessor::Factory aFFTProcessorFactory = nullptr,
                          FFTWindowFactory aFFTWindowFactory = nullptr,
                          QObject* aParent = nullptr);

    /// @brief Get spectrogram rows for a channel
    /// @param aChannel Channel index (0-based)
    /// @param aFirstFrame First frame position (aligned to stride)
    /// @param aRowCount Number of rows to compute
    /// @return 2D vector [aRowCount][frequency_bins] containing frequency magnitudes
    /// @throws std::out_of_range if aChannel is invalid
    ///
    /// Each row represents one time window in the spectrogram.
    [[nodiscard]] std::vector<std::vector<float>> GetRows(ChannelCount aChannel,
                                                          FramePosition aFirstFrame,
                                                          size_t aRowCount) const;

    /// @brief Get a single spectrogram row for a channel
    /// @param aChannel Channel index (0-based)
    /// @param aFirstFrame First frame position (aligned to stride)
    /// @return Vector of frequency magnitudes for the specified row
    /// @throws std::out_of_range if aChannel is invalid
    /// @note Uses internal caching to avoid redundant computations
    /// @note If ANY samples in the requested window are not available, returns a
    /// vector of zeros.
    [[nodiscard]] std::vector<float> GetRow(ChannelCount aChannel, FramePosition aFirstFrame) const;

    /// @brief Compute FFT for a channel at a specific frame position
    /// @param aChannel Channel index (0-based)
    /// @param aFirstFrame First frame position (aligned to stride)
    /// @return Vector of frequency magnitudes
    /// @throws std::out_of_range if aChannel is invalid
    /// @throws std::out_of_range if requested samples are not available
    /// @note Does not use caching; called internally by GetRow
    [[nodiscard]] std::vector<float> ComputeFFT(ChannelCount aChannel,
                                                FrameIndex aFirstFrame) const;

    /// @brief Get the number of available frames
    /// @return Number of frames currently available in the audio buffer
    [[nodiscard]] FrameCount GetAvailableFrameCount() const;

    /// @brief Get the number of available channels
    /// @return Number of audio channels
    [[nodiscard]] ChannelCount GetChannelCount() const;

    /// @brief Reset FFT processing components
    ///
    /// Clears spectrogram cache, recreates FFTProcessor and FFTWindow for each
    /// channel.  Called when settings or audio buffer change.  Also used during
    /// initialization.
    void ResetFFT();

    /// @brief Get reference to application settings
    /// @return Reference to Settings instance
    [[nodiscard]] const Settings& GetSettings() const { return mSettings; }

    /// @brief Calculate the first frame in the stride-aligned window containing aCursorFrame
    /// @param aCursorFrame Cursor frame index
    /// @return First frame index of the stride-aligned window containing aCursorFrame
    /// @note Returns a negative value if aCursorFrame is less than one transform window
    [[nodiscard]] FramePosition CalculateTopOfWindow(FramePosition aCursorFrame) const;

    /// @brief round a frame index down to nearest window stride
    /// @param aFrame Frame index
    /// @return Frame index rounded down to nearest window stride
    [[nodiscard]] FramePosition RoundToStride(FramePosition aFrame) const;

    /// @brief Get frequency resolution in Hz per FFT bin
    /// @return Frequency resolution in Hz
    [[nodiscard]] float GetHzPerBin() const;

  private:
    const Settings& mSettings;       // Reference to application settings model
    const AudioBuffer& mAudioBuffer; // Reference to audio buffer model

    // FFT processing components (per channel)
    std::vector<std::unique_ptr<IFFTProcessor>> mFFTProcessors;
    std::vector<std::unique_ptr<FFTWindow>> mFFTWindows;

    IFFTProcessor::Factory mFFTProcessorFactory;
    FFTWindowFactory mFFTWindowFactory;

    // Spectrogram row cache.  Key: (channel, first frame).  Stores a single row
    // of spectrogram data for reuse.
    mutable std::map<std::pair<ChannelCount, FrameIndex>, std::vector<float>> mSpectrogramRowCache;
};
