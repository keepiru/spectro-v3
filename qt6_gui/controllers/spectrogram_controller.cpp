#include <audio_buffer.h>
#include <fft_processor.h>
#include <fft_window.h>
#include <sample_buffer.h>
#include <spectrogram_controller.h>
#include <stdexcept>
#include <stft_processor.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

SpectrogramController::SpectrogramController(AudioBuffer& aAudioBuffer,
                                             FFTProcessorFactory aFFTProcessorFactory,
                                             FFTWindowFactory aFFTWindowFactory,
                                             QObject* aParent)
  : QObject(aParent)
  , mAudioBuffer(aAudioBuffer)
  , mFFTProcessorFactory(std::move(aFFTProcessorFactory))
  , mFFTWindowFactory(std::move(aFFTWindowFactory))
  , mWindowStride(0)

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
    constexpr size_t kDefaultFFTSize = 512;
    SetFFTSettings(kDefaultFFTSize, FFTWindow::Type::kHann);
}

void
SpectrogramController::SetWindowStride(size_t aStride)
{
    if (aStride == 0) {
        throw std::invalid_argument("Window stride must be greater than zero");
    }
    mWindowStride = aStride;
}

void
SpectrogramController::SetFFTSettings(const size_t aTransformSize,
                                      const FFTWindow::Type aWindowType)
{
    // Clear out the old DSP objects
    mFFTProcessors.clear();
    mFFTWindows.clear();
    mSTFTProcessors.clear();

    // Create FFT and window instances for each channel
    for (size_t i = 0; i < mAudioBuffer.GetChannelCount(); i++) {
        auto fftProcessor = mFFTProcessorFactory(aTransformSize);
        auto fftWindow = mFFTWindowFactory(aTransformSize, aWindowType);
        const auto& sampleBuffer = mAudioBuffer.GetChannelBuffer(i);
        auto stftProcessor =
          std::make_unique<STFTProcessor>(*fftProcessor, *fftWindow, sampleBuffer);

        mFFTProcessors.emplace_back(std::move(fftProcessor));
        mFFTWindows.emplace_back(std::move(fftWindow));
        mSTFTProcessors.emplace_back(std::move(stftProcessor));
    }
}

std::vector<std::vector<float>>
SpectrogramController::GetRows(size_t aChannel, int64_t aFirstSample, size_t aRowCount) const
{
    if (aChannel >= mAudioBuffer.GetChannelCount()) {
        throw std::out_of_range("Channel index out of range");
    }

    return mSTFTProcessors[aChannel]->ComputeSpectrogram(aFirstSample, mWindowStride, aRowCount);
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
