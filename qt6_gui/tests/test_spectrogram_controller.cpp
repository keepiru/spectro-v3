#include "controllers/spectrogram_controller.h"
#include "models/audio_buffer.h"
#include "models/settings.h"
#include <QObject>
#include <QTest>
#include <cstddef>
#include <fft_window.h>
#include <ifft_processor.h>
#include <memory>
#include <mock_fft_processor.h>
#include <stdexcept>
#include <vector>

class TestSpectrogramController : public QObject
{
    Q_OBJECT

  private slots:
    static void TestConstructor()
    {
        const Settings settings;
        const AudioBuffer audioBuffer;
        const SpectrogramController controller(settings, audioBuffer);
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

        Settings settings;
        const AudioBuffer audioBuffer;
        const SpectrogramController controller(settings, audioBuffer, procSpy, windowSpy);

        // Constructor calls with defaults
        QCOMPARE(procCalls,
                 (std::vector<size_t>{ SpectrogramController::KDefaultFftSize,
                                       SpectrogramController::KDefaultFftSize }));
        QCOMPARE(windowCalls,
                 (std::vector<std::pair<size_t, FFTWindow::Type>>{
                   std::make_pair(SpectrogramController::KDefaultFftSize,
                                  SpectrogramController::KDefaultWindowType),
                   std::make_pair(SpectrogramController::KDefaultFftSize,
                                  SpectrogramController::KDefaultWindowType),
                 }));

        settings.SetFFTSettings(1024, FFTWindow::Type::Rectangular);

        // SetFFTSettings calls again with new settings
        QCOMPARE(procCalls,
                 (std::vector<size_t>{ SpectrogramController::KDefaultFftSize,
                                       SpectrogramController::KDefaultFftSize,
                                       1024,
                                       1024 }));
        QCOMPARE(windowCalls,
                 (std::vector<std::pair<size_t, FFTWindow::Type>>{
                   std::make_pair(SpectrogramController::KDefaultFftSize,
                                  SpectrogramController::KDefaultWindowType),
                   std::make_pair(SpectrogramController::KDefaultFftSize,
                                  SpectrogramController::KDefaultWindowType),
                   std::make_pair(1024, FFTWindow::Type::Rectangular),
                   std::make_pair(1024, FFTWindow::Type::Rectangular) }));
    }

    static std::unique_ptr<SpectrogramController> CreateControllerWithMockFFT(Settings& aSettings,
                                                                              AudioBuffer& aBuffer)
    {
        // The mock FFT processor just returns input samples as magnitudes
        auto mockFFTProcessorFactory = [](size_t size) {
            return std::make_unique<MockFFTProcessor>(size);
        };

        aSettings.SetFFTSettings(8, FFTWindow::Type::Rectangular);
        auto controller = std::make_unique<SpectrogramController>(
          aSettings, aBuffer, mockFFTProcessorFactory, nullptr);
        aSettings.SetWindowScale(1);

        return controller;
    }

    static void TestGetRowsSingleWindowComputation()
    {
        AudioBuffer buffer;
        buffer.Reset(1, 44100);
        buffer.AddSamples({ 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
                            13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 });
        Settings settings;
        auto controller = CreateControllerWithMockFFT(settings, buffer);

        const std::vector<std::vector<float>> kWant = {
            { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },
        };
        QCOMPARE(controller->GetRows(0, 0, 1), kWant);
        QCOMPARE(controller->GetRow(0, 0), kWant[0]);
        QCOMPARE(controller->ComputeFFT(0, 0), kWant[0]);
    }

    static void TestGetRowsMultipleNonoverlappingWindows()
    {
        AudioBuffer buffer;
        buffer.Reset(1, 44100);
        buffer.AddSamples({ 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
                            13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 });
        Settings settings;
        auto controller = CreateControllerWithMockFFT(settings, buffer);
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
        Settings settings;
        AudioBuffer buffer;
        buffer.Reset(1, 44100);
        buffer.AddSamples({ 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
                            13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24 });
        auto controller = CreateControllerWithMockFFT(settings, buffer);
        settings.SetWindowScale(2); // 50% overlap

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
        Settings settings;
        AudioBuffer buffer;
        buffer.Reset(1, 44100);
        buffer.AddSamples({ 1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13,
                            14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26 });
        auto controller = CreateControllerWithMockFFT(settings, buffer);
        settings.SetWindowScale(4); // 75% overlap

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

    static void TestGetRowsWithStartSampleBeyondBufferEndReturnsZeroedRows()
    {
        Settings settings;
        AudioBuffer buffer;
        buffer.Reset(1, 44100);
        buffer.AddSamples({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });
        auto controller = CreateControllerWithMockFFT(settings, buffer);

        const std::vector<std::vector<float>> kWant = {
            { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f },
            { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
        };

        QCOMPARE(controller->GetRows(0, 0, 2), kWant);

        // Do the same thing with GetRow
        QCOMPARE(controller->GetRow(0, 0), kWant[0]);
        QCOMPARE(controller->GetRow(0, 8), kWant[1]);

        // And with ComputeFFT
        QCOMPARE(controller->ComputeFFT(0, 0), kWant[0]);
        QVERIFY_THROWS_EXCEPTION(std::out_of_range, (void)controller->ComputeFFT(0, 8));
    }

    static void TestGetRowsWithNegativeStartSampleReturnsZeroedRows()
    {
        Settings settings;
        AudioBuffer buffer;
        buffer.Reset(1, 44100);
        buffer.AddSamples({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 });
        auto controller = CreateControllerWithMockFFT(settings, buffer);
        const std::vector<std::vector<float>> kWant = {
            { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f },
            { 7.0f, 8.0f, 9.0f, 10.0f, 11.0f },
        };
        QCOMPARE(controller->GetRows(0, -2, 2), kWant);

        // Do the same thing with GetRow
        QCOMPARE(controller->GetRow(0, -2), kWant[0]);
        QCOMPARE(controller->GetRow(0, 6), kWant[1]);

        // And with ComputeFFT
        QVERIFY_THROWS_EXCEPTION(std::out_of_range, (void)controller->ComputeFFT(0, -2));
        QCOMPARE(controller->ComputeFFT(0, 6), kWant[1]);
    }

    static void TestGetRowsWithHannWindowIntegration()
    {
        Settings settings;
        AudioBuffer buffer;
        buffer.Reset(1, 44100);
        buffer.AddSamples({ 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 });
        auto controller = CreateControllerWithMockFFT(settings, buffer);
        settings.SetFFTSettings(8, FFTWindow::Type::Hann);

        // Hann window attenuates edges, so we'll see lower magnitudes at the
        // edges.  Keep in mind our MockFFTProcessor just returns the input
        // samples as magnitudes so the only transformation is from the
        // windowing.
        const std::vector<std::vector<float>> kWant = {
            { 0.0f, 0.188255101f, 0.611260474f, 0.950484395f, 0.950484395f },
            { 0.0f, 0.188255101f, 0.611260474f, 0.950484395f, 0.950484395f }
        };

        QCOMPARE(controller->GetRows(0, 0, 2), kWant);
        QCOMPARE(controller->GetRow(0, 0), kWant[0]);
        QCOMPARE(controller->ComputeFFT(0, 0), kWant[0]);
    }

    // NOLINTNEXTLINE(readability-function-cognitive-complexity) QTest macro expansion
    static void TestGetRowsThrowsOnInvalidChannel()
    {
        const Settings settings;
        AudioBuffer buffer;
        buffer.Reset(1, 44100);
        const SpectrogramController controller(settings, buffer);

        QVERIFY_THROWS_EXCEPTION(std::out_of_range, (void)controller.GetRows(1, 0, 1));
        QVERIFY_THROWS_EXCEPTION(std::out_of_range, (void)controller.GetRow(1, 0));
        QVERIFY_THROWS_EXCEPTION(std::out_of_range, (void)controller.ComputeFFT(1, 0));
    }

    static void TestGetAvailableSampleCount()
    {
        const Settings settings;
        AudioBuffer buffer;
        const SpectrogramController controller(settings, buffer);

        buffer.AddSamples({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 });
        const auto kExpectedSampleCount = 5; // 10 samples / 2 channels
        QCOMPARE(controller.GetAvailableSampleCount(), kExpectedSampleCount);
    }

    static void TestGetChannelCount()
    {
        const Settings settings;
        AudioBuffer buffer;
        buffer.Reset(3, 44100);
        const SpectrogramController controller(settings, buffer);
        QCOMPARE(controller.GetChannelCount(), 3);
    }

    static void TestCalculateTopOfWindow()
    {
        Settings settings;
        const AudioBuffer audioBuffer;
        SpectrogramController controller(settings, audioBuffer);

        settings.SetFFTSettings(8, FFTWindow::Type::Rectangular);

        auto check = [&](int64_t index, size_t scale, int64_t want) {
            settings.SetWindowScale(scale);

            const int64_t have = controller.CalculateTopOfWindow(index);
            qDebug("CalculateTopOfWindow: sample=%ld scale=%zu => topSample=%ld (want %ld)",
                   index,
                   scale,
                   have,
                   want);
            QCOMPARE(have, want);
        };

        check(6, 1, -8); //[-8 -7 -6 -5 -4 -3 -2 -1] 0  1  2  3  4  5
        check(6, 2, -4); // -8 -7 -6 -5[-4 -3 -2 -1  0  1  2  3] 4  5
        check(6, 4, -2); // -8 -7 -6 -5 -4 -3[-2 -1  0  1  2  3  4  5]
        check(6, 8, -2); // -8 -7 -6 -5 -4 -3[-2 -1  0  1  2  3  4  5]

        check(7, 1, -8); //[-8 -7 -6 -5 -4 -3 -2 -1] 0  1  2  3  4  5  6
        check(7, 2, -4); // -8 -7 -6 -5[-4 -3 -2 -1  0  1  2  3] 4  5  6
        check(7, 4, -2); // -8 -7 -6 -5 -4 -3[-2 -1  0  1  2  3  4  5] 6
        check(7, 8, -1); // -8 -7 -6 -5 -4 -3 -2[-1  0  1  2  3  4  5  6]

        check(8, 1, 0); // -8 -7 -6 -5 -4 -3 -2 -1 [0  1  2  3  4  5  6  7]
        check(8, 2, 0); // -8 -7 -6 -5 -4 -3 -2 -1 [0  1  2  3  4  5  6  7]
        check(8, 4, 0); // -8 -7 -6 -5 -4 -3 -2 -1 [0  1  2  3  4  5  6  7]
        check(8, 8, 0); // -8 -7 -6 -5 -4 -3 -2 -1 [0  1  2  3  4  5  6  7]

        check(12, 1, 0); // [0  1  2  3  4  5  6  7] 8  9  10  11
        check(12, 2, 4); //  0  1  2  3 [4  5  6  7  8  9  10  11]
        check(12, 4, 4); //  0  1  2  3 [4  5  6  7  8  9  10  11]
        check(12, 8, 4); //  0  1  2  3 [4  5  6  7  8  9  10  11]

        check(13, 1, 0); // [0  1  2  3  4  5  6  7] 8  9  10  11  12
        check(13, 2, 4); //  0  1  2  3 [4  5  6  7  8  9  10  11] 12
        check(13, 4, 4); //  0  1  2  3 [4  5  6  7  8  9  10  11] 12
        check(13, 8, 5); //  0  1  2  3  4 [5  6  7  8  9  10  11  12]

        check(14, 1, 0); // [0  1  2  3  4  5  6  7] 8  9  10  11  12  13
        check(14, 2, 4); //  0  1  2  3 [4  5  6  7  8  9  10  11] 12  13
        check(14, 4, 6); //  0  1  2  3  4  5 [6  7  8  9  10  11  12  13]
        check(14, 8, 6); //  0  1  2  3  4  5 [6  7  8  9  10  11  12  13]

        check(15, 1, 0); // [0  1  2  3  4  5  6  7] 8  9  10  11  12  13  14
        check(15, 2, 4); //  0  1  2  3 [4  5  6  7  8  9  10  11] 12  13  14
        check(15, 4, 6); //  0  1  2  3  4  5 [6  7  8  9  10  11  12  13] 14
        check(15, 8, 7); //  0  1  2  3  4  5  6 [7  8  9  10  11  12  13  14]

        check(16, 1, 8); //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15]
        check(16, 2, 8); //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15]
        check(16, 4, 8); //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15]
        check(16, 8, 8); //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15]

        check(17, 1, 8); //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15] 16
        check(17, 2, 8); //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15] 16
        check(17, 4, 8); //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15] 16
        check(17, 8, 9); //  0  1  2  3  4  5  6  7  8 [9  10  11  12  13  14  15  16]

        check(18, 1, 8);  //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15] 16  17
        check(18, 2, 8);  //  0  1  2  3  4  5  6  7 [8  9  10  11  12  13  14  15] 16  17
        check(18, 4, 10); //  0  1  2  3  4  5  6  7  8  9 [10  11  12  13  14  15  16  17]
        check(18, 8, 10); //  0  1  2  3  4  5  6  7  8  9 [10  11  12  13  14  15  16  17]
    }

    static void TestRoundToStride()
    {
        Settings settings;
        const AudioBuffer audioBuffer;
        SpectrogramController controller(settings, audioBuffer);

        settings.SetFFTSettings(8, FFTWindow::Type::Rectangular);

        auto check = [&](size_t stride, int64_t sample, int64_t want) {
            if (settings.GetFFTSize() % stride != 0) {
                throw std::invalid_argument("Stride must divide FFT size evenly");
            }
            settings.SetWindowScale(settings.GetFFTSize() / stride);

            const int64_t have = controller.RoundToStride(sample);
            qDebug("RoundToStride: stride=%zu, sample=%ld, got=%ld (want %ld)",
                   stride,
                   sample,
                   have,
                   want);
            QCOMPARE(have, want);
        };

        check(1, -2, -2);
        check(1, -1, -1);
        check(1, 0, 0);
        check(1, 1, 1);
        check(1, 2, 2);

        check(2, -3, -4);
        check(2, -2, -2);
        check(2, -1, -2);
        check(2, 0, 0);
        check(2, 1, 0);
        check(2, 2, 2);

        check(4, -5, -8);
        check(4, -4, -4);
        check(4, -3, -4);
        check(4, -2, -4);
        check(4, -1, -4);
        check(4, 0, 0);
        check(4, 1, 0);
        check(4, 2, 0);
        check(4, 3, 0);
        check(4, 4, 4);
        check(4, 5, 4);

        check(8, -10, -16);
        check(8, -9, -16);
        check(8, -8, -8);
        check(8, -1, -8);
        check(8, 0, 0);
        check(8, 7, 0);
        check(8, 8, 8);
        check(8, 15, 8);
        check(8, 16, 16);
        check(8, 17, 16);
    }
};

QTEST_GUILESS_MAIN(TestSpectrogramController)
#include "test_spectrogram_controller.moc"