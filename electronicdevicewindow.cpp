#include "electronicdevicewindow.h"
#include "ui_electronicdevicewindow.h"

ElectronicDeviceWindow::ElectronicDeviceWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ElectronicDeviceWindow)
{
    ui->setupUi(this);
}

ElectronicDeviceWindow::~ElectronicDeviceWindow()
{
    delete ui;
}