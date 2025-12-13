#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class AudioBuffer;
class AudioRecorder;
class ConfigPanel;
class SpectrogramController;
class SpectrogramView;
class SpectrumPlot;

/**
 * @brief Main application window for Spectro-v3 spectrum analyzer
 *
 * Manages the application's UI layout, menus, and coordinate interaction between
 * the audio processing components and visualization widgets.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    /**
     * @brief Constructor for MainWindow
     * @param parent Optional parent widget
     */
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

  private:
    /**
     * @brief Sets up the main layout of the application window
     */
    void createLayout();

    /**
     * @brief Sets up signal-slot connections between components
     */
    void setupConnections();

    // Models and controllers
    AudioBuffer* mAudioBuffer = nullptr;
    SpectrogramController* mSpectrogramController = nullptr;
    AudioRecorder* mAudioRecorder = nullptr;

    // View widgets
    SpectrogramView* mSpectrogramView = nullptr;
    SpectrumPlot* mSpectrumPlot = nullptr;
    ConfigPanel* mConfigPanel = nullptr;
};

#endif // MAINWINDOW_H
