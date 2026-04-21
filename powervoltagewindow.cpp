#include "powervoltagewindow.h"
#include "ui_powervoltagewindow.h"

PowerVoltageWindow::PowerVoltageWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PowerVoltageWindow)
{
    ui->setupUi(this);
}

PowerVoltageWindow::~PowerVoltageWindow()
{
    delete ui;
}