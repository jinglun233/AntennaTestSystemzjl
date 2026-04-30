/****************************************************************************
** 网络管理层实现
**
** 职责：
**   1. 封装 TCP Server / Client / Instru 三种连接的生命周期
**   2. 为每个连接维护独立的 RingBuffer
**   3. 通过信号向外部报告连接状态和数据
**   4. 记录所有接收到的原始数据到 D:\时间_Log.txt
****************************************************************************/
#include "networkmanager.h"
#include "ringbuffer.h"
#include <QHostAddress>
#include <QDateTime>
#include <QDir>
#include <QTextStream>

// ============================================================================
//                              构造 / 析构
// ============================================================================

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , m_tcpServer(nullptr)
    , m_nextClientId(1)
    , m_maxClients(0)
    , m_clientSocket(nullptr)
    , m_instruSocket(nullptr)
    , m_logFile(nullptr)
    , m_logFileOpened(false)
    , m_commandLogFile(nullptr)
    , m_commandLogOpened(false)
{
    // 初始化接收数据日志文件：D:\yyyyMMdd_Log.txt，每天自动轮转
    openLogForToday();
    // 初始化指令发送日志文件：D:\yyyyMMdd_LogCommand.txt，每天自动轮转
    openCommandLogForToday();
}

/**
 * @brief 打开/切换到当天的接收日志文件
 */
void NetworkManager::openLogForToday()
{
    // 关闭旧文件（如果有）
    if (m_logFile && m_logFileOpened) {
        m_logFile->close();
        m_logFileOpened = false;
    }

    QString today = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString logPath = QString("D:/%1_Log.txt").arg(today);

    if (!m_logFile) {
        m_logFile = new QFile(this);
    }
    m_logFile->setFileName(logPath);

    if (m_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_logDate = today;
        m_logFileOpened = true;
        emit logMessage(QString("[日志] 接收数据记录已开启: %1").arg(logPath));
    } else {
        emit logMessage(QString("[日志] 无法打开日志文件: %1").arg(logPath));
    }
}

/**
 * @brief 打开/切换到当天的指令发送日志文件
 *
 * 文件路径：D:\yyyyMMdd_LogCommand.txt
 * 每天自动轮转，与接收数据日志独立。
 */
void NetworkManager::openCommandLogForToday()
{
    // 关闭旧文件（如果有）
    if (m_commandLogFile && m_commandLogOpened) {
        m_commandLogFile->close();
        m_commandLogOpened = false;
    }

    QString today = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString logPath = QString("D:/%1_LogCommand.txt").arg(today);

    if (!m_commandLogFile) {
        m_commandLogFile = new QFile(this);
    }
    m_commandLogFile->setFileName(logPath);

    if (m_commandLogFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_commandLogDate = today;
        m_commandLogOpened = true;
        emit logMessage(QString("[日志] 指令发送记录已开启: %1").arg(logPath));
    } else {
        emit logMessage(QString("[日志] 无法打开指令日志文件: %1").arg(logPath));
    }
}

NetworkManager::~NetworkManager()
{
    stopServer();
    disconnectFromServer();
    disconnectFromInstru();

    if (m_logFile && m_logFileOpened) {
        m_logFile->close();
        m_logFileOpened = false;
    }
    if (m_commandLogFile && m_commandLogOpened) {
        m_commandLogFile->close();
        m_commandLogOpened = false;
    }
}

// ============================================================================
//                         服务器模式
// ============================================================================

bool NetworkManager::startServer(const QString &ip, quint16 port, int maxClients)
{
    if (m_tcpServer) {
        emit logMessage("[网络] 服务器已在运行");
        return false;
    }

    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &NetworkManager::onNewConnection);

    QHostAddress addr(ip);
    if (!m_tcpServer->listen(addr, port)) {
        emit errorOccurred(QString("服务器启动失败: %1").arg(m_tcpServer->errorString()));
        delete m_tcpServer;
        m_tcpServer = nullptr;
        return false;
    }

    m_maxClients = maxClients;
    emit logMessage(QString("[网络] 服务器已启动，监听 %1:%2 (最大客户端: %3)")
                    .arg(ip).arg(port).arg(maxClients));
    return true;
}

void NetworkManager::stopServer()
{
    if (!m_tcpServer) return;

    for (QTcpSocket *sock : m_serverClients) {
        disconnect(sock, nullptr, this, nullptr);
        sock->abort();
        sock->deleteLater();
    }
    m_serverClients.clear();
    m_clientIds.clear();
    m_idToSocket.clear();
    m_serverBuffers.clear();

    m_tcpServer->close();
    m_tcpServer->deleteLater();
    m_tcpServer = nullptr;
    m_maxClients = 0;

    emit logMessage("[网络] 服务器已停止");
}

bool NetworkManager::isServerRunning() const
{
    return m_tcpServer && m_tcpServer->isListening();
}

QVector<ConnectionHandle> NetworkManager::serverClients() const
{
    QVector<ConnectionHandle> handles;
    for (QTcpSocket *sock : m_serverClients) {
        ConnectionHandle h;
        h.id = m_clientIds.value(sock, -1);
        h.type = ConnectionType::ServerClient;
        h.peerInfo = peerInfo(sock);
        h.connected = (sock->state() == QTcpSocket::ConnectedState);
        handles.append(h);
    }
    return handles;
}

// ============================================================================
//                         客户端模式（主界面使用）
// ============================================================================

bool NetworkManager::connectToServer(const QString &ip, quint16 port)
{
    if (m_clientSocket) {
        emit errorOccurred("客户端已连接，请先断开");
        return false;
    }

    m_clientSocket = new QTcpSocket(this);
    connect(m_clientSocket, &QTcpSocket::readyRead,
            this, &NetworkManager::onClientSocketReadyRead);
    connect(m_clientSocket, &QTcpSocket::disconnected,
            this, &NetworkManager::onClientSocketDisconnected);

    m_clientSocket->connectToHost(QHostAddress(ip), port);
    if (!m_clientSocket->waitForConnected(3000)) {
        emit errorOccurred(QString("连接服务器失败: %1").arg(m_clientSocket->errorString()));
        cleanupSocket(m_clientSocket);
        m_clientSocket = nullptr;
        return false;
    }

    m_clientBuffer = std::make_shared<RingBuffer>(128 * 1024);
    emit clientConnected();
    emit logMessage(QString("[网络] 已连接到服务器 %1:%2").arg(ip).arg(port));
    return true;
}

void NetworkManager::disconnectFromServer()
{
    if (!m_clientSocket) return;

    cleanupSocket(m_clientSocket);
    m_clientSocket = nullptr;
    m_clientBuffer.reset();

    emit clientDisconnected();
    emit logMessage("[网络] 已与服务器断开");
}

bool NetworkManager::isClientConnected() const
{
    return m_clientSocket && m_clientSocket->state() == QTcpSocket::ConnectedState;
}

// ============================================================================
//                         仪器模式（AutoTestWindow 使用）
// ============================================================================

bool NetworkManager::connectToInstru(const QString &ip, quint16 port)
{
    if (m_instruSocket) {
        emit errorOccurred("仪器已连接，请先断开");
        return false;
    }

    m_instruSocket = new QTcpSocket(this);
    connect(m_instruSocket, &QTcpSocket::readyRead,
            this, &NetworkManager::onInstruSocketReadyRead);
    connect(m_instruSocket, &QTcpSocket::disconnected,
            this, &NetworkManager::onInstruSocketDisconnected);

    m_instruSocket->connectToHost(QHostAddress(ip), port);
    if (!m_instruSocket->waitForConnected(3000)) {
        emit errorOccurred(QString("连接仪器失败: %1").arg(m_instruSocket->errorString()));
        cleanupSocket(m_instruSocket);
        m_instruSocket = nullptr;
        return false;
    }

    emit instruConnected();
    emit logMessage(QString("[网络] 已连接到仪器 %1:%2").arg(ip).arg(port));
    return true;
}

void NetworkManager::disconnectFromInstru()
{
    if (!m_instruSocket) return;

    cleanupSocket(m_instruSocket);
    m_instruSocket = nullptr;

    emit instruDisconnected();
    emit logMessage("[网络] 已与仪器断开");
}

bool NetworkManager::isInstruConnected() const
{
    return m_instruSocket && m_instruSocket->state() == QTcpSocket::ConnectedState;
}

// ============================================================================
//                         数据发送
// ============================================================================

bool NetworkManager::sendToServer(const QByteArray &data)
{
    if (!isClientConnected()) {
        emit errorOccurred("客户端未连接，无法发送");
        return false;
    }
    qint64 sent = m_clientSocket->write(data);
    m_clientSocket->flush();
    // 记录指令发送日志（天线地检 → 外部服务器）
    logSentData(data, "天线地检");
    return sent == data.size();
}

bool NetworkManager::sendToInstru(const QByteArray &data)
{
    if (!isInstruConnected()) {
        emit errorOccurred("仪器未连接，无法发送");
        return false;
    }
    qint64 sent = m_instruSocket->write(data);
    m_instruSocket->flush();
    // 记录指令发送日志（→ 仪器）
    logSentData(data, "仪器");
    return sent == data.size();
}

void NetworkManager::broadcastToClients(const QByteArray &data)
{
    QVector<QTcpSocket*> clientsCopy = m_serverClients;
    for (QTcpSocket *sock : clientsCopy) {
        if (sock->state() == QTcpSocket::ConnectedState) {
            sock->write(data);
            sock->flush();
        }
    }
    // 记录指令发送日志（广播 → 电子设备）
    logSentData(data, "电子设备");
}

// ============================================================================
//                         查询接口
// ============================================================================

ConnectionHandle NetworkManager::clientHandle() const
{
    ConnectionHandle h;
    h.id = 0;
    h.type = ConnectionType::Client;
    h.peerInfo = m_clientSocket ? peerInfo(m_clientSocket) : QString();
    h.connected = isClientConnected();
    return h;
}

ConnectionHandle NetworkManager::instruHandle() const
{
    ConnectionHandle h;
    h.id = 0;
    h.type = ConnectionType::Instru;
    h.peerInfo = m_instruSocket ? peerInfo(m_instruSocket) : QString();
    h.connected = isInstruConnected();
    return h;
}

// ============================================================================
//                         私有槽函数 — 服务器
// ============================================================================

void NetworkManager::onNewConnection()
{
    while (m_tcpServer->hasPendingConnections()) {
        // m_maxClients <= 0 表示不限制连接数量
        if (m_maxClients > 0 && m_serverClients.size() >= m_maxClients) {
            QTcpSocket *rejected = m_tcpServer->nextPendingConnection();
            emit logMessage(QString("[网络] 拒绝连接 %1，已达上限").arg(peerInfo(rejected)));
            rejected->disconnectFromHost();
            rejected->deleteLater();
            continue;
        }

        QTcpSocket *clientSocket = m_tcpServer->nextPendingConnection();
        int id = m_nextClientId++;

        m_serverClients.append(clientSocket);
        m_clientIds[clientSocket] = id;
        m_idToSocket[id] = clientSocket;
        m_serverBuffers[clientSocket] = std::make_shared<RingBuffer>(128 * 1024);

        connect(clientSocket, &QTcpSocket::disconnected,
                this, &NetworkManager::onServerClientDisconnected);
        connect(clientSocket, &QTcpSocket::readyRead,
                this, &NetworkManager::onServerClientReadyRead);

        ConnectionHandle h;
        h.id = id;
        h.type = ConnectionType::ServerClient;
        h.peerInfo = peerInfo(clientSocket);
        h.connected = true;

        emit serverClientConnected(h);
    }
}

void NetworkManager::onServerClientDisconnected()
{
    QTcpSocket *sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock) return;

    int id = m_clientIds.value(sock, -1);

    ConnectionHandle h;
    h.id = id;
    h.type = ConnectionType::ServerClient;
    h.peerInfo = peerInfo(sock);
    h.connected = false;

    m_serverClients.removeAll(sock);
    m_clientIds.remove(sock);
    m_idToSocket.remove(id);
    m_serverBuffers.remove(sock);

    sock->deleteLater();

    emit serverClientDisconnected(h);
    emit logMessage(QString("[网络] 客户端 #%1 %2 已断开").arg(id).arg(h.peerInfo));
}

// ============================================================================
//                         私有槽函数 — 服务端模式
// ============================================================================
void NetworkManager::onServerClientReadyRead()
{
    QTcpSocket *sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock || !m_serverBuffers.contains(sock)) return;

    QByteArray data = sock->readAll();
    if (data.isEmpty()) return;

    auto &buf = m_serverBuffers[sock];
    buf->write(data);

    // 记录接收数据到日志
    logReceivedData(data, "电子设备");

    // TODO: 帧解析逻辑从 RingBuffer 中提取完整帧（保留原有 MainWindow 的解析流程）
    int id = m_clientIds.value(sock, -1);
    ConnectionHandle h;
    h.id = id;
    h.type = ConnectionType::ServerClient;
    h.peerInfo = peerInfo(sock);
    h.connected = true;

    emit serverDataReceived(h, data);
}

// ============================================================================
//                         私有槽函数 — 客户端模式
// ============================================================================

void NetworkManager::onClientSocketReadyRead()
{
    if (!m_clientSocket || !m_clientBuffer) return;

    QByteArray data = m_clientSocket->readAll();
    if (data.isEmpty()) return;

    // 记录接收数据到日志
    logReceivedData(data, "天线地检");

    m_clientBuffer->write(data);
    emit clientDataReceived(data);
}

void NetworkManager::onClientSocketDisconnected()
{
    if (!m_clientSocket) return;

    emit logMessage(QString("[网络] 服务器连接已断开 %1").arg(peerInfo(m_clientSocket)));

    cleanupSocket(m_clientSocket);
    m_clientSocket = nullptr;
    m_clientBuffer.reset();

    emit clientDisconnected();
}

// ============================================================================
//                         私有槽函数 — 仪器
// ============================================================================

void NetworkManager::onInstruSocketReadyRead()
{
    if (!m_instruSocket) return;

    QByteArray data = m_instruSocket->readAll();
    if (data.isEmpty()) return;

    // 记录接收数据到日志
    logReceivedData(data, "仪器");

    emit instruDataReceived(data);
}

void NetworkManager::onInstruSocketDisconnected()
{
    if (!m_instruSocket) return;

    emit logMessage(QString("[网络] 仪器连接已断开 %1").arg(peerInfo(m_instruSocket)));

    cleanupSocket(m_instruSocket);
    m_instruSocket = nullptr;

    emit instruDisconnected();
}

// ============================================================================
//                         内部辅助
// ============================================================================

void NetworkManager::cleanupSocket(QTcpSocket *socket)
{
    if (!socket) return;
    disconnect(socket, nullptr, this, nullptr);
    socket->abort();
    socket->deleteLater();
}

QString NetworkManager::peerInfo(QTcpSocket *socket) const
{
    if (!socket) return "(null)";
    return QString("%1:%2")
           .arg(socket->peerAddress().toString())
           .arg(socket->peerPort());
}

// ============================================================================
//                         接收数据日志
// ============================================================================

/**
 * @brief 记录接收到的原始数据到 D:\yyyyMMdd_Log.txt
 *
 * 格式示例：
 *   2024/5/27 14:47:51 服务端 接收
 *   eb 90 02 c0 05
 */
void NetworkManager::logReceivedData(const QByteArray &data, const QString &typeLabel)
{
    if (data.isEmpty()) return;

    // 检查日期是否变更，自动轮转到新日志文件
    QString today = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    if (today != m_logDate) {
        openLogForToday();
    }

    if (!m_logFile || !m_logFileOpened) return;

    // 时间戳格式: 2024/5/27 14:47:51 服务端 接收
    QString timestamp = QDateTime::currentDateTime().toString("yyyy/M/d hh:mm:ss");

    // 将字节数据转为十六进制空格分隔字符串
    QStringList hexParts;
    for (int i = 0; i < data.size(); ++i) {
        hexParts.append(QString("%1").arg((unsigned char)data[i], 2, 16, QChar('0')));
    }
    QString hexString = hexParts.join(" ");

    QTextStream stream(m_logFile);
    stream << timestamp << " " << typeLabel << " 接收" << "\n"
           << hexString << "\n";
    stream.flush();
}

// ============================================================================
//                         指令发送日志
// ============================================================================

/**
 * @brief 记录发送的指令数据到 D:\yyyyMMdd_LogCommand.txt
 *
 * 格式示例：
 *   2024/5/27 14:48:03 天线地检 发送
 *   eb 90 02 c0 05
 */
void NetworkManager::logSentData(const QByteArray &data, const QString &typeLabel)
{
    if (data.isEmpty()) return;

    // 检查日期是否变更，自动轮转到新日志文件
    QString today = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    if (today != m_commandLogDate) {
        openCommandLogForToday();
    }

    if (!m_commandLogFile || !m_commandLogOpened) return;

    // 时间戳格式: 2024/5/27 14:48:03 天线地检 发送
    QString timestamp = QDateTime::currentDateTime().toString("yyyy/M/d hh:mm:ss");

    // 将字节数据转为十六进制空格分隔字符串
    QStringList hexParts;
    for (int i = 0; i < data.size(); ++i) {
        hexParts.append(QString("%1").arg((unsigned char)data[i], 2, 16, QChar('0')));
    }
    QString hexString = hexParts.join(" ");

    QTextStream stream(m_commandLogFile);
    stream << timestamp << " " << typeLabel << " 发送" << "\n"
           << hexString << "\n";
    stream.flush();
}
