#include "controllers/spectrogram_controller.h"
#include "models/audio_buffer.h"
#include <QObject>
#include <QTest>
#include <cstddef>
#include <fft_window.h>
#include <ifft_processor.h>
#include <memory>
#include <mock_fft_processor.h>
#include <span>
#include <stdexcept>
#include <vector>

class TestSpectrogramController : public QObject
{
    Q_OBJECT

  private slots:
    static void TestConstructor()
    {
        AudioBuffer audioBuffer(2, 44100);
        const SpectrogramController controller(audioBuffer);
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
        const SpectrogramController::FFTProcessorFactory procSpy = [&procCalls](size_t size) {
            procCalls.emplace_back(size);
            return std::make_unique<MockFFTProcessor>(size);
        };

        std::vector<std::pair<size_t, FFTWindow::Type>> windowCalls;
        const SpectrogramController::FFTWindowFactory windowSpy =
          [&windowCalls](size_t size, FFTWindow::Type type) {
              windowCalls.emplace_back(size, type);
              return std::make_unique<FFTWindow>(size, type);
          };

        AudioBuffer audioBuffer(2, 44100);
        SpectrogramController controller(audioBuffer, procSpy, windowSpy);

        // Constructor calls with defaults
        QCOMPARE(procCalls,
                 (std::vector<size_t>{ SpectrogramController::kDefaultFFTSize,
                                       SpectrogramController::kDefaultFFTSize }));
        QCOMPARE(windowCalls,
                 (std::vector<std::pair<size_t, FFTWindow::Type>>{
                   std::make_pair(SpectrogramController::kDefaultFFTSize,
                                  SpectrogramController::kDefaultWindowType),
                   std::make_pair(SpectrogramController::kDefaultFFTSize,
                                  SpectrogramController::kDefaultWindowType),
                 }));

        controller.SetFFTSettings(1024, FFTWindow::Type::kRectangular);

        // SetFFTSettings calls again with new settings
        QCOMPARE(procCalls,
                 (std::vector<size_t>{ SpectrogramController::kDefaultFFTSize,
                                       SpectrogramController::kDefaultFFTSize,
                                       1024,
                                       1024 }));
        QCOMPARE(windowCalls,
                 (std::vector<std::pair<size_t, FFTWindow::Type>>{
                   std::make_pair(SpectrogramController::kDefaultFFTSize,
                                  SpectrogramController::kDefaultWindowType),
                   std::make_pair(SpectrogramController::kDefaultFFTSize,
                                  SpectrogramController::kDefaultWindowType),
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

    static std::unique_ptr<SpectrogramController> CreateControllerWithMockFFT(AudioBuffer& buffer)
    {
        // The mock FFT processor just returns input samples as magnitudes
        auto mockFFTProcessorFactory = [](size_t size) {
            return std::make_unique<MockFFTProcessor>(size);
        };

        auto controller =
          std::make_unique<SpectrogramController>(buffer, mockFFTProcessorFactory, nullptr);
        controller->SetFFTSettings(8, FFTWindow::Type::kRectangular);
        controller->SetWindowStride(8);

        return controller;
    }

    static void TestGetRowsSingleWindowComputation()
    {
        AudioBuffer buffer(1, 44100);
        buffer.AddSamples({ 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
                            13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 });
        auto controller = CreateControllerWithMockFFT(buffer);

        const std::vector<std::vector<float>> kWant = {
            { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },
        };
        const auto kGot = controller->GetRows(0, 0, 1);
        QCOMPARE(kGot, kWant);
    }

    static void TestGetRowsMultipleNonoverlappingWindows()
    {
        AudioBuffer buffer(1, 44100);
        buffer.AddSamples({ 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
                            13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 });
        auto controller = CreateControllerWithMockFFT(buffer);

        const std::vector<std::vector<float>> kWant = {
            { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },
            { 9.0f, 10.0f, 11.0f, 12.0f, 13.0f },
            { 17.0f, 18.0f, 19.0f, 20.0f, 21.0f },
        };
        const auto kGot = controller->GetRows(0, 0, 3);
        QCOMPARE(kGot, kWant);
    }

    static void TestGetRows50PercentOverlap()
    {
        AudioBuffer buffer(1, 44100);
        buffer.AddSamples({ 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
                            13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 });
        auto controller = CreateControllerWithMockFFT(buffer);
        controller->SetWindowStride(4); // 50% overlap

        const std::vector<std::vector<float>> kWant = {
            { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },      { 5.0f, 6.0f, 7.0f, 8.0f, 9.0f },
            { 9.0f, 10.0f, 11.0f, 12.0f, 13.0f },  { 13.0f, 14.0f, 15.0f, 16.0f, 17.0f },
            { 17.0f, 18.0f, 19.0f, 20.0f, 21.0f },
        };
        const auto kGot = controller->GetRows(0, 0, 5);
        QCOMPARE(kGot, kWant);
    }

    static void TestGetRows75PercentOverlap()
    {
        AudioBuffer buffer(1, 44100);
        buffer.AddSamples({ 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
                            13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 });
        auto controller = CreateControllerWithMockFFT(buffer);
        controller->SetWindowStride(2); // 75% overlap

        const std::vector<std::vector<float>> kWant = {
            { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },      { 3.0f, 4.0f, 5.0f, 6.0f, 7.0f },
            { 5.0f, 6.0f, 7.0f, 8.0f, 9.0f },      { 7.0f, 8.0f, 9.0f, 10.0f, 11.0f },
            { 9.0f, 10.0f, 11.0f, 12.0f, 13.0f },  { 11.0f, 12.0f, 13.0f, 14.0f, 15.0f },
            { 13.0f, 14.0f, 15.0f, 16.0f, 17.0f }, { 15.0f, 16.0f, 17.0f, 18.0f, 19.0f },
            { 17.0f, 18.0f, 19.0f, 20.0f, 21.0f }, { 19.0f, 20.0f, 21.0f, 22.0f, 23.0f },
        };
        const auto kGot = controller->GetRows(0, 0, 10);
        QCOMPARE(kGot, kWant);
    }

    static void TestGetRowsWithNegativeStartSampleAndZeroPadding()
    {
        AudioBuffer buffer(1, 44100);
        buffer.AddSamples({ 1, 2, 3, 4, 5, 6, 7, 8 });
        auto controller = CreateControllerWithMockFFT(buffer);
        controller->SetWindowStride(4);

        const std::vector<std::vector<float>> kWant = {
            { 0.0f, 0.0f, 0.0f, 0.0f, 1.0f },
            { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },
            { 5.0f, 6.0f, 7.0f, 8.0f, 0.0f },
        };
        const auto kGot = controller->GetRows(0, -4, 3);
        QCOMPARE(kGot, kWant);
    }

    static void TestGetRowsWithStartSampleBeyondBufferEndAndZeroPadding()
    {
        AudioBuffer buffer(1, 44100);
        buffer.AddSamples({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 });
        auto controller = CreateControllerWithMockFFT(buffer);

        const std::vector<std::vector<float>> kWant = {
            { 13.0f, 14.0f, 15.0f, 16.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
        };
        const auto kGot = controller->GetRows(0, 12, 2);
        QCOMPARE(kGot, kWant);
    }

    static void TestGetRowsWithHannWindowIntegration()
    {
        AudioBuffer buffer(1, 44100);
        buffer.AddSamples({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 });
        auto controller = CreateControllerWithMockFFT(buffer);
        controller->SetFFTSettings(8, FFTWindow::Type::kHann);

        // Hann window attenuates edges, so we'll see lower magnitudes at the
        // edges.  Keep in mind our MockFFTProcessor just returns the input
        // samples as magnitudes so the only transformation is from the
        // windowing.
        const std::vector<std::vector<float>> kWant = {
            { 0.0f, 0.188255101f, 0.611260474f, 0.950484395f, 0.950484395f },
            { 0.0f, 0.188255101f, 0.611260474f, 0.950484395f, 0.950484395f }
        };

        const auto kGot = controller->GetRows(0, 0, 2);
        QCOMPARE(kGot, kWant);
    }

    static void TestGetRowsThrowsOnInvalidChannel()
    {
        AudioBuffer buffer(1, 44100);
        const SpectrogramController controller(buffer);

        QVERIFY_THROWS_EXCEPTION(std::out_of_range, (void)controller.GetRows(1, 0, 1));
    }

    static void TestGetAvailableSampleCount()
    {
        AudioBuffer buffer(2, 44100);
        buffer.AddSamples({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 });

        const SpectrogramController controller(buffer);
        const auto kExpectedSampleCount = 5; // 10 samples / 2 channels
        QCOMPARE(controller.GetAvailableSampleCount(), kExpectedSampleCount);
    }

    static void TestGetChannelCount()
    {
        AudioBuffer buffer(3, 44100);
        const SpectrogramController controller(buffer);
        QCOMPARE(controller.GetChannelCount(), 3);
    }
};

QTEST_GUILESS_MAIN(TestSpectrogramController)
#include "test_spectrogram_controller.moc"