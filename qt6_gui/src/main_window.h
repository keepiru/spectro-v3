#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QLabel;

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
};

#endif // MAINWINDOW_H
