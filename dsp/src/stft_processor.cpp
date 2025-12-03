#include <fft_window.h>
#include <ifft_processor.h>
#include <sample_buffer.h>
#include <stdexcept>
#include <stft_processor.h>

STFTProcessor::STFTProcessor(IFFTProcessor& fft_processor, FFTWindow& window, SampleBuffer& buffer)
  : m_fft_processor(fft_processor)
  , m_window(window)
  , m_buffer(buffer)
{
    // Validate that window size matches FFT transform size
    if (m_window.getSize() != m_fft_processor.getTransformSize()) {
        throw std::invalid_argument("Window size (" + std::to_string(m_window.getSize()) +
                                    ") must match FFT transform size (" +
                                    std::to_string(m_fft_processor.getTransformSize()) + ")");
    }
}

std::vector<std::vector<float>>
STFTProcessor::computeSpectrogram(int64_t first_sample,
                                   size_t window_stride,
                                   size_t window_count) const
{
    // Validate parameters
    if (window_stride == 0) {
        throw std::invalid_argument("window_stride must be greater than zero");
    }

    const size_t window_size = m_window.getSize();
    std::vector<std::vector<float>> spectrogram;
    spectrogram.reserve(window_count);

    // Process each time window
    for (size_t i = 0; i < window_count; ++i) {
        int64_t window_first_sample = first_sample + static_cast<int64_t>(i * window_stride);

        // Future performance optimization: grab the entire needed range once
        // before the loop to minimize locking and copy overhead.
        std::vector<float> samples = m_buffer.getSamples(window_first_sample, window_size);

        std::vector<float> windowed_samples = m_window.apply(samples);
        std::vector<float> spectrum = m_fft_processor.computeMagnitudes(windowed_samples);
        spectrogram.push_back(std::move(spectrum));
    }

    return spectrogram;
}
