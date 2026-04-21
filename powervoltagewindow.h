#ifndef POWERVOLTAGEWINDOW_H
#define POWERVOLTAGEWINDOW_H

#include <QWidget>

namespace Ui {
class PowerVoltageWindow;
}

class PowerVoltageWindow : public QWidget
{
    Q_OBJECT

public:
    explicit PowerVoltageWindow(QWidget *parent = nullptr);
    ~PowerVoltageWindow();

private:
    Ui::PowerVoltageWindow *ui;
};

#endif // POWERVOLTAGEWINDOW_H