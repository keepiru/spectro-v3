#include "controllers/spectrogram_controller.h"
#include "models/audio_buffer.h"
#include "views/spectrogram_view.h"
#include <QSignalSpy>
#include <catch2/catch_all.hpp>
#include <mock_fft_processor.h>
#include <vector>

using namespace Catch::Matchers;

namespace {

/// @brief Test fixture that injects controllable viewport dimensions
///
/// SpectrogramView depends on Qt's viewport()->width() and viewport()->height()
/// methods, which return unpredictable values in test environments. This fixture
/// overrides the virtual getters to return controlled test dimensions.
///
/// The test dimensions (640x478) were chosen to match realistic window sizes
/// and to enable testing of scroll calculations with known values.
class TestableSpectrogramView : public SpectrogramView
{
  public:
    using SpectrogramView::SpectrogramView;

    [[nodiscard]] int GetViewportWidth() const override { return mTestWidth; }
    [[nodiscard]] int GetViewportHeight() const override { return mTestHeight; }

    void SetTestWidth(int aWidth) { mTestWidth = aWidth; }
    void SetTestHeight(int aHeight) { mTestHeight = aHeight; }

  private:
    int mTestWidth = 640;  // pixels
    int mTestHeight = 478; // pixels (matches kViewportHeight constant below)
};

/// @brief Convert a QImage to a string representation for easy comparison in tests.
/// @param image The QImage to convert.
/// @return A string representation of the image's pixel RGB values.
std::string
QImageToString(const QImage& image)
{
    std::string result = "\n";
    for (int yCoord = 0; yCoord < image.height(); ++yCoord) {
        for (int xCoord = 0; xCoord < image.width(); ++xCoord) {
            auto pixel = image.pixel(xCoord, yCoord);
            result += std::format("{:02X}{:02X}{:02X} ", qRed(pixel), qGreen(pixel), qBlue(pixel));
        }
        result += '\n';
    }
    return result;
}
} // namespace

TEST_CASE("SpectrogramView constructor", "[spectrogram_view]")
{
    const Settings settings;
    const AudioBuffer audioBuffer;
    const SpectrogramController controller(settings, audioBuffer);
    const SpectrogramView view(controller);

    REQUIRE(view.minimumWidth() > 0);
    REQUIRE(view.minimumHeight() > 0);
}

TEST_CASE("SpectrogramView is widget", "[spectrogram_view]")
{
    const Settings settings;
    const AudioBuffer audioBuffer;
    const SpectrogramController controller(settings, audioBuffer);
    SpectrogramView view(controller);
    REQUIRE(qobject_cast<QWidget*>(&view) != nullptr);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity) macro expansion
TEST_CASE("SpectrogramView::GenerateSpectrogramImage", "[spectrogram_view]")
{
    Settings settings;

    AudioBuffer audioBuffer;
    const SpectrogramController controller(
      settings, audioBuffer, MockFFTProcessor::GetFactory(), nullptr);
    SpectrogramView view(controller);

    SECTION("throws on invalid channel count")
    {
        // mock controller to return invalid channel count
        class MockController : public SpectrogramController
        {
          public:
            using SpectrogramController::SpectrogramController;
            ChannelCount GetChannelCount() const override { return GENERATE(0, GKMaxChannels + 1); }
        };
        const MockController mockController(settings, audioBuffer);
        SpectrogramView mockView(mockController);
        REQUIRE_THROWS_MATCHES(mockView.GenerateSpectrogramImage(256, 256),
                               std::runtime_error,
                               MessageMatches(ContainsSubstring("out of range")));
    }

    SECTION("generates image of correct size")
    {
        constexpr size_t width = 512;
        constexpr size_t height = 256;
        const QImage image = view.GenerateSpectrogramImage(width, height);
        CHECK(image.width() == width);
        CHECK(image.height() == height);
    }

    SECTION("generates a black image if aperture range is zero")
    {
        settings.SetApertureMinDecibels(10.0f);
        settings.SetApertureMaxDecibels(10.0f); // zero range

        const QImage image = view.GenerateSpectrogramImage(256, 256);

        CHECK(image.width() == 256);
        CHECK(image.height() == 256);

        // Check that all pixels are black
        for (int yCoord = 0; yCoord < image.height(); ++yCoord) {
            for (int xCoord = 0; xCoord < image.width(); ++xCoord) {
                REQUIRE(image.pixel(xCoord, yCoord) == qRgb(0, 0, 0));
            }
        }
    }

    SECTION("generates correct image data based on audio buffer samples")
    {
        // Integration test pulling data through the full pipeline.

        // With this aperture and a linear colormap, the input samples 0..255 will
        // go through the mock FFT, become decibel ranges 0..255, and map directly
        // to RGB values.
        settings.SetApertureMinDecibels(0);
        settings.SetApertureMaxDecibels(255);

        // Fill audio buffer with known samples
        audioBuffer.Reset(1, 44100);
        audioBuffer.AddSamples({ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 });

        settings.SetFFTSettings(8, FFTWindow::Type::Rectangular);
        settings.SetWindowScale(1); // no overlap

        // Update scrollbar range so frame 0 is at the top of the view
        view.UpdateScrollbarRange(FrameCount(32));

        const std::string kHave = QImageToString(view.GenerateSpectrogramImage(6, 4));
        const std::string kWant = "\n"
                                  "010001 020002 030003 040004 050005 000000 \n"
                                  "090009 0A000A 0B000B 0C000C 0D000D 000000 \n"
                                  "000000 000000 000000 000000 000000 000000 \n"
                                  "000000 000000 000000 000000 000000 000000 \n";

        REQUIRE(kHave == kWant);
    }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity) macro expansion
TEST_CASE("Benchmark", "[spectrogram_view][!benchmark]")
{
    Settings settings;
    const AudioBuffer audioBuffer; // No data loaded, so GetRows will return all zeros
    SpectrogramController controller(settings, audioBuffer);
    SpectrogramView view(controller);

    SECTION("GenerateSpectrogramImage performance")
    {
        // Simple performance benchmark to ensure image generation is efficient.
        settings.SetFFTSettings(2048, FFTWindow::Type::Rectangular);

        // No data is loaded in the audio buffer, so GetRows will return zeroed
        // data.  Let's verify that assumption first.
        SECTION("Verify zeroed data assumption")
        {
            const auto have = controller.GetRows(0, FramePosition{ 0 }, 10);
            REQUIRE(have.size() == 10);
            for (const auto& row : have) {
                REQUIRE(row.size() == (settings.GetFFTSize() / 2) + 1);
                for (const auto& value : row) {
                    REQUIRE(value == 0.0f);
                }
            }
        }

        // Let's also see how fast GetRows is because it will affect the overall
        // image generation time.
        BENCHMARK("SpectrogramController::GetRows 1024 rows")
        {
            return controller.GetRows(0, FramePosition{ 0 }, 1024);
        };

        BENCHMARK("GenerateSpectrogramImage 800x600")
        {
            return view.GenerateSpectrogramImage(800, 600);
        };

        BENCHMARK("GenerateSpectrogramImage 1920x1080")
        {
            return view.GenerateSpectrogramImage(1920, 1080);
        };
    }
}

TEST_CASE("SpectrogramView::GetRenderConfig", "[spectrogram_view]")
{
    Settings settings;
    const AudioBuffer audioBuffer;
    const SpectrogramController controller(settings, audioBuffer);
    SpectrogramView view(controller);

    SECTION("returns correct RenderConfig values")
    {

        settings.SetApertureMinDecibels(-100.0f);
        settings.SetApertureMaxDecibels(0.0f);
        settings.SetFFTSettings(2048, FFTWindow::Type::Hann);
        settings.SetWindowScale(2); // stride = 1024

        constexpr size_t height = 256;
        const size_t kPageStep = height * settings.GetWindowStride();

        RenderConfig want{
            .channels = 2,
            .stride = 1024,
            .top_frame = FramePosition{ 0 },
            .min_decibels = -100.0f,
            .max_decibels = 0.0f,
            .decibel_range = 100.0f,
            .inverse_decibel_range = 2.55f,
            .color_map_lut = settings.GetColorMapLUTs(),
        };

        SECTION("with zero data")
        {
            // Zero data, bottom is at frame 0, top is back one pagestep
            want.top_frame = FramePosition{ static_cast<long>(-kPageStep) };
            const RenderConfig have = view.GetRenderConfig(height);
            REQUIRE(ToString(have) == ToString(want));
        }

        SECTION("with one page of data")
        {
            view.UpdateScrollbarRange(FrameCount(kPageStep));

            want.top_frame = FramePosition{ 0 };
            const RenderConfig have = view.GetRenderConfig(height);
            REQUIRE(ToString(have) == ToString(want));
        }

        SECTION("with a page and a half of data")
        {
            const FrameCount kTotalFrames{ 0x60000 };
            view.UpdateScrollbarRange(kTotalFrames);

            want.top_frame = FramePosition{ 0x20000 };
            const RenderConfig have = view.GetRenderConfig(height);
            REQUIRE(ToString(have) == ToString(want));
        }

        SECTION("with a page and a half of data plus a bit more")
        {
            const FrameCount kTotalFrames{ 0x60000 + 1023 }; // just shy of one stride
            view.UpdateScrollbarRange(kTotalFrames);

            want.top_frame = FramePosition{ 0x20000 };
            const RenderConfig have = view.GetRenderConfig(height);
            REQUIRE(ToString(have) == ToString(want));
        }
    }

    SECTION("throws on invalid channel count")
    {
        // mock controller to return invalid channel count
        class MockController : public SpectrogramController
        {
          public:
            using SpectrogramController::SpectrogramController;
            ChannelCount GetChannelCount() const override { return GENERATE(0, GKMaxChannels + 1); }
        };
        const MockController mockController(settings, audioBuffer);
        const SpectrogramView mockView(mockController);
        REQUIRE_THROWS_MATCHES(mockView.GetRenderConfig(256),
                               std::runtime_error,
                               MessageMatches(ContainsSubstring("out of range")));
    }
}

TEST_CASE("SpectrogramView scrollbar integration", "[spectrogram_view]")
{
    Settings settings;
    const AudioBuffer audioBuffer;
    const SpectrogramController controller(settings, audioBuffer);
    TestableSpectrogramView view(controller);

    // Set up FFT settings with known stride
    settings.SetFFTSettings(2048, FFTWindow::Type::Hann);
    settings.SetWindowScale(2); // stride = 1024

    auto* scrollBar = view.findChild<QScrollBar*>("SpectrogramViewVerticalScrollBar");
    REQUIRE(scrollBar != nullptr);

    // MainWindow connects scrollbar to Settings::ClearLiveMode, simulate that here
    QObject::connect(scrollBar, &QScrollBar::actionTriggered, &settings, &Settings::ClearLiveMode);

    // Compute the page step in frames.  This offset is relevant in scrollbar tests
    // where we need to account for the page step in the maximum value.
    const int kPageStepFrames = view.GetViewportHeight() * settings.GetWindowStride();

    SECTION("scrollbar is initialized on construction")
    {
        CHECK(scrollBar->orientation() == Qt::Vertical);
        CHECK(scrollBar->minimum() == 0);
        CHECK(scrollBar->maximum() == 0);
        CHECK(scrollBar->value() == 0);
    }

    SECTION("throws when pagestep would exceed int max")
    {
        // Stride is 1024.
        // Set viewport height to a large but valid value.
        constexpr size_t kMaxAllowedHeight = (1 << 21) - 1;
        view.SetTestHeight(kMaxAllowedHeight);
        // This makes page step:
        constexpr int kExpectedPageStep = (1LL << 31) - 1024;

        REQUIRE_NOTHROW(view.UpdateScrollbarRange(FrameCount{ 1 }));
        CHECK(scrollBar->pageStep() == kExpectedPageStep);

        // Now increase height by 1 to cause overflow.
        view.SetTestHeight(kMaxAllowedHeight + 1);

        REQUIRE_THROWS_MATCHES(
          view.UpdateScrollbarRange(FrameCount{ 1 }),
          std::overflow_error,
          MessageMatches(ContainsSubstring("scroll page step exceeds int max")));
    }

    SECTION("throws when overflowing scrollbar maximum")
    {
        const int64_t kMaxAllowed = (1LL << 31) - kPageStepFrames - 1;
        REQUIRE_NOTHROW(view.UpdateScrollbarRange(FrameCount(kMaxAllowed)));

        const int64_t kTooLarge = kMaxAllowed + 1;
        REQUIRE_THROWS_MATCHES(
          view.UpdateScrollbarRange(FrameCount(kTooLarge)),
          std::overflow_error,
          MessageMatches(ContainsSubstring("Count(2147483648) exceeds int max")));
    }

    SECTION("UpdateScrollbarRange updates scrollbar maximum")
    {
        view.UpdateScrollbarRange(FrameCount(10000));
        REQUIRE(scrollBar->maximum() == 10000 + kPageStepFrames);
    }

    SECTION("UpdateScrollbarRange follows live data when in live mode")
    {
        settings.SetLiveMode(true);
        // Set initial range and position at max (live mode)
        view.UpdateScrollbarRange(FrameCount(10000));
        scrollBar->setValue(scrollBar->maximum());

        // Add more data
        view.UpdateScrollbarRange(FrameCount(20000));

        // Position should follow to new maximum (live mode)
        REQUIRE(scrollBar->value() == 20000);
        REQUIRE(scrollBar->maximum() == 20000 + kPageStepFrames);

        // Clear live mode
        settings.SetLiveMode(false);

        // Add more data again
        view.UpdateScrollbarRange(FrameCount(30000));

        // Position should be preserved (not live mode)
        REQUIRE(scrollBar->value() == 20000);
        REQUIRE(scrollBar->maximum() == 30000 + kPageStepFrames);
    }

    SECTION("UpdateScrollbarRange emits valueChanged signal to trigger repaint")
    {
        // We can't directly test paintEvent, but we can at least verify that
        // the scrollbar's valueChanged signal is emitted.
        QSignalSpy spy(scrollBar, &QScrollBar::valueChanged);
        view.UpdateScrollbarRange(FrameCount(10000));
        REQUIRE(spy.count() == 1);
        REQUIRE(spy.takeFirst().takeFirst().toInt() == 10000);
    }

    SECTION("Scrollbar action clears live mode in settings")
    {
        settings.SetLiveMode(true);
        REQUIRE(settings.IsLiveMode() == true);

        scrollBar->triggerAction(QAbstractSlider::SliderSingleStepAdd);

        // Live mode should be cleared
        REQUIRE(settings.IsLiveMode() == false);
    }
}