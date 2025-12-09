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
        SpectrogramController controller(audioBuffer);
    }

    static void TestSetWindowStride()
    {
        AudioBuffer audioBuffer(2, 44100);
        SpectrogramController controller(audioBuffer);

        QCOMPARE(controller.GetWindowStride(), 0);

        controller.SetWindowStride(512);
        QCOMPARE(controller.GetWindowStride(), 512);
    }

    static void TestSetWindowStrideThorwsOnZeroStride()
    {
        AudioBuffer audioBuffer(2, 44100);
        SpectrogramController controller(audioBuffer);

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
        SpectrogramController controller(audioBuffer);
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
                                 controller.SetFFTSettings(0, FFTWindow::Type::kHann));
        QVERIFY_THROWS_EXCEPTION(std::invalid_argument,
                                 controller.SetFFTSettings(255, FFTWindow::Type::kHann));
    }

    static void TestGetRows()
    {
        // The mock FFT processor just returns input samples as magnitudes
        auto mockFFTProcessorFactory = [](size_t size) {
            return std::make_unique<MockFFTProcessor>(size);
        };

        AudioBuffer buffer(1, 44100);
        buffer.AddSamples({ 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
                            13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 });

        SpectrogramController controller(buffer, mockFFTProcessorFactory);
        controller.SetFFTSettings(8, FFTWindow::Type::kRectangular);
        controller.SetWindowStride(8);

        // First one - stride 8, 3 rows
        std::vector<std::vector<float>> want = {
            { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },
            { 9.0f, 10.0f, 11.0f, 12.0f, 13.0f },
            { 17.0f, 18.0f, 19.0f, 20.0f, 21.0f },
        };
        QCOMPARE(controller.GetRows(0, 0, 3), want);

        // Next with a sample offset
        want = {
            { 5.0f, 6.0f, 7.0f, 8.0f, 9.0f },
            { 13.0f, 14.0f, 15.0f, 16.0f, 17.0f },
        };
        QCOMPARE(controller.GetRows(0, 4, 2), want);

        // Then with an overlapping stride
        controller.SetWindowStride(2);
        want = {
            { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },
            { 3.0f, 4.0f, 5.0f, 6.0f, 7.0f },
            { 5.0f, 6.0f, 7.0f, 8.0f, 9.0f },
        };
        QCOMPARE(controller.GetRows(0, 0, 3), want);
    }

    static void TestGetRowsThrowsOnInvalidChannel()
    {
        AudioBuffer buffer(1, 44100);
        SpectrogramController controller(buffer);

        QVERIFY_THROWS_EXCEPTION(std::out_of_range, (void)controller.GetRows(1, 0, 1));
    }
};

QTEST_GUILESS_MAIN(TestSpectrogramController)
#include "test_spectrogram_controller.moc"