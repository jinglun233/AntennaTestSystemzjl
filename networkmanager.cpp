/****************************************************************************
** 网络管理层实现
**
** 职责：
**   1. 封装 TCP Server / Client / VNA 三种连接的生命周期
**   2. 为每个连接维护独立的 RingBuffer
**   3. 通过信号向外部报告连接状态和数据
****************************************************************************/
#include "networkmanager.h"
#include "ringbuffer.h"
#include <QHostAddress>
#include <QDateTime>

// ============================================================================
//                              构造 / 析构
// ============================================================================

NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
    , m_tcpServer(nullptr)
    , m_nextClientId(1)
    , m_maxClients(0)
    , m_clientSocket(nullptr)
    , m_vnaSocket(nullptr)
{
}

NetworkManager::~NetworkManager()
{
    stopServer();
    disconnectFromServer();
    disconnectFromVna();
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
//                         矢网仪器模式（AutoTestWindow 使用）
// ============================================================================

bool NetworkManager::connectToVna(const QString &ip, quint16 port)
{
    if (m_vnaSocket) {
        emit errorOccurred("矢网已连接，请先断开");
        return false;
    }

    m_vnaSocket = new QTcpSocket(this);
    connect(m_vnaSocket, &QTcpSocket::readyRead,
            this, &NetworkManager::onVnaSocketReadyRead);
    connect(m_vnaSocket, &QTcpSocket::disconnected,
            this, &NetworkManager::onVnaSocketDisconnected);

    m_vnaSocket->connectToHost(QHostAddress(ip), port);
    if (!m_vnaSocket->waitForConnected(3000)) {
        emit errorOccurred(QString("连接矢网失败: %1").arg(m_vnaSocket->errorString()));
        cleanupSocket(m_vnaSocket);
        m_vnaSocket = nullptr;
        return false;
    }

    emit vnaConnected();
    emit logMessage(QString("[网络] 已连接到矢网 %1:%2").arg(ip).arg(port));
    return true;
}

void NetworkManager::disconnectFromVna()
{
    if (!m_vnaSocket) return;

    cleanupSocket(m_vnaSocket);
    m_vnaSocket = nullptr;

    emit vnaDisconnected();
    emit logMessage("[网络] 已与矢网断开");
}

bool NetworkManager::isVnaConnected() const
{
    return m_vnaSocket && m_vnaSocket->state() == QTcpSocket::ConnectedState;
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
    return sent == data.size();
}

bool NetworkManager::sendToVna(const QByteArray &data)
{
    if (!isVnaConnected()) {
        emit errorOccurred("矢网未连接，无法发送");
        return false;
    }
    qint64 sent = m_vnaSocket->write(data);
    m_vnaSocket->flush();
    return sent == data.size();
}

bool NetworkManager::sendToClient(int clientId, const QByteArray &data)
{
    QTcpSocket *sock = m_idToSocket.value(clientId, nullptr);
    if (!sock || sock->state() != QTcpSocket::ConnectedState) {
        emit errorOccurred(QString("客户端 #%1 未连接").arg(clientId));
        return false;
    }
    qint64 sent = sock->write(data);
    sock->flush();
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

ConnectionHandle NetworkManager::vnaHandle() const
{
    ConnectionHandle h;
    h.id = 0;
    h.type = ConnectionType::Vna;
    h.peerInfo = m_vnaSocket ? peerInfo(m_vnaSocket) : QString();
    h.connected = isVnaConnected();
    return h;
}

// ============================================================================
//                         私有槽函数 — 服务器
// ============================================================================

void NetworkManager::onNewConnection()
{
    while (m_tcpServer->hasPendingConnections()) {
        if (m_serverClients.size() >= m_maxClients) {
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
        emit logMessage(QString("[网络] 客户端 #%1 已接入 %2").arg(id).arg(h.peerInfo));
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

void NetworkManager::onServerClientReadyRead()
{
    QTcpSocket *sock = qobject_cast<QTcpSocket*>(sender());
    if (!sock || !m_serverBuffers.contains(sock)) return;

    QByteArray data = sock->readAll();
    if (data.isEmpty()) return;

    auto &buf = m_serverBuffers[sock];
    buf->write(data);

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
//                         私有槽函数 — 矢网
// ============================================================================

void NetworkManager::onVnaSocketReadyRead()
{
    if (!m_vnaSocket) return;

    QByteArray data = m_vnaSocket->readAll();
    if (data.isEmpty()) return;

    emit vnaDataReceived(data);
}

void NetworkManager::onVnaSocketDisconnected()
{
    if (!m_vnaSocket) return;

    emit logMessage(QString("[网络] 矢网连接已断开 %1").arg(peerInfo(m_vnaSocket)));

    cleanupSocket(m_vnaSocket);
    m_vnaSocket = nullptr;

    emit vnaDisconnected();
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
