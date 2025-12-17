#include "controllers/spectrogram_controller.h"
#include "models/audio_buffer.h"
#include "models/settings.h"
#include <QObject>
#include <cstddef>
#include <cstdint>
#include <fft_processor.h>
#include <fft_window.h>
#include <memory>
#include <sample_buffer.h>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

SpectrogramController::SpectrogramController(const Settings& aSettings,
                                             const AudioBuffer& aAudioBuffer,
                                             FFTProcessorFactory aFFTProcessorFactory,
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

    // Initialize with default FFT settings
    SetFFTSettings(kDefaultFFTSize, kDefaultWindowType);
}

void
SpectrogramController::SetFFTSettings(const size_t aTransformSize,
                                      const FFTWindow::Type aWindowType)
{
    // Clear out the old DSP objects
    mFFTProcessors.clear();
    mFFTWindows.clear();
    mSpectrogramRowCache.clear();

    // Create FFT and window instances for each channel
    for (size_t i = 0; i < mAudioBuffer.GetChannelCount(); i++) {
        auto fftProcessor = mFFTProcessorFactory(aTransformSize);
        auto fftWindow = mFFTWindowFactory(aTransformSize, aWindowType);
        const auto& sampleBuffer = mAudioBuffer.GetChannelBuffer(i);

        mFFTProcessors.emplace_back(std::move(fftProcessor));
        mFFTWindows.emplace_back(std::move(fftWindow));
    }
}

std::vector<std::vector<float>>
SpectrogramController::GetRows(size_t aChannel, int64_t aFirstSample, size_t aRowCount) const
{
    if (aChannel >= mAudioBuffer.GetChannelCount()) {
        throw std::out_of_range("Channel index out of range");
    }

    std::vector<std::vector<float>> spectrogram;
    spectrogram.reserve(aRowCount);

    const auto kSampleCount = mFFTWindows[aChannel]->GetSize();
    const auto kWindowStride = mSettings.GetWindowStride();

    for (size_t row = 0; row < aRowCount; row++) {
        const int64_t kWindowFirstSample = aFirstSample + static_cast<int64_t>(row * kWindowStride);
        const auto kAvailableSamples = GetAvailableSampleCount();
        const auto kLastNeededSample = kWindowFirstSample + static_cast<int64_t>(kSampleCount);

        if (kLastNeededSample < kWindowFirstSample) {
            // Yikes, we overflowed an int64, something is very wrong
            throw std::out_of_range("Requested sample range is invalid (overflow detected)");
        }

        // Check if the requested window is within available data, else return zeroed row
        if (kWindowFirstSample < 0 ||
            static_cast<uint64_t>(kLastNeededSample) > kAvailableSamples) {
            spectrogram.emplace_back((kSampleCount / 2) + 1, 0.0f);
            continue;
        }

        // Check cache first
        const std::pair<size_t, int64_t> cacheKey = {
            aChannel, aFirstSample + static_cast<int64_t>(row * kWindowStride)
        };

        const auto cacheIt = mSpectrogramRowCache.find(cacheKey);
        if (cacheIt != mSpectrogramRowCache.end()) {
            // Found in cache
            spectrogram.push_back(cacheIt->second);
            continue;
        }

        // Not in cache, compute it

        // Future performance optimization: grab the entire needed range once
        // before the loop to minimize locking and copy overhead.
        const auto kSamples = mAudioBuffer.GetSamples(aChannel, kWindowFirstSample, kSampleCount);
        auto windowedSamples = mFFTWindows[aChannel]->Apply(kSamples);
        auto windowedSpan = std::span<float>(windowedSamples);
        const auto kSpectrum = mFFTProcessors[aChannel]->ComputeMagnitudes(windowedSpan);

        // Store in cache
        mSpectrogramRowCache.emplace(cacheKey, kSpectrum);

        // And in the result
        spectrogram.push_back(kSpectrum);
    }
    return spectrogram;
}

size_t
SpectrogramController::GetAvailableSampleCount() const
{
    if (mAudioBuffer.GetChannelCount() == 0) {
        return 0;
    }

    // We currently assume all channels have the same sample count.
    // TODO: Maybe delegate to AudioBuffer in the future
    return mAudioBuffer.GetChannelBuffer(0).NumSamples();
}

size_t
SpectrogramController::GetChannelCount() const
{
    return mAudioBuffer.GetChannelCount();
}
