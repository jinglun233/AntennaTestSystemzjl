#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QComboBox>
#include <QVector>
#include <QMap>
#include <memory>          // std::shared_ptr

// 新增：数据收发
#include "ringbuffer.h"
#include "dataprotocol.h"
#include "antennadevicewindow.h"
#include "electronicdevicewindow.h"
#include "temperatureinfowindow.h"
#include "powervoltagewindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @brief 工作模式枚举
 */
enum class WorkMode {
    None = 0,           // 未选择模式
    AntennaGroundTest,  // 单独天线地检模式（允许连接1个客户端）
    SimulateDevice      // 模拟电子设备模式（允许连接2个客户端）
};

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

    // ========== TCP 服务器相关 ==========
    QTcpServer *m_tcpServer;
    QVector<QTcpSocket*> m_clients;
    int m_maxClients;
    bool m_serverRunning;

    // ========== 数据收发相关 ==========
    /**
     * @brief 每个客户端对应一个环形缓冲区
     *
     * key = QTcpSocket* (客户端socket指针)
     * value = 该客户端的接收缓冲区
     */
    QMap<QTcpSocket*, std::shared_ptr<RingBuffer>> m_clientBuffers;

    // ========== 设备子窗口 ==========
    AntennaDeviceWindow *m_antennaDeviceWindow;
    ElectronicDeviceWindow *electronicTab;
    TemperatureInfoWindow *temperatureTab;
    PowerVoltageWindow *powerTab;

private slots:
    void updateDateTime();
    void onConfirmModeClicked();
    void onCancelModeClicked();
    void onStartServerClicked();
    void onStopServerClicked();
    void onNewConnection();
    void onClientDisconnected();
    void onClientDataReady();          // TCP 数据到达 → 写入环形缓冲 → 尝试解析帧
    void onSendRawCommand(const QByteArray &command);  // 接收子窗口原始指令，广播给所有客户端
    void onPeriodicCommandTimer();                     // 周期性指令定时器回调（1s）

private:
    // ========== UI 辅助方法 ==========
    void updateUIState();
    void updateModeStatusLabel();
    int getMaxClientsForMode(WorkMode mode) const;
    void appendLog(const QString &msg);

    // ========== 数据收发核心方法 ==========

    /**
     * @brief 向指定客户端发送一帧完整数据（自动打包+校验）
     * @param client 目标客户端 socket
     * @param command 指令码（Protocol::Cmd::*）
     * @param payload 数据域
     * @return true=发送成功
     */
    bool sendFrame(QTcpSocket *client, quint8 command, const QByteArray &payload);

    /**
     * @brief 向所有已连接客户端广播发送一帧数据
     * @param command 指令码
     * @param payload 数据域
     */
    void broadcastFrame(quint8 command, const QByteArray &payload);

    /**
     * @brief 处理从环形缓冲中解析出的完整数据帧
     *
     * 此方法是"业务层入口"，每解出一帧就会调用一次。
     * 根据命令码分发到具体处理函数。
     *
     * @param client 来源客户端
     * @param frame  解析完成的帧
     */
    void handleReceivedFrame(QTcpSocket *client, const DataFrame &frame);

    // ========== 业务处理（可按需扩展） ==========
    void handleHeartbeat(QTcpSocket *client, const DataFrame &frame);      // 心跳包
    void handleTelemetryData(QTcpSocket *client, const DataFrame &frame);   // 遥测数据上传
    void handleControlAck(QTcpSocket *client, const DataFrame &frame);      // 遥控应答
    void handleDeviceStatus(QTcpSocket *client, const DataFrame &frame);    // 设备状态上报
    void handleErrorReport(QTcpSocket *client, const DataFrame &frame);     // 错误报告

private:
    // ========== 内部辅助（日志标签） ==========
    QString clientPeerLabel(QTcpSocket *client);  // 客户端地址标签
    QString commandName(quint8 cmd);                // 指令码可读名称
};

#endif // MAINWINDOW_H
