#include "mainwindow.h"
#include "ui_mainwindow.h"
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
    setWindowTitle("相控阵天线地面测试系统");

    // ========== 成员初始化 ==========
    m_currentMode = WorkMode::None;
    m_autoTestWindow = nullptr;
    m_instrumentControlWindow = nullptr;

    // ★★★ 核心变更：创建网络管理器（替代所有原始 QTcpSocket）★★★
    m_network = new NetworkManager(this);

    // 连接 NetworkManager 信号到 MainWindow 槽函数
    // --- 服务器事件 ---
    connect(m_network, &NetworkManager::logMessage,
            this, &MainWindow::onNetworkLog);
    connect(m_network, &NetworkManager::errorOccurred,
            this, &MainWindow::onNetworkError);

    connect(m_network, &NetworkManager::serverClientConnected,
            this, &MainWindow::onServerClientConnected);
    connect(m_network, &NetworkManager::serverClientDisconnected,
            this, &MainWindow::onServerClientDisconnected);
    connect(m_network, &NetworkManager::serverDataReceived,
            this, &MainWindow::onServerDataReceived);

    // --- 主界面客户端事件 ---
    connect(m_network, &NetworkManager::clientConnected,
            this, &MainWindow::onMainClientConnected);
    connect(m_network, &NetworkManager::clientDisconnected,
            this, &MainWindow::onMainClientDisconnected);
    connect(m_network, &NetworkManager::clientDataReceived,
            this, &MainWindow::onMainClientDataReceived);

    // --- 矢网事件（由 MainWindow 转发到 AutoTestWindow）---
    connect(m_network, &NetworkManager::vnaConnected,
            this, &MainWindow::onVnaConnected);
    connect(m_network, &NetworkManager::vnaDisconnected,
            this, &MainWindow::onVnaDisconnected);
    connect(m_network, &NetworkManager::vnaDataReceived,
            this, &MainWindow::onVnaDataReceived);

    // ========== 周期性指令定时器初始化（1秒周期，不自动启动） ==========
    m_periodicCommandTimer = new QTimer(this);
    connect(m_periodicCommandTimer, &QTimer::timeout, this, &MainWindow::onPeriodicCommandTimer);

    // ========== 创建设备子窗口（添加 parent，修复内存泄漏） ==========
    m_antennaDeviceWindow = new AntennaDeviceWindow(this);
    electronicTab = new ElectronicDeviceWindow(this);
    temperatureTab = new TemperatureInfoWindow(this);
    powerTab = new PowerVoltageWindow(this);

    // 在 addTab 之前记录每个 widget 的 UI 设计器原始几何尺寸
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

    // 客户端设置按钮信号槽
    connect(ui->connectClientButton, &QPushButton::clicked, [this]() {
        QString ip = ui->clientIpLineEdit->text().trimmed();
        quint16 port = static_cast<quint16>(ui->clientPortSpinBox->value());

        if (ip.isEmpty()) {
            QMessageBox::warning(this, "错误", "请输入服务器 IP 地址！");
            return;
        }
        QHostAddress addr(ip);
        if (addr.isNull()) {
            QMessageBox::warning(this, "错误", "IP 地址格式无效！");
            return;
        }

        if (!m_network->connectToServer(ip, port)) {
            QMessageBox::critical(this, "错误", QString("无法连接到服务器！\n%1")
                                  .arg("请检查网络设置"));
        }
    });

    connect(ui->disconnectClientButton, &QPushButton::clicked, [this]() {
        m_network->disconnectFromServer();
        if (m_periodicCommandTimer->isActive()) {
            m_periodicCommandTimer->stop();
        }
    });

    // ========== 子窗口信号槽连接 ==========
    connect(m_antennaDeviceWindow, &AntennaDeviceWindow::sendRawCommandToServer,
            this,      &MainWindow::onSendRawCommand);
    connect(temperatureTab,       &TemperatureInfoWindow::sendRawCommandToServer,
            this,                 &MainWindow::onSendRawCommand);

    // ========== 菜单栏信号槽连接 ==========
    connect(ui->actionAutoTestWindow, &QAction::triggered, this, &MainWindow::onActionAutoTestWindow);
    connect(ui->actionPatternSimulation, &QAction::triggered, this, &MainWindow::onActionPatternSimulation);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::onActionExit);
    connect(ui->actionDataReplay, &QAction::triggered, this, &MainWindow::onActionDataReplay);
    connect(ui->actionFirmwareUpgrade, &QAction::triggered, this, &MainWindow::onActionFirmwareUpgrade);
    connect(ui->actionNetworkConfig, &QAction::triggered, this, &MainWindow::onActionNetworkConfig);
    connect(ui->actionDataPathConfig, &QAction::triggered, this, &MainWindow::onActionDataPathConfig);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::onActionAbout);

    updateUIState();
}

MainWindow::~MainWindow()
{
    // 停止定时器
    if (m_periodicCommandTimer) {
        m_periodicCommandTimer->stop();
    }
    if (m_dateTimeTimer) {
        m_dateTimeTimer->stop();
    }

    // NetworkManager 的析构会自动清理所有 socket（因为它有 parent = this）
    delete ui;
}

// ============================================================================
//                          UI 状态更新
// ============================================================================

void MainWindow::updateUIState()
{
    bool modeSelected = (m_currentMode != WorkMode::None);

    bool serverRunning = m_network->isServerRunning();
    bool clientConnected = m_network->isClientConnected();

    ui->modeComboBox->setEnabled(!serverRunning && !clientConnected);
    ui->confirmModeButton->setEnabled(!modeSelected && !serverRunning && !clientConnected);
    ui->cancelModeButton->setEnabled(modeSelected && !serverRunning && !clientConnected);

    // 根据模式控制服务器/客户端设置 GroupBox 的可见性
    switch (m_currentMode) {
    case WorkMode::None:
        ui->serverGroupBox->setVisible(false);
        ui->clientGroupBox->setVisible(false);
        break;
    case WorkMode::AntennaGroundTest:
        ui->serverGroupBox->setVisible(false);
        ui->clientGroupBox->setVisible(true);
        break;
    case WorkMode::SimulateDevice:
        ui->serverGroupBox->setVisible(true);
        ui->clientGroupBox->setVisible(true);
        break;
    }

    // 服务器控件状态
    bool serverControlsEnabled = modeSelected && !serverRunning;
    ui->ipLineEdit->setEnabled(serverControlsEnabled);
    ui->portSpinBox->setEnabled(serverControlsEnabled);
    ui->startServerButton->setEnabled(modeSelected && !serverRunning);
    ui->stopServerButton->setEnabled(serverRunning);

    // 客户端控件状态
    bool clientControlsEnabled = modeSelected && !clientConnected;
    ui->clientIpLineEdit->setEnabled(clientControlsEnabled);
    ui->clientPortSpinBox->setEnabled(clientControlsEnabled);
    ui->connectClientButton->setEnabled(modeSelected && !clientConnected);
    ui->disconnectClientButton->setEnabled(clientConnected);

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

    updateUIState();
}

void MainWindow::onCancelModeClicked()
{
    if (m_network->isServerRunning()) {
        m_network->stopServer();
        appendLog("已自动停止服务器。");
    }

    m_currentMode = WorkMode::None;

    appendLog("已取消工作模式。");
    updateUIState();
}

// ============================================================================
//                         TCP 服务器管理（委托给 NetworkManager）
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

    int maxClients = getMaxClientsForMode(m_currentMode);
    if (!m_network->startServer(ip, port, maxClients)) {
        QMessageBox::critical(this, "错误",
                              QString("无法启动服务！\n%1\n请检查端口是否被占用。")
                              .arg("请检查网络设置"));
        return;
    }

    // 保存目标客户端IP（用于周期性指令定向发送）
    m_targetClientIP = "127.0.0.1";

    onServerStarted();
    updateUIState();
}

void MainWindow::onStopServerClicked()
{
    m_network->stopServer();
    m_targetClientIP.clear();
    onServerStopped();
    updateUIState();
}

// ============================================================================
//                     NetworkManager 事件转发 — 服务器
// ============================================================================

void MainWindow::onServerStarted()
{
    QString ip = ui->ipLineEdit->text().trimmed();
    quint16 port = static_cast<quint16>(ui->portSpinBox->value());
    appendLog(QString("[成功] 服务已启动，监听 %1:%2").arg(ip).arg(port));
    appendLog(QString("--- 等待客户端连接 ---").arg(getMaxClientsForMode(m_currentMode)));
}

void MainWindow::onServerStopped()
{
    // 清理环形缓冲区
    m_clientBuffers.clear();
    appendLog("--- 已手动停止服务器 ---");
}

void MainWindow::onServerClientConnected(const ConnectionHandle &h)
{
    // 为该客户端创建独立的环形缓冲区
    m_clientBuffers[h.id] = std::make_shared<RingBuffer>(128 * 1024);
    appendLog(QString("[连接] 客户端 #%1 已接入：%2（当前 %3/%4）")
              .arg(h.id).arg(h.peerInfo).arg(h.id).arg(getMaxClientsForMode(m_currentMode)));
}

void MainWindow::onServerClientDisconnected(const ConnectionHandle &h)
{
    m_clientBuffers.remove(h.id);
    appendLog(QString("[断开] 客户端 #%1 %2 已离开").arg(h.id).arg(h.peerInfo));
}

void MainWindow::onServerDataReceived(const ConnectionHandle &h, const QByteArray &data)
{
    // 写入对应客户端的环形缓冲区
    auto it = m_clientBuffers.find(h.id);
    if (it == m_clientBuffers.end()) return;

    auto &ringBuf = it.value();
    ringBuf->write(data);

    // 循环解析完整帧
    while (true) {
        size_t available = ringBuf->readableBytes();
        if (!ProtocolCodec::hasCompleteFrame(static_cast<int>(available))) {
            break;
        }

        size_t peekSize = std::min(available, static_cast<size_t>(65536));
        QByteArray preview = ringBuf->peek(peekSize);
        if (preview.isEmpty()) break;

        DataFrame frame;
        int consumed = 0;
        bool parsed = ProtocolCodec::parse(preview, frame, consumed);

        if (!parsed) {
            if (consumed > 0) {
                ringBuf->consume(static_cast<size_t>(consumed));
                continue;
            } else {
                break;
            }
        }

        ringBuf->consume(static_cast<size_t>(consumed));
        handleReceivedFrame(h.id, frame);
    }
}

// ============================================================================
//                   NetworkManager 事件转发 — 客户端模式
// ============================================================================

void MainWindow::onMainClientConnected()
{
    // 为客户端 socket 创建环形缓冲区
    // （实际数据在 onMainClientDataReceived 中写入）
    m_clientBuffers[0] = std::make_shared<RingBuffer>(128 * 1024);

    // 启动周期性指令定时器（1秒间隔）—— 客户端模式下主动发送
    if (!m_periodicCommandTimer->isActive()) {
        m_periodicCommandTimer->start(1000);
    }

    ConnectionHandle h = m_network->clientHandle();
    updateUIState();
    appendLog(QString("[成功] 已连接到服务器 %1").arg(h.peerInfo));
}

void MainWindow::onMainClientDisconnected()
{
    // 停止周期性指令定时器
    if (m_periodicCommandTimer->isActive()) {
        m_periodicCommandTimer->stop();
    }

    m_clientBuffers.remove(0);
    updateUIState();
    ConnectionHandle h = m_network->clientHandle();
    appendLog(QString("[断开] 与服务器的连接已断开：%1").arg(h.peerInfo));
}

void MainWindow::onMainClientDataReceived(const QByteArray &data)
{
    // 写入客户端的环形缓冲区并尝试解析帧
    auto it = m_clientBuffers.find(0);   // clientId=0 表示主界面客户端
    if (it == m_clientBuffers.end()) return;

    auto &ringBuf = it.value();
    ringBuf->write(data);

    while (true) {
        size_t available = ringBuf->readableBytes();
        if (!ProtocolCodec::hasCompleteFrame(static_cast<int>(available))) {
            break;
        }

        size_t peekSize = std::min(available, static_cast<size_t>(65536));
        QByteArray preview = ringBuf->peek(peekSize);
        if (preview.isEmpty()) break;

        DataFrame frame;
        int consumed = 0;
        bool parsed = ProtocolCodec::parse(preview, frame, consumed);

        if (!parsed) {
            if (consumed > 0) {
                ringBuf->consume(static_cast<size_t>(consumed));
                continue;
            } else {
                break;
            }
        }

        ringBuf->consume(static_cast<size_t>(consumed));
        handleReceivedFrame(0, frame);
    }
}

// ============================================================================
//                   NetworkManager 事件转发 — 矢网仪器
// ============================================================================

void MainWindow::onVnaConnected()
{
    // 通知 AutoTestWindow 连接已成功
    if (m_autoTestWindow) {
        m_autoTestWindow->onConnected();
    }
}

void MainWindow::onVnaDisconnected()
{
    // 通知 AutoTestWindow 连接已断开
    if (m_autoTestWindow) {
        m_autoTestWindow->onDisconnected();
    }
}

void MainWindow::onVnaDataReceived(const QByteArray &data)
{
    // TODO: 处理矢网仪器返回数据
    appendLog(QString("[矢网←] %1 字节").arg(data.size()));
}

void MainWindow::onNetworkError(const QString &error)
{
    appendLog(QString("[ERROR] %1").arg(error));
}

void MainWindow::onNetworkLog(const QString &msg)
{
    // 直接追加日志（NetworkManager 日志带时间戳前缀）
    appendLog(msg);
}

// ============================================================================
//
//                       ★ 数据发送接口 ★
//
// ============================================================================

bool MainWindow::sendFrame(int clientId, quint8 command, const QByteArray &payload)
{
    DataFrame frame;
    frame.command = command;
    frame.payload = payload;

    QByteArray packet = ProtocolCodec::encode(frame);

    if (clientId == 0) {
        // 主界面客户端模式 → 通过 NetworkManager 发送
        bool ok = m_network->sendToServer(packet);
        if (!ok) appendLog("[发送失败] 目标客户端未连接");
        return ok;
    } else {
        // 服务端客户端模式 → 通过 NetworkManager 发送
        bool ok = m_network->sendToClient(clientId, packet);
        if (!ok) appendLog(QString("[发送失败] 客户端 #%1 未连接").arg(clientId));
        return ok;
    }
}

void MainWindow::broadcastFrame(quint8 command, const QByteArray &payload)
{
    DataFrame frame;
    frame.command = command;
    frame.payload = payload;
    QByteArray packet = ProtocolCodec::encode(frame);

    m_network->broadcastToClients(packet);

    QString cmdName = commandName(command);
    appendLog(QString("[广播] 命令=%2 数据长度=%3 字节").arg(cmdName).arg(payload.size()));
}

/**
 * @brief 接收子窗口发出的原始控制指令，通过主界面客户端发送
 */
void MainWindow::onSendRawCommand(const QByteArray &command)
{
    QString hexStr = command.toHex(' ').toUpper();

    // 客户端模式下，通过 clientSocket 向服务器发送指令
    if (m_network->isClientConnected() && m_network->sendToServer(command)) {
        appendLog(QString("[指令发送→服务器] %1 字节 [%2]")
                  .arg(command.size()).arg(hexStr));
    } else {
        appendLog("[指令] 无可用连接，无法发送指令");
    }
}

// ============================================================================
//
//                      ★ 周期性指令（1秒）★
//
// ============================================================================

/**
 * @brief 1秒周期定时器回调：向特定IP的客户端发送周期指令 {0xEB, 0x90, 0x02, 0xC0, 0x05}
 */
void MainWindow::onPeriodicCommandTimer()
{
    QByteArray periodicCmd = QByteArray::fromRawData("\xEB\x90\x02\xC0\x05", 5);

    // 客户端模式下，向服务器发送周期指令
    if (m_network->isClientConnected() && m_network->sendToServer(periodicCmd)) {
        appendLog("[周期指令→服务器] [EB 90 02 C0 05]");
    }
}

// ============================================================================
//
//                      ★ 业务帧分发处理 ★
//
// ============================================================================

void MainWindow::handleReceivedFrame(int clientId, const DataFrame &frame)
{
    QString src = clientPeerLabel(clientId);
    QString cmdName = commandName(frame.command);

    appendLog(QString("[接收←%1] 命令=%2 数据长度=%3 字节")
              .arg(src).arg(cmdName).arg(frame.payload.size()));

    switch (frame.command) {
    case Protocol::Cmd::TELEMETRY_DATA:
        handleTelemetryData(clientId, frame);
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
void MainWindow::handleHeartbeat(int clientId, const DataFrame &frame)
{
    Q_UNUSED(clientId)
    Q_UNUSED(frame)
}

/** 遥测数据上传 — 接收客户端上报的遥测数据 */
void MainWindow::handleTelemetryData(int clientId, const DataFrame &frame)
{
    Q_UNUSED(clientId)

    if (m_antennaDeviceWindow) {
        m_antennaDeviceWindow->parseWaveControlTelemetry(frame.payload);
    }
    if (temperatureTab) {
        temperatureTab->parseThermalTelemetry(frame.payload);
    }
}

/** 遥控指令应答 */
void MainWindow::handleControlAck(int clientId, const DataFrame &frame)
{
    Q_UNUSED(clientId)

    if (frame.payload.size() >= 1) {
        quint8 result = static_cast<quint8>(frame.payload.at(0));
        appendLog(QString("    遥控应答: 结果=%1").arg(result == 0 ? "成功" : "失败"));
    }
}

/** 设备状态上报 */
void MainWindow::handleDeviceStatus(int clientId, const DataFrame &frame)
{
    Q_UNUSED(clientId)
    Q_UNUSED(frame)
    appendLog("    设备状态已收到");
}

/** 错误报告 */
void MainWindow::handleErrorReport(int clientId, const DataFrame &frame)
{
    Q_UNUSED(clientId)
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

    constexpr int MAX_LOG_LINES = 5000;
    QTextDocument *doc = ui->logPlainTextEdit->document();
    if (doc->blockCount() > MAX_LOG_LINES) {
        QTextCursor cursor(doc);
        cursor.movePosition(QTextCursor::Start);
        cursor.select(QTextCursor::BlockUnderCursor);
        cursor.removeSelectedText();
        cursor.deleteChar();
    }

    QTextCursor cursor = ui->logPlainTextEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->logPlainTextEdit->setTextCursor(cursor);
}

void MainWindow::updateDateTime()
{
    m_timeLabel->setText(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
}

QString MainWindow::clientPeerLabel(int clientId)
{
    if (clientId == 0) {
        // 主界面客户端
        return "主客户端";
    }
    // 从 NetworkManager 获取信息（简化处理）
    return QString("客户端#%1").arg(clientId);
}

QString MainWindow::commandName(quint8 cmd)
{
    switch (cmd) {
    case Protocol::Cmd::TELEMETRY_DATA: return "遥测数据";
    default:                            return QString("0x%1").arg(cmd, 2, 16, QLatin1Char('0'));
    }
}

// ============================================================================
//                         菜单栏功能实现
// ============================================================================

void MainWindow::onActionAutoTestWindow()
{
    if (!m_autoTestWindow) {
        m_autoTestWindow = new AutoTestWindow(this);

        // 天线加电/断电请求转发
        connect(m_autoTestWindow, &AutoTestWindow::antennaPowerOnRequested,
                m_antennaDeviceWindow, &AntennaDeviceWindow::antennaPowerOn);
        connect(m_autoTestWindow, &AutoTestWindow::antennaPowerOffRequested,
                m_antennaDeviceWindow, &AntennaDeviceWindow::antennaPowerOff);

        // ★★★ 矢网连接请求转发到 NetworkManager ★★★
        connect(m_autoTestWindow, &AutoTestWindow::connectToHostRequested,
                this, [this](const QString &ip, quint16 port) {
            if (!m_network->connectToVna(ip, port)) {
                QMessageBox::critical(this, "错误",
                    QString("无法连接到矢网仪器！\n%1").arg("请检查网络设置"));
            }
        });

        connect(m_autoTestWindow, &AutoTestWindow::disconnectFromHostRequested,
                this, [this]() {
            m_network->disconnectFromVna();
        });
    }
    m_autoTestWindow->show();
    m_autoTestWindow->raise();
    m_autoTestWindow->activateWindow();
}

void MainWindow::onActionInstrumentControlWindow()
{
    if (!m_instrumentControlWindow) {
        m_instrumentControlWindow = new InstrumentControlWindow(this);
    }
    m_instrumentControlWindow->show();
    m_instrumentControlWindow->raise();
    m_instrumentControlWindow->activateWindow();
}

void MainWindow::onActionExit()
{
    close();
}

void MainWindow::onActionPatternSimulation()
{
    appendLog("[菜单] 方向图仿真功能暂未实现");
}

void MainWindow::onActionDataReplay()
{
    appendLog("[菜单] 数据回放功能暂未实现");
}

void MainWindow::onActionFirmwareUpgrade()
{
    appendLog("[菜单] 固件升级功能暂未实现");
}

void MainWindow::onActionNetworkConfig()
{
    appendLog("[菜单] 网络设置功能暂未实现");
}

void MainWindow::onActionDataPathConfig()
{
    appendLog("[菜单] 数据保存路径功能暂未实现");
}

void MainWindow::onActionAbout()
{
    QMessageBox::about(this, "关于",
        "<h2>相控阵天线地面测试系统</h2>"
        "<p>版本：v1.0.0</p>"
        "<p>版权所有 © 2026 齐鲁空天信息研究院</p>"
        "<p>保留所有权利</p>");
}
