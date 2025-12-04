#pragma once

#include <QMainWindow>

namespace spectro {

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

  private:
    void setupUi();
};

} // namespace spectro
