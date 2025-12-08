#include <audio_buffer.h>
#include <fft_window.h>
#include <ifft_processor.h>
#include <sample_buffer.h>
#include <spectrogram_controller.h>
#include <stdexcept>
#include <stft_processor.h>

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
}

void
SpectrogramController::SetWindowStride(size_t aStride)
{
    if (aStride == 0) {
        throw std::invalid_argument("Window stride must be greater than zero");
    }
    mWindowStride = aStride;
}