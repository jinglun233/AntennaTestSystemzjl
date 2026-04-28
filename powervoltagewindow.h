#ifndef POWERVOLTAGEWINDOW_H
#define POWERVOLTAGEWINDOW_H

#include <QWidget>

namespace Ui {
class PowerVoltageWindow;
}

/**
 * @brief 电源电压窗口（含浏览器入口）
 */
class PowerVoltageWindow : public QWidget
{
    Q_OBJECT

public:
    explicit PowerVoltageWindow(QWidget *parent = nullptr);
    ~PowerVoltageWindow();

private slots:
    void on_openBrowserButton_clicked();   // 打开系统浏览器

private:
    Ui::PowerVoltageWindow *ui;
};

#endif // POWERVOLTAGEWINDOW_H