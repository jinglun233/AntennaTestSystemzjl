#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QComboBox>
#include <memory>

// 网络管理层（统一封装 TCP Server / Client / Instru）
#include "networkmanager.h"

// 数据收发
#include "ringbuffer.h"
#include "dataprotocol.h"

// 子窗口
#include "antennadevicewindow.h"
#include "electronicdevicewindow.h"
#include "temperatureinfowindow.h"
#include "powervoltagewindow.h"
#include "autotestwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @brief 工作模式枚举
 */
enum class WorkMode {
    None = 0,           // 未选择模式
    AntennaGroundTest,  // 单独天线地检模式（客户端模式，连接外部服务器）
    SimulateDevice      // 模拟电子设备模式（服务器+客户端双模式）
};

/**
 * @brief 主窗口
 *
 * 职责：
 *   1. UI 主界面控制器（工作模式、状态栏、菜单栏）
 *   2. 通过 NetworkManager 管理 TCP 服务器/客户端连接
 *   3. 数据帧解析与业务分发
 *   4. 子窗口生命周期管理（Tab 子窗口 + 菜单弹窗子窗口）
 *
 * Socket 生命周期完全委托给 NetworkManager，MainWindow 不直接操作 QTcpSocket。
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    // ========== 时钟相关 ==========
    QTimer *m_dateTimeTimer;
    QLabel *m_timeLabel;

    // ========== 周期性指令定时器 ==========
    QTimer *m_periodicCommandTimer;   // 1秒周期定时器
    QString m_targetClientIP;         // 需要接收周期指令的目标客户端IP

    // ========== 工作模式相关 ==========
    WorkMode m_currentMode;
    QLabel *m_modeStatusLabel;

    // ★★★ 核心变更：统一网络管理器（替代所有原始 QTcpSocket）★★★
    NetworkManager *m_network;

    // ========== 数据收发相关 ==========
    /**
     * @brief 每个服务端客户端对应一个环形缓冲区（用于帧解析）
     *
     * key = 客户端ID（int），value = 该客户端的接收缓冲区
     */
    QMap<int, std::shared_ptr<RingBuffer>> m_clientBuffers;

    // ========== 设备子窗口（Tab 页，有 parent = this） ==========
    AntennaDeviceWindow   *m_antennaDeviceWindow;
    ElectronicDeviceWindow *electronicTab;
    TemperatureInfoWindow *temperatureTab;
    PowerVoltageWindow     *powerTab;

    // ========== 菜单弹窗子窗口（有 parent = this） ==========
    AutoTestWindow        *m_autoTestWindow;

private slots:
    void updateDateTime();
    void onConfirmModeClicked();
    void onCancelModeClicked();
    void onStartServerClicked();
    void onStopServerClicked();

    // 原始控制指令发送（来自子窗口信号）
    void onSendRawCommand(const QByteArray &command);
    // 周期性指令定时器回调（1s）
    void onPeriodicCommandTimer();

    // ========== NetworkManager 事件转发 ==========
    void onServerStarted();                          // 服务器启动成功
    void onServerStopped();                          // 服务器已停止
    void onServerClientConnected(const ConnectionHandle &h);   // 服务端新客户端接入
    void onServerClientDisconnected(const ConnectionHandle &h);// 服务端客户端断开
    void onServerDataReceived(const ConnectionHandle &h, const QByteArray &data);

    void onMainClientConnected();                    // 主界面客户端连接成功
    void onMainClientDisconnected();                 // 主界面客户端断开
    void onMainClientDataReceived(const QByteArray &data);

    void onInstruConnected();                        // 仪器连接成功
    void onInstruDisconnected();                     // 仪器断开
    void onInstruDataReceived(const QByteArray &data);

    void onNetworkError(const QString &error);       // 网络错误
    void onNetworkLog(const QString &msg);           // 网络日志

    // ========== 菜单栏槽函数 ==========
    void onActionAutoTestWindow();
    void onActionPatternSimulation();
    void onActionExit();
    void onActionDataReplay();
    void onActionFirmwareUpgrade();
    void onActionNetworkConfig();
    void onActionDataPathConfig();
    void onActionAbout();

private:
    // ========== UI 辅助方法 ==========
    void updateUIState();
    void updateModeStatusLabel();
    int getMaxClientsForMode(WorkMode mode) const;
    void appendLog(const QString &msg);

    // ========== 数据收发核心方法 ==========
    bool sendFrame(int clientId, quint8 command, const QByteArray &payload);
    void broadcastFrame(quint8 command, const QByteArray &payload);
    void handleReceivedFrame(int clientId, const DataFrame &frame);

    // ========== 业务处理 ==========
    void handleHeartbeat(int clientId, const DataFrame &frame);
    void handleTelemetryData(int clientId, const DataFrame &frame);
    void handleControlAck(int clientId, const DataFrame &frame);
    void handleDeviceStatus(int clientId, const DataFrame &frame);
    void handleErrorReport(int clientId, const DataFrame &frame);

    // ========== 内部辅助 ==========
    QString clientPeerLabel(int clientId);
    QString commandName(quint8 cmd);
};

#endif // MAINWINDOW_H
