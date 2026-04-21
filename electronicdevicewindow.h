#ifndef ELECTRONICDEVICEWINDOW_H
#define ELECTRONICDEVICEWINDOW_H

#include <QWidget>

namespace Ui {
class ElectronicDeviceWindow;
}

class ElectronicDeviceWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ElectronicDeviceWindow(QWidget *parent = nullptr);
    ~ElectronicDeviceWindow();

private:
    Ui::ElectronicDeviceWindow *ui;
};

#endif // ELECTRONICDEVICEWINDOW_H