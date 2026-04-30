#ifndef POWERVOLTAGEWINDOW_H
#define POWERVOLTAGEWINDOW_H

#include <QWidget>

// 前向声明
class QAxWidget;
class QLineEdit;

namespace Ui {
class powervoltagewindow;   // Qt UIC 生成的是小写名（与 ui_*.h 一致）
}

/**
 * @brief 电源电压窗口（含内嵌浏览器）
 */
class PowerVoltageWindow : public QWidget
{
    Q_OBJECT

public:
    explicit PowerVoltageWindow(QWidget *parent = nullptr);
    ~PowerVoltageWindow();

private slots:
    void on_navigateButton_clicked();   // 在嵌入式浏览器中导航到指定网址
    void onBrowserStatusTextChange(const QString &text);  // 拦截IE状态栏消息（吞掉）

private:
    void setIEEmulationMode();            // 设置IE11仿真模式
    void initBrowser();                   // 初始化嵌入浏览器

    Ui::powervoltagewindow *ui;       // 必须与 ui_*.h 生成类名一致（小写）
    QAxWidget *m_browser;               // 内嵌 IE/Edge 浏览器控件
    QLineEdit *m_urlEdit;               // 地址栏
};

#endif // POWERVOLTAGEWINDOW_H
