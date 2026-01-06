#include "controllers/spectrogram_controller.h"
#include "ifft_processor.h"
#include "include/global_constants.h"
#include "models/audio_buffer.h"
#include "models/settings.h"
#include <QObject>
#include <audio_types.h>
#include <cassert>
#include <cstddef>
#include <fft_processor.h>
#include <fft_window.h>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

SpectrogramController::SpectrogramController(const Settings& aSettings,
                                             const AudioBuffer& aAudioBuffer,
                                             IFFTProcessorFactory aFFTProcessorFactory,
                                             FFTWindowFactory aFFTWindowFactory,
                                             QObject* aParent)
  : QObject(aParent)
  , mSettings(aSettings)
  , mAudioBuffer(aAudioBuffer)
  , mFFTProcessorFactory(std::move(aFFTProcessorFactory))
  , mFFTWindowFactory(std::move(aFFTWindowFactory))
{
    // Provide default factories if none supplied
    if (!mFFTProcessorFactory) {
        mFFTProcessorFactory = [](FFTSize size) { return std::make_unique<FFTProcessor>(size); };
    }

    if (!mFFTWindowFactory) {
        mFFTWindowFactory = [](FFTSize size, FFTWindow::Type type) {
            return std::make_unique<FFTWindow>(size, type);
        };
    }

    // Reset FFT when settings update (such as size or window type)
    connect(&mSettings, &Settings::FFTSettingsChanged, this, &SpectrogramController::ResetFFT);

    // Reset FFT when audio buffer is reset (such as new recording or file load)
    connect(&mAudioBuffer, &AudioBuffer::BufferReset, this, &SpectrogramController::ResetFFT);

    // Initialize with default FFT settings
    ResetFFT();
}

void
SpectrogramController::ResetFFT()
{
    // Clear out the old DSP objects
    mFFTProcessors.clear();
    mFFTWindows.clear();
    mSpectrogramRowCache.clear();

    // Create FFT and window instances for each channel
    for (size_t i = 0; i < mAudioBuffer.GetChannelCount(); i++) {
        auto fftProcessor = mFFTProcessorFactory(mSettings.GetFFTSize());
        auto fftWindow = mFFTWindowFactory(mSettings.GetFFTSize(), mSettings.GetWindowType());
        const auto& sampleBuffer = mAudioBuffer.GetChannelBuffer(i);

        mFFTProcessors.emplace_back(std::move(fftProcessor));
        mFFTWindows.emplace_back(std::move(fftWindow));
    }
}

std::vector<std::vector<float>>
SpectrogramController::GetRows(ChannelCount aChannel,
                               FramePosition aFirstFrame,
                               size_t aRowCount) const
{
    if (aChannel >= mAudioBuffer.GetChannelCount()) {
        throw std::out_of_range("Channel index out of range");
    }

    std::vector<std::vector<float>> spectrogram;
    spectrogram.reserve(aRowCount);

    const FFTSize kWindowStride = mSettings.GetWindowStride();

    for (size_t row = 0; row < aRowCount; row++) {
        const FramePosition kWindowFirstSample =
          aFirstFrame + (static_cast<FramePosition>(row) * kWindowStride);
        const auto kSpectrum = GetRow(aChannel, kWindowFirstSample);
        spectrogram.push_back(kSpectrum);
    }
    return spectrogram;
}

std::vector<float>
SpectrogramController::GetRow(ChannelCount aChannel, FramePosition aFirstFrame) const
{
    if (aChannel >= mAudioBuffer.GetChannelCount()) {
        throw std::out_of_range("Channel index out of range");
    }

    const FFTSize kFFTSize = mFFTWindows.at(aChannel)->GetSize();
    // We want the number of samples in single channel
    const FrameCount kAvailableFrames = GetAvailableFrameCount();
    const FramePosition kLastNeededSample = aFirstFrame + kFFTSize;
    // Check if the requested window is within available data, else return zeroed row
    if (aFirstFrame < 0 || kLastNeededSample > kAvailableFrames.Get()) {
        return std::vector<float>((kFFTSize / 2) + 1, 0.0f);
    }

    // The aFirstFrame < 0 check above ensures this cast is safe
    const FrameIndex kFirstFrameIndex(aFirstFrame);

    // Check cache first
    const std::pair<ChannelCount, FrameIndex> cacheKey = { aChannel, kFirstFrameIndex };

    const auto cacheIt = mSpectrogramRowCache.find(cacheKey);
    if (cacheIt != mSpectrogramRowCache.end()) {
        // Found in cache
        return cacheIt->second;
    }

    // Not in cache, compute it
    const auto kSpectrum = ComputeFFT(aChannel, kFirstFrameIndex);
    // Store in cache
    mSpectrogramRowCache.emplace(cacheKey, kSpectrum);

    return kSpectrum;
}

std::vector<float>
SpectrogramController::ComputeFFT(ChannelCount aChannel, FrameIndex aFirstFrame) const
{
    const FFTSize kFFTSize = mFFTWindows.at(aChannel)->GetSize();
    // We need to convert FrameIndex to SampleIndex for audio buffer access
    const SampleIndex kFirstSample(aFirstFrame.Get());
    // Future performance optimization: grab the entire needed range once
    // before the loop to minimize locking and copy overhead.
    const auto kSamples = mAudioBuffer.GetSamples(aChannel, kFirstSample, SampleCount(kFFTSize));
    auto windowedSamples = mFFTWindows[aChannel]->Apply(kSamples);
    auto windowedSpan = std::span<float>(windowedSamples);
    return mFFTProcessors[aChannel]->ComputeDecibels(windowedSpan);
}

FrameCount
SpectrogramController::GetAvailableFrameCount() const
{
    return mAudioBuffer.GetFrameCount();
}

ChannelCount
SpectrogramController::GetChannelCount() const
{
    return mAudioBuffer.GetChannelCount();
}

FramePosition
SpectrogramController::CalculateTopOfWindow(FramePosition aCursorFrame) const
{
    const FramePosition kUnalignedFrame = aCursorFrame - mSettings.GetFFTSize();
    return RoundToStride(kUnalignedFrame);
}

FramePosition
SpectrogramController::RoundToStride(FramePosition aFrame) const
{
    const FFTSize kStride = mSettings.GetWindowStride();

    // Calculate floor(aFrame / kStride) for both positive and negative values.
    // For positive values, this is just integer division.
    // For negative values, division truncates toward zero, so we use:
    //      ceil(n / d) == ( n + d - 1) / d
    //     ceil(-n / d) == (-n + d - 1) / d
    //     floor(n / d) == -ceil(-n / d)
    //                  == -((-n + d - 1) / d)
    const FramePosition kStrideIndex =
      aFrame >= 0 ? aFrame / kStride : -((-aFrame + kStride - 1) / kStride);
    return kStrideIndex * kStride;
}

float
SpectrogramController::GetHzPerBin() const
{
    const SampleRate kSampleRate = mAudioBuffer.GetSampleRate();
    const FFTSize kFFTSize = mSettings.GetFFTSize();
    return static_cast<float>(kSampleRate) / static_cast<float>(kFFTSize);
}