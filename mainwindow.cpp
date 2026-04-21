#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "antennadevicewindow.h"
#include <QTime>
#include <QDateTime>
#include <QMessageBox>
#include <QHostAddress>
#include <QTextCursor>
#include <memory>

// ============================================================================
//                              构造 / 析构
// ============================================================================

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // ========== 成员初始化 ==========
    m_currentMode = WorkMode::None;
    m_serverRunning = false;
    m_tcpServer = nullptr;
    m_maxClients = 0;

    // ========== 周期性指令定时器初始化（1秒周期，不自动启动） ==========
    m_periodicCommandTimer = new QTimer(this);
    connect(m_periodicCommandTimer, &QTimer::timeout, this, &MainWindow::onPeriodicCommandTimer);

    // ========== 创建设备子窗口 ==========
    m_antennaDeviceWindow = new AntennaDeviceWindow();
    electronicTab = new ElectronicDeviceWindow();
    temperatureTab = new TemperatureInfoWindow();
    powerTab = new PowerVoltageWindow();

    // 在 addTab 之前记录每个 widget 的 UI 设计器原始几何尺寸
    // （addTab 后布局会覆盖 widget 原始尺寸，导致分离时无法恢复）
    ui->telemetryTabWidget->recordDesignSize(m_antennaDeviceWindow, m_antennaDeviceWindow->size());
    ui->telemetryTabWidget->recordDesignSize(temperatureTab, temperatureTab->size());
    ui->telemetryTabWidget->recordDesignSize(powerTab, powerTab->size());
    ui->telemetryTabWidget->recordDesignSize(electronicTab, electronicTab->size());

    ui->telemetryTabWidget->addTab(m_antennaDeviceWindow, "天线设备界面");
    ui->telemetryTabWidget->addTab(temperatureTab, "天线温度界面");
    ui->telemetryTabWidget->addTab(powerTab, "电源电压界面");
    ui->telemetryTabWidget->addTab(electronicTab, "电子设备界面");

    ui->telemetryTabWidget->replaceTabBar();


    // ========== 状态栏三段布局 ==========
    m_modeStatusLabel = new QLabel(this);
    m_modeStatusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    QFont statusFont = m_modeStatusLabel->font();
    statusFont.setPointSize(9);
    m_modeStatusLabel->setFont(statusFont);
    m_modeStatusLabel->setText("当前模式：未选择");
    ui->statusbar->addWidget(m_modeStatusLabel, 1);

    ui->statusbar->addWidget([](){
        QLabel* lab = new QLabel("Copyright @2025 齐鲁空天信息研究院，版权所有 All Rights Reserved");
        lab->setAlignment(Qt::AlignCenter);
        return lab;
    }(), 1);

    m_timeLabel = new QLabel(this);
    m_timeLabel->setAlignment(Qt::AlignRight);
    ui->statusbar->addWidget(m_timeLabel, 1);

    // ========== 时钟定时器 ==========
    m_dateTimeTimer = new QTimer(this);
    connect(m_dateTimeTimer, &QTimer::timeout, this, &MainWindow::updateDateTime);
    updateDateTime();
    m_dateTimeTimer->start(1000);

    // ========== 按钮信号槽连接 ==========
    connect(ui->confirmModeButton, &QPushButton::clicked, this, &MainWindow::onConfirmModeClicked);
    connect(ui->cancelModeButton, &QPushButton::clicked, this, &MainWindow::onCancelModeClicked);
    connect(ui->startServerButton, &QPushButton::clicked, this, &MainWindow::onStartServerClicked);
    connect(ui->stopServerButton, &QPushButton::clicked, this, &MainWindow::onStopServerClicked);

    // ========== 子窗口信号槽连接 ==========
    connect(m_antennaDeviceWindow, &AntennaDeviceWindow::sendRawCommandToServer,
            this,      &MainWindow::onSendRawCommand);
    connect(temperatureTab,       &TemperatureInfoWindow::sendRawCommandToServer,
            this,                 &MainWindow::onSendRawCommand);

    updateUIState();
}

MainWindow::~MainWindow()
{
    if (m_serverRunning) {
        for (QTcpSocket *sock : m_clients) {
            sock->disconnectFromHost();
        }
        if (m_tcpServer && m_tcpServer->isListening()) {
            m_tcpServer->close();
        }
    }
    qDeleteAll(m_clients);
    delete m_tcpServer;
    delete ui;
}

// ============================================================================
//                          UI 状态更新
// ============================================================================

void MainWindow::updateUIState()
{
    bool modeSelected = (m_currentMode != WorkMode::None);

    ui->modeComboBox->setEnabled(!m_serverRunning);
    ui->confirmModeButton->setEnabled(!modeSelected && !m_serverRunning);
    ui->cancelModeButton->setEnabled(modeSelected && !m_serverRunning);

    bool serverControlsEnabled = modeSelected && !m_serverRunning;
    ui->ipLineEdit->setEnabled(serverControlsEnabled);
    ui->portSpinBox->setEnabled(serverControlsEnabled);
    ui->startServerButton->setEnabled(modeSelected && !m_serverRunning);
    ui->stopServerButton->setEnabled(m_serverRunning);

    updateModeStatusLabel();
}

void MainWindow::updateModeStatusLabel()
{
    QString text;
    QString style;

    switch (m_currentMode) {
    case WorkMode::None:
        text = "当前模式：未选择";
        style = "color: #888888; padding-left: 4px;";
        break;
    case WorkMode::AntennaGroundTest:
        text = "当前模式：单独天线地检模式";
        style = "color: #28a745; padding-left: 4px;";
        break;
    case WorkMode::SimulateDevice:
        text = "当前模式：模拟电子设备模式";
        style = "color: #fd7e14; padding-left: 4px;";
        break;
    }

    m_modeStatusLabel->setText(text);
    m_modeStatusLabel->setStyleSheet(style);
}

int MainWindow::getMaxClientsForMode(WorkMode mode) const
{
    switch (mode) {
    case WorkMode::AntennaGroundTest: return 1;
    case WorkMode::SimulateDevice:     return 2;
    default:                           return 0;
    }
}

// ============================================================================
//                        工作模式控制
// ============================================================================

void MainWindow::onConfirmModeClicked()
{
    int idx = ui->modeComboBox->currentIndex();

    if (idx == 0) {
        m_currentMode = WorkMode::AntennaGroundTest;
        appendLog("工作模式：【单独天线地检模式】（最多连接 1 个客户端）");
    } else if (idx == 1) {
        m_currentMode = WorkMode::SimulateDevice;
        appendLog("工作模式：【模拟电子设备模式】（最多连接 2 个客户端）");
    }

    m_maxClients = getMaxClientsForMode(m_currentMode);
    updateUIState();
}

void MainWindow::onCancelModeClicked()
{
    if (m_serverRunning) {
        onStopServerClicked();
        appendLog("已自动停止服务器。");
    }

    m_currentMode = WorkMode::None;
    m_maxClients = 0;

    appendLog("已取消工作模式。");
    updateUIState();
}

// ============================================================================
//                         TCP 服务器管理
// ============================================================================

void MainWindow::onStartServerClicked()
{
    QString ip = ui->ipLineEdit->text().trimmed();
    quint16 port = static_cast<quint16>(ui->portSpinBox->value());

    if (ip.isEmpty()) {
        QMessageBox::warning(this, "错误", "请输入监听 IP 地址！");
        return;
    }

    QHostAddress addr(ip);
    if (addr.isNull()) {
        QMessageBox::warning(this, "错误", "IP 地址格式无效！");
        return;
    }

    if (!m_tcpServer) {
        m_tcpServer = new QTcpServer(this);
        connect(m_tcpServer, &QTcpServer::newConnection, this, &MainWindow::onNewConnection);
    }

    if (!m_tcpServer->listen(addr, port)) {
        QMessageBox::critical(this, "错误",
                              QString("无法启动服务！\n%1\n请检查端口是否被占用。")
                              .arg(m_tcpServer->errorString()));
        appendLog(QString("[ERROR] 服务启动失败：%1").arg(m_tcpServer->errorString()));
        return;
    }

    m_serverRunning = true;
    updateUIState();

    // 保存目标客户端IP（即监听IP，用于周期性指令定向发送）
    m_targetClientIP = "127.0.0.1";

    // 启动周期性指令定时器（1秒间隔）
    if (!m_periodicCommandTimer->isActive()) {
        m_periodicCommandTimer->start(1000);
    }

    appendLog(QString("[成功] 服务已启动，监听 %1:%2").arg(ip).arg(port));
//    appendLog(QString("--- 周期性指令已启动（1s），目标IP: %1 ---").arg(m_targetClientIP));
    appendLog(QString("--- 等待客户端连接 ---").arg(m_maxClients));
}

void MainWindow::onStopServerClicked()
{
    if (!m_serverRunning) return;

    // 停止周期性指令定时器
    if (m_periodicCommandTimer->isActive()) {
        m_periodicCommandTimer->stop();
    }

    for (QTcpSocket *sock : m_clients) {
        sock->disconnectFromHost();
    }
    m_clients.clear();
    m_clientBuffers.clear();   // 清空所有环形缓冲区

    if (m_tcpServer && m_tcpServer->isListening()) {
        m_tcpServer->close();
    }

    m_serverRunning = false;
    m_targetClientIP.clear();
    updateUIState();
    appendLog("--- 已手动停止服务器（周期性指令已停止） ---");
}

// ============================================================================
//                     TCP 客户端连接/断开
// ============================================================================

void MainWindow::onNewConnection()
{
    while (m_tcpServer->hasPendingConnections()) {
        // 检查连接上限
        if (static_cast<int>(m_clients.size()) >= m_maxClients) {
            QTcpSocket *rejected = m_tcpServer->nextPendingConnection();
            QString peerInfo = QString("%1:%2")
                               .arg(rejected->peerAddress().toString())
                               .arg(rejected->peerPort());
            appendLog(QString("[拒绝] 客户端 %1 尝试连接，已达上限（%2/%3）")
                      .arg(peerInfo).arg(m_clients.size()).arg(m_maxClients));
            rejected->disconnectFromHost();
            rejected->deleteLater();
            continue;
        }

        // 接受新连接
        QTcpSocket *clientSocket = m_tcpServer->nextPendingConnection();
        QString clientInfo = QString("%1:%2")
                             .arg(clientSocket->peerAddress().toString())
                             .arg(clientSocket->peerPort());
        int clientId = static_cast<int>(m_clients.size()) + 1;

        m_clients.append(clientSocket);

        // 为该客户端创建独立的环形缓冲区（初始容量 128KB）
        auto buffer = std::make_shared<RingBuffer>(128 * 1024);  // 128KB
        m_clientBuffers.insert(clientSocket, buffer);

        appendLog(QString("[连接] 客户端 #%1 已接入：%2（当前 %3/%4）")
                  .arg(clientId).arg(clientInfo).arg(m_clients.size()).arg(m_maxClients));

        // 连接断开和数据到达信号
        connect(clientSocket, &QTcpSocket::disconnected, this, &MainWindow::onClientDisconnected);
        connect(clientSocket, &QTcpSocket::readyRead, this, &MainWindow::onClientDataReady);
    }
}

void MainWindow::onClientDisconnected()
{
    QTcpSocket *sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock) return;

    QString info = QString("%1:%2")
                   .arg(sock->peerAddress().toString())
                   .arg(sock->peerPort());

    int idx = m_clients.indexOf(sock);
    if (idx >= 0) {
        m_clients.removeAt(idx);
    }

    // 移除该客户端的环形缓冲区
    m_clientBuffers.remove(sock);

    sock->deleteLater();

    appendLog(QString("[断开] 客户端 #%1 %2 已离开（剩余 %3/%4）")
              .arg(idx + 1).arg(info).arg(m_clients.size()).arg(m_maxClients));
}

// ============================================================================
//
//                       ★ 数据接收核心 ★
//                   TCP → 环形缓冲 → 帧解析 → 业务处理
//
// ============================================================================

/**
 * @brief TCP 有数据可读时的回调
 *
 * 流程：
 *   1. 从 socket 读出所有原始字节
 *   2. 写入该客户端对应的 RingBuffer
 *   3. 循环尝试从 RingBuffer 中解析完整帧
 *   4. 每解出一帧，调用 handleReceivedFrame() 分发到业务处理
 */
void MainWindow::onClientDataReady()
{
    QTcpSocket *sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock || !m_clientBuffers.contains(sock)) return;

    // ====== 步骤1：读取 socket 所有可用数据 ======
    QByteArray rawData = sock->readAll();
    if (rawData.isEmpty()) return;

    // ====== 步骤2：写入环形缓冲区 ======
    auto &ringBuf = m_clientBuffers[sock];
    size_t written = ringBuf->write(rawData);
    Q_UNUSED(written);

    // ====== 步骤3+4：循环解析完整帧 ======
    while (true) {
        // 检查是否有足够数据构成一完整帧（1029字节）
        size_t available = ringBuf->readableBytes();
        if (!ProtocolCodec::hasCompleteFrame(static_cast<int>(available))) {
            break; // 数据不够一整帧，等下次数据到来
        }

        // 预览足够多的数据用于帧搜索和解析
        // 一次预览最大 64KB，防止一次性拷贝过多内存
        size_t peekSize = std::min(available, static_cast<size_t>(65536));
        QByteArray preview = ringBuf->peek(peekSize);
        if (preview.isEmpty()) break;

        DataFrame frame;
        int consumed = 0;

        bool parsed = ProtocolCodec::parse(preview, frame, consumed);

        if (!parsed) {
            // 解析失败的原因：
            //   - consumed > 0：跳过垃圾字节或无效帧头，继续循环重试
            //   - consumed == 0：数据不足完整帧，退出等待更多数据
            if (consumed > 0) {
                ringBuf->consume(static_cast<size_t>(consumed));
                continue; // 跳过后重新尝试解析
            } else {
                break; // 数据不足，退出循环
            }
        }

        // ====== 步骤5：解析成功！消费掉这帧的字节 ======
        ringBuf->consume(static_cast<size_t>(consumed));

        // ====== 步骤6：分发到业务层处理 ======
        handleReceivedFrame(sock, frame);
    }
}

// ============================================================================
//
//                       ★ 数据发送接口 ★
//
// ============================================================================

/**
 * @brief 向指定客户端发送一帧完整数据
 *
 * 自动完成：
 *   - 构建 DataFrame 结构体
 *   - 调用 ProtocolCodec::encode() 序列化（含帧头、长度、CRC校验）
 *   - 通过 socket 发送
 *
 * @return true=写入socket成功；false=发送失败或socket无效
 */
bool MainWindow::sendFrame(QTcpSocket *client, quint8 command, const QByteArray &payload)
{
    if (!client || client->state() != QTcpSocket::ConnectedState) {
        appendLog("[发送失败] 目标客户端未连接");
        return false;
    }

    DataFrame frame;
    frame.command = command;
    frame.payload = payload;

    QByteArray packet = ProtocolCodec::encode(frame);

    qint64 sent = client->write(packet);
    if (sent != packet.size()) {
        appendLog(QString("[发送异常] 只写出 %1/%2 字节").arg(sent).arg(packet.size()));
        return false;
    }

    client->flush(); // 尽快发出

    QString cmdName = commandName(command);
    appendLog(QString("[发送→%1] 命令=%2 数据长度=%3 字节")
              .arg(clientPeerLabel(client)).arg(cmdName).arg(payload.size()));

    return true;
}

/**
 * @brief 向所有已连接的客户端广播同一帧数据
 */
void MainWindow::broadcastFrame(quint8 command, const QByteArray &payload)
{
    for (QTcpSocket *client : m_clients) {
        sendFrame(client, command, payload);
    }
}

/**
 * @brief 接收子窗口发出的原始控制指令，直接广播给所有已连接客户端
 *
 * 用于发送短控制命令（如天线开关机），这些指令不需要经过协议帧编解码，
 * 直接将原始字节流写入 socket。
 */
void MainWindow::onSendRawCommand(const QByteArray &command)
{
    if (!m_serverRunning || m_clients.isEmpty()) {
        appendLog("[指令] 服务器未运行或无客户端连接，无法发送指令");
        return;
    }

    // 构造可读的十六进制字符串用于日志显示
    QString hexStr = command.toHex(' ').toUpper();

    for (QTcpSocket *client : m_clients) {
        if (!client || client->state() != QTcpSocket::ConnectedState) continue;

        qint64 sent = client->write(command);
        if (sent != command.size()) {
            appendLog(QString("[指令异常→%1] 只写出 %2/%3 字节")
                      .arg(clientPeerLabel(client)).arg(sent).arg(command.size()));
        } else {
            client->flush();
            appendLog(QString("[指令发送→%1] %2 字节 [%3]")
                      .arg(clientPeerLabel(client))
                      .arg(command.size())
                      .arg(hexStr));
        }
    }
}

// ============================================================================
//
//                      ★ 周期性指令（1秒）★
//
// ============================================================================

/**
 * @brief 1秒周期定时器回调：向特定IP的客户端发送周期指令 {0xEB, 0x90, 0x02, 0xC0, 0x05}
 *
 * 仅向 IP 匹配 m_targetClientIP 的客户端发送。
 * 如果目标客户端未连接或已断开，则跳过本次发送（不报错，静默处理）。
 */
void MainWindow::onPeriodicCommandTimer()
{
    if (!m_serverRunning || m_clients.isEmpty()) return;

    QByteArray periodicCmd = QByteArray::fromRawData("\xEB\x90\x02\xC0\x05", 5);

    for (QTcpSocket *client : m_clients) {
        // 仅向匹配目标IP的客户端发送
        if (client->peerAddress().toString() != m_targetClientIP) continue;
        if (!client || client->state() != QTcpSocket::ConnectedState) continue;

        qint64 sent = client->write(periodicCmd);
        client->flush();
        if (sent == periodicCmd.size()) {
            appendLog(QString("[周期指令→%1] [EB 90 02 C0 05]")
                      .arg(clientPeerLabel(client)));
        }
    }
}

// ============================================================================
//
//                      ★ 业务帧分发处理 ★
//
// ============================================================================

/**
 * @brief 根据命令码将收到的完整帧分发到对应的处理函数
 */
void MainWindow::handleReceivedFrame(QTcpSocket *client, const DataFrame &frame)
{
    QString src = clientPeerLabel(client);
    QString cmdName = commandName(frame.command);

    appendLog(QString("[接收←%1] 命令=%2 数据长度=%3 字节")
              .arg(src).arg(cmdName).arg(frame.payload.size()));

    switch (frame.command) {
    case Protocol::Cmd::TELEMETRY_DATA:
        handleTelemetryData(client, frame);
        break;

    default:
        appendLog(QString("[未知命令] 收到未知指令码 0x%1，来自 %2")
                  .arg(frame.command, 2, 16, QLatin1Char('0')).arg(src));
        break;
    }
}

// ============================================================================
//                       各类业务处理函数
// ============================================================================

/** 心跳包 — 自动回复心跳应答 */
void MainWindow::handleHeartbeat(QTcpSocket *client, const DataFrame &frame)
{
    Q_UNUSED(frame)

}

/** 遥测数据上传 — 接收客户端上报的遥测数据 */
void MainWindow::handleTelemetryData(QTcpSocket *client, const DataFrame &frame)
{
    Q_UNUSED(client)

    if (m_antennaDeviceWindow) {
        m_antennaDeviceWindow->parseWaveControlTelemetry(frame.payload);
    }
    // 温度窗口独立解析112路测温+50路开关
    if (temperatureTab) {
        temperatureTab->parseThermalTelemetry(frame.payload);
    }
}

/** 遥控指令应答 — 客户端对遥控指令的执行结果反馈 */
void MainWindow::handleControlAck(QTcpSocket *client, const DataFrame &frame)
{
    Q_UNUSED(client)

    // payload 通常包含：[指令序号][执行结果(0成功/非0失败)][附加信息]
    if (frame.payload.size() >= 1) {
        quint8 result = static_cast<quint8>(frame.payload.at(0));
        appendLog(QString("    遥控应答: 结果=%1").arg(result == 0 ? "成功" : "失败"));
    }
}

/** 设备状态上报 — 客户端上报自身设备运行状态 */
void MainWindow::handleDeviceStatus(QTcpSocket *client, const DataFrame &frame)
{
    Q_UNUSED(client)
    Q_UNUSED(frame)
    // TODO: 解析设备状态并更新UI显示
    appendLog("    设备状态已收到");
}

/** 错误报告 — 客户端报告错误信息 */
void MainWindow::handleErrorReport(QTcpSocket *client, const DataFrame &frame)
{
    Q_UNUSED(client)
    if (!frame.payload.isEmpty()) {
        QString errMsg = QString::fromUtf8(frame.payload);
        appendLog(QString("    └ 错误报告: %1").arg(errMsg));
    }
}

// ============================================================================
//                            辅助功能
// ============================================================================

void MainWindow::appendLog(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss]");
    ui->logPlainTextEdit->appendPlainText(timestamp + " " + message);
    QTextCursor cursor = ui->logPlainTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->logPlainTextEdit->setTextCursor(cursor);
}

void MainWindow::updateDateTime()
{
    m_timeLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
}

/** 获取客户端的可读标签（用于日志） */
QString MainWindow::clientPeerLabel(QTcpSocket *client)
{
    if (!client) return "(null)";
    return QString("%1:%2").arg(client->peerAddress().toString()).arg(client->peerPort());
}

/** 将指令码转为可读名称 */
QString MainWindow::commandName(quint8 cmd)
{
    switch (cmd) {
//    case Protocol::Cmd::HEARTBEAT:      return "心跳";
    case Protocol::Cmd::TELEMETRY_DATA: return "遥测数据";
//    case Protocol::Cmd::CONTROL_CMD:    return "遥控指令";
//    case Protocol::Cmd::DEVICE_STATUS:  return "设备状态";
//    case Protocol::Cmd::ERROR_REPORT:   return "错误报告";
    default:                            return QString("0x%1").arg(cmd, 2, 16, QLatin1Char('0'));
    }
}
