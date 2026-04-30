#ifndef ELECTRONICDEVICEWINDOW_H
#define ELECTRONICDEVICEWINDOW_H

#include <QWidget>
#include <QByteArray>
#include <QStringList>

// 前向声明 Calc 类（避免头文件依赖暴露到外部）
class Calc;

namespace Ui {
class ElectronicDeviceWindow;
}

class ElectronicDeviceWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ElectronicDeviceWindow(QWidget *parent = nullptr);
    ~ElectronicDeviceWindow();

    /**
     * @brief 加载遥测解码配置 CSV 文件（通用单文件接口）
     * @param csvPath CSV 配置文件路径
     * @return 是否加载成功
     */
    bool loadDecodeConfig(const QString &csvPath);

    /**
     * @brief 启动时自动加载默认配置文件
     *
     * 自动从 D:\calc_nom.csv 和 D:\calc_dir.csv 加载两套解码配置，
     * 在 MainWindow 创建此窗口后立即调用。
     */
    void autoLoadDefaultConfigs();

public slots:
    /**
     * @brief 解码并显示遥测数据（使用两套配置同时解码）
     * @param payload 原始遥测帧字节数据
     */
    void decodeTelemetry(const QByteArray &payload);

private:
    Ui::ElectronicDeviceWindow *ui;

    // ========== 遥测解码（两套配置） ==========
    Calc *m_calcNom;     // calc_nom.csv 解码器
    Calc *m_calcDir;     // calc_dir.csv 解码器
};

#endif // ELECTRONICDEVICEWINDOW_H
