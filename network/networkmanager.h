/****************************************************************************
** 网络管理层 — 统一封装 TCP Server / Client / Instru 三种连接
**
** 设计原则：
**   1. MainWindow 只持有 NetworkManager，不直接操作 QTcpSocket
**   2. 子窗口通过 NetworkManager 的信号接收数据，通过槽发送数据
**   3. 所有 socket 生命周期由 NetworkManager 内部管理
****************************************************************************/
#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QVector>
#include <QMap>
#include <QFile>
#include <memory>

// 前向声明
class RingBuffer;

/**
 * @brief 连接类型枚举
 */
enum class ConnectionType {
    ServerClient,   // 服务器接受的客户端
    Client,         // 主界面客户端模式（主动连接外部服务器）
    Instru          // 仪器连接
};

/**
 * @brief 网络连接描述符
 *
 * 封装一个 QTcpSocket 及其关联的环形缓冲区，
 * 对外隐藏 socket 指针细节。
 */
struct ConnectionHandle {
    int id;                 // 连接唯一ID
    ConnectionType type;    // 连接类型
    QString peerInfo;       // 对端地址信息
    bool connected;         // 是否已连接

    ConnectionHandle() : id(-1), type(ConnectionType::ServerClient), connected(false) {}
};

/**
 * @brief 网络管理器
 *
 * 统一管理三种 TCP 连接：
 *   - 服务器模式：监听端口，接受多个客户端
 *   - 客户端模式：主动连接外部服务器（主界面使用）
 *   - 仪器模式：连接仪器（AutoTestWindow 使用）
 */
class NetworkManager : public QObject
{
    Q_OBJECT

public:
    explicit NetworkManager(QObject *parent = nullptr);
    ~NetworkManager();

    // ========== 服务器模式 ==========
    bool startServer(const QString &ip, quint16 port, int maxClients);
    void stopServer();
    bool isServerRunning() const;
    QVector<ConnectionHandle> serverClients() const;

    // ========== 客户端模式 ==========
    bool connectToServer(const QString &ip, quint16 port);
    void disconnectFromServer();
    bool isClientConnected() const;

    // ========== 仪器模式 ==========
    bool connectToInstru(const QString &ip, quint16 port);
    void disconnectFromInstru();
    bool isInstruConnected() const;

    // ========== 数据发送 ==========
    bool sendToServer(const QByteArray &data);
    bool sendToInstru(const QByteArray &data);
    void broadcastToClients(const QByteArray &data);

    // ========== 查询 ==========
    ConnectionHandle clientHandle() const;   // 客户端模式连接信息
    ConnectionHandle instruHandle() const;   // 仪器连接信息

signals:
    // 服务器事件
    void serverClientConnected(const ConnectionHandle &handle);
    void serverClientDisconnected(const ConnectionHandle &handle);
    void serverDataReceived(const ConnectionHandle &handle, const QByteArray &data);

    // 客户端事件
    void clientConnected();
    void clientDisconnected();
    void clientDataReceived(const QByteArray &data);

    // 仪器事件
    void instruConnected();
    void instruDisconnected();
    void instruDataReceived(const QByteArray &data);

    // 通用错误
    void errorOccurred(const QString &error);

    // 日志（供 MainWindow 统一输出）
    void logMessage(const QString &msg);

private slots:
    void onNewConnection();
    void onServerClientDisconnected();
    void onServerClientReadyRead();

    void onClientSocketReadyRead();
    void onClientSocketDisconnected();

    void onInstruSocketReadyRead();
    void onInstruSocketDisconnected();

private:
    // 内部辅助
    void setupSocketConnections(QTcpSocket *socket, ConnectionType type);
    void cleanupSocket(QTcpSocket *socket);
    QString peerInfo(QTcpSocket *socket) const;

    // 接收数据日志
    void logReceivedData(const QByteArray &data, const QString &typeLabel);
    void openLogForToday();                              // 打开/切换当日接收日志文件

    // 指令发送日志（与接收日志独立文件）
    void logSentData(const QByteArray &data, const QString &typeLabel);
    void openCommandLogForToday();                        // 打开/切换当日发送日志文件

    // 服务器
    QTcpServer *m_tcpServer;
    QVector<QTcpSocket*> m_serverClients;
    QMap<QTcpSocket*, int> m_clientIds;          // socket -> id
    QMap<int, QTcpSocket*> m_idToSocket;         // id -> socket
    QMap<QTcpSocket*, std::shared_ptr<RingBuffer>> m_serverBuffers;
    int m_nextClientId;
    int m_maxClients;

    // 客户端
    QTcpSocket *m_clientSocket;
    std::shared_ptr<RingBuffer> m_clientBuffer;

    // 仪器
    QTcpSocket *m_instruSocket;

    // ========== 接收数据日志 ==========
    QFile *m_logFile;              // 接收数据日志文件
    bool m_logFileOpened;          // 日志文件是否已打开
    QString m_logDate;             // 当前接收日志文件的日期（用于每日轮转）

    // ========== 指令发送日志 ==========
    QFile *m_commandLogFile;       // 指令发送日志文件
    bool m_commandLogOpened;       // 发送日志是否已打开
    QString m_commandLogDate;      // 当前发送日志文件的日期（用于每日轮转）
};

#endif // NETWORKMANAGER_H
