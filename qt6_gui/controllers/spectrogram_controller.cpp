#include "controllers/spectrogram_controller.h"
#include "ifft_processor.h"
#include "include/global_constants.h"
#include "models/audio_buffer.h"
#include "models/settings.h"
#include <QObject>
#include <cstddef>
#include <cstdint>
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
        mFFTProcessorFactory = [](size_t size) { return std::make_unique<FFTProcessor>(size); };
    }

    if (!mFFTWindowFactory) {
        mFFTWindowFactory = [](size_t size, FFTWindow::Type type) {
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
SpectrogramController::GetRows(ChannelCount aChannel, int64_t aFirstSample, size_t aRowCount) const
{
    if (aChannel >= mAudioBuffer.GetChannelCount()) {
        throw std::out_of_range("Channel index out of range");
    }

    std::vector<std::vector<float>> spectrogram;
    spectrogram.reserve(aRowCount);

    const int64_t kWindowStride = mSettings.GetWindowStride();

    for (size_t row = 0; row < aRowCount; row++) {
        const int64_t kWindowFirstSample =
          aFirstSample + (static_cast<int64_t>(row) * kWindowStride);
        const auto kSpectrum = GetRow(aChannel, kWindowFirstSample);
        spectrogram.push_back(kSpectrum);
    }
    return spectrogram;
}

std::vector<float>
SpectrogramController::GetRow(ChannelCount aChannel, int64_t aFirstSample) const
{
    if (aChannel >= mAudioBuffer.GetChannelCount()) {
        throw std::out_of_range("Channel index out of range");
    }

    const int64_t kSampleCount = mFFTWindows.at(aChannel)->GetSize();
    const int64_t kAvailableSamples = GetAvailableFrameCount();
    const int64_t kLastNeededSample = aFirstSample + kSampleCount;

    // Check if the requested window is within available data, else return zeroed row
    if (aFirstSample < 0 || kLastNeededSample > kAvailableSamples) {
        return std::vector<float>((kSampleCount / 2) + 1, 0.0f);
    }

    // Check cache first
    const std::pair<size_t, int64_t> cacheKey = { aChannel, aFirstSample };

    const auto cacheIt = mSpectrogramRowCache.find(cacheKey);
    if (cacheIt != mSpectrogramRowCache.end()) {
        // Found in cache
        return cacheIt->second;
    }

    // Not in cache, compute it
    const auto kSpectrum = ComputeFFT(aChannel, aFirstSample);

    // Store in cache
    mSpectrogramRowCache.emplace(cacheKey, kSpectrum);

    return kSpectrum;
}

std::vector<float>
SpectrogramController::ComputeFFT(ChannelCount aChannel, int64_t aFirstSample) const
{
    const int64_t kSampleCount = mFFTWindows.at(aChannel)->GetSize();
    // Future performance optimization: grab the entire needed range once
    // before the loop to minimize locking and copy overhead.
    const auto kSamples = mAudioBuffer.GetSamples(aChannel, aFirstSample, kSampleCount);
    auto windowedSamples = mFFTWindows[aChannel]->Apply(kSamples);
    auto windowedSpan = std::span<float>(windowedSamples);
    return mFFTProcessors[aChannel]->ComputeDecibels(windowedSpan);
}

int64_t
SpectrogramController::GetAvailableFrameCount() const
{
    return mAudioBuffer.FrameCount();
}

ChannelCount
SpectrogramController::GetChannelCount() const
{
    return mAudioBuffer.GetChannelCount();
}

int64_t
SpectrogramController::CalculateTopOfWindow(int64_t aCursorFrame) const
{
    const int64_t kUnalignedFrame = aCursorFrame - mSettings.GetFFTSize();
    return RoundToStride(kUnalignedFrame);
}

int64_t
SpectrogramController::RoundToStride(int64_t aFrame) const
{
    const int64_t kStride = mSettings.GetWindowStride();

    // Calculate floor(aFrame / kStride) for both positive and negative values.
    // For positive values, this is just integer division.
    // For negative values, division truncates toward zero, so we use:
    //      ceil(n / d) == ( n + d - 1) / d
    //     ceil(-n / d) == (-n + d - 1) / d
    //     floor(n / d) == -ceil(-n / d)
    //                  == -((-n + d - 1) / d)
    const int64_t kStrideIndex =
      aFrame >= 0 ? aFrame / kStride : -((-aFrame + kStride - 1) / kStride);
    return kStrideIndex * kStride;
}

float
SpectrogramController::GetHzPerBin() const
{
    const size_t kSampleRate = mAudioBuffer.GetSampleRate();
    const size_t kFFTSize = mSettings.GetFFTSize();
    return static_cast<float>(kSampleRate) / static_cast<float>(kFFTSize);
}