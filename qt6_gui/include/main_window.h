#pragma once

#include <QMainWindow>

namespace spectro_qt6 {

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

  private:
    void setupUi();
};

} // namespace spectro_qt6
