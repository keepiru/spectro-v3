#include <cstddef>
#include <cstdint>
#include <fft_window.h>
#include <ifft_processor.h>
#include <sample_buffer.h>
#include <stdexcept>
#include <stft_processor.h>
#include <string>
#include <vector>

STFTProcessor::STFTProcessor(IFFTProcessor& aFFTProcessor,
                             FFTWindow& aWindow,
                             SampleBuffer& aBuffer)
  : mFFTProcessor(aFFTProcessor)
  , mWindow(aWindow)
  , mBuffer(aBuffer)
{
    // Validate that window size matches FFT transform size
    if (mWindow.GetSize() != mFFTProcessor.GetTransformSize()) {
        throw std::invalid_argument("Window size (" + std::to_string(mWindow.GetSize()) +
                                    ") must match FFT transform size (" +
                                    std::to_string(mFFTProcessor.GetTransformSize()) + ")");
    }
}

std::vector<std::vector<float>>
STFTProcessor::ComputeSpectrogram(int64_t aFirstSample,
                                  size_t aWindowStride,
                                  size_t aWindowCount) const
{
    // Validate parameters
    if (aWindowStride == 0) {
        throw std::invalid_argument("aWindowStride must be greater than zero");
    }

    const size_t kWindowSize = mWindow.GetSize();
    std::vector<std::vector<float>> spectrogram;
    spectrogram.reserve(aWindowCount);

    // Process each time window
    for (size_t i = 0; i < aWindowCount; ++i) {
        int64_t const kWindowFirstSample = aFirstSample + static_cast<int64_t>(i * aWindowStride);

        // Future performance optimization: grab the entire needed range once
        // before the loop to minimize locking and copy overhead.
        std::vector<float> const kSamples = mBuffer.GetSamples(kWindowFirstSample, kWindowSize);

        std::vector<float> windowedSamples = mWindow.Apply(kSamples);
        std::vector<float> const kSpectrum = mFFTProcessor.ComputeMagnitudes(windowedSamples);
        spectrogram.push_back(kSpectrum);
    }

    return spectrogram;
}
