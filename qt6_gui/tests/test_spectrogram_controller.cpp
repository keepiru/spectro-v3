#include <QObject>
#include <QTest>
#include <audio_buffer.h>
#include <cstddef>
#include <fft_window.h>
#include <ifft_processor.h>
#include <memory>
#include <mock_fft_processor.h>
#include <span>
#include <spectrogram_controller.h>
#include <stdexcept>
#include <vector>

class TestSpectrogramController : public QObject
{
    Q_OBJECT

  private slots:
    static void TestConstructor()
    {
        AudioBuffer audioBuffer(2, 44100);
        SpectrogramController controller(audioBuffer, nullptr, nullptr);
    }

    static void TestSetWindowStride()
    {
        AudioBuffer audioBuffer(2, 44100);
        SpectrogramController controller(audioBuffer, nullptr, nullptr);

        QCOMPARE(controller.GetWindowStride(), 0);

        controller.SetWindowStride(512);
        QCOMPARE(controller.GetWindowStride(), 512);
    }

    static void TestSetWindowStrideThorwsOnZeroStride()
    {
        AudioBuffer audioBuffer(2, 44100);
        SpectrogramController controller(audioBuffer, nullptr, nullptr);

        QVERIFY_THROWS_EXCEPTION(std::invalid_argument, controller.SetWindowStride(0));
    }

    static void TestSetFFTSettings()
    {
        std::vector<size_t> procCalls;
        SpectrogramController::FFTProcessorFactory procSpy = [&procCalls](size_t size) {
            procCalls.emplace_back(size);
            return std::make_unique<MockFFTProcessor>(size);
        };

        std::vector<std::pair<size_t, FFTWindow::Type>> windowCalls;
        SpectrogramController::FFTWindowFactory windowSpy = [&windowCalls](size_t size,
                                                                           FFTWindow::Type type) {
            windowCalls.emplace_back(size, type);
            return std::make_unique<FFTWindow>(size, type);
        };

        AudioBuffer audioBuffer(2, 44100);
        SpectrogramController controller(audioBuffer, procSpy, windowSpy);

        // Constructor calls with defaults
        QCOMPARE(procCalls, (std::vector<size_t>{ 512, 512 }));
        QCOMPARE(windowCalls,
                 (std::vector<std::pair<size_t, FFTWindow::Type>>{
                   std::make_pair(512, FFTWindow::Type::kHann),
                   std::make_pair(512, FFTWindow::Type::kHann) }));

        controller.SetFFTSettings(1024, FFTWindow::Type::kRectangular);

        // SetFFTSettings calls again with new settings
        QCOMPARE(procCalls, (std::vector<size_t>{ 512, 512, 1024, 1024 }));
        QCOMPARE(windowCalls,
                 (std::vector<std::pair<size_t, FFTWindow::Type>>{
                   std::make_pair(512, FFTWindow::Type::kHann),
                   std::make_pair(512, FFTWindow::Type::kHann),
                   std::make_pair(1024, FFTWindow::Type::kRectangular),
                   std::make_pair(1024, FFTWindow::Type::kRectangular) }));
    }

    static void TestSetFFTSettingsThrowsOnInvalidWindow()
    {
        AudioBuffer audioBuffer(2, 44100);
        SpectrogramController controller(audioBuffer, nullptr, nullptr);
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
                                 controller.SetFFTSettings(0, FFTWindow::Type::kHann));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
                                 controller.SetFFTSettings(255, FFTWindow::Type::kHann));
    }
};

QTEST_GUILESS_MAIN(TestSpectrogramController)
#include "test_spectrogram_controller.moc"