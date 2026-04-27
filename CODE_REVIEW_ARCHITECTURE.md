# 相控阵天线地面测试系统 — 架构分析与提升建议

> 分析日期：2026-04-27
> 代码规模：约 15 个核心源文件（不含 QCustomPlot 第三方库）
> 技术栈：Qt 5.12.10 MinGW 32-bit

---

## 一、当前架构问题清单（按严重程度排序）

### P0 — 严重（架构级缺陷）

#### 1. MainWindow 承担过多职责（上帝类 / God Class）
**文件**: `mainwindow.h/cpp`

`MainWindow` 同时承担了以下角色：
- UI 主窗口控制器
- TCP 服务器管理器（监听、接受连接、客户端生命周期）
- TCP 客户端管理器（主动连接外部服务器）
- VNA 矢网仪器 TCP 连接管理器
- 数据帧解析调度器（环形缓冲区 + 协议解码）
- 业务逻辑分发器（心跳、遥测、遥控应答、设备状态、错误报告）
- 子窗口生命周期管理者
- 周期性指令定时器管理
- 日志系统

**后果**: 
- 代码行数超过 960 行，且仍在增长
- 任何网络层改动都需要修改主窗口
- 单元测试几乎不可能（需要实例化整个 MainWindow）
- 多人协作时冲突概率极高

#### 2. 网络层与业务层完全耦合
**文件**: `mainwindow.cpp`

TCP 数据的读取、帧解析、业务处理全部在 `MainWindow::onClientDataReady()` 中完成：
```cpp
void MainWindow::onClientDataReady() {
    // 1. 读取 socket 数据
    // 2. 写入环形缓冲区
    // 3. 调用 ProtocolCodec::parse()
    // 4. 调用 handleReceivedFrame()
    // 5. handleReceivedFrame 中直接操作 UI（m_antennaDeviceWindow->parseWaveControlTelemetry）
}
```

**后果**:
- 网络线程（Qt 的 socket 事件）与 UI 线程未做明确分离
- 数据解析直接在 socket readyRead 回调中执行，大数据量时可能阻塞 UI
- 无法支持后续的多线程数据预处理

#### 3. 三种 Socket 管理混乱（服务器/客户端/矢网仪器）
**文件**: `mainwindow.h`

```cpp
QTcpServer *m_tcpServer;           // 服务器模式
QVector<QTcpSocket*> m_clients;    // 服务器接受的客户端
QTcpSocket *m_clientSocket;        // 客户端模式（主动连接）
QTcpSocket *m_vnaSocket;           // 矢网仪器连接
```

- 三种连接的生命周期管理散落在不同函数中
- `m_clientBuffers` 只给服务器客户端和客户端模式的 socket 用，矢网仪器没有环形缓冲区
- 状态标志（`m_serverRunning`, `m_clientConnected`, `m_vnaConnected`）各自独立，容易不一致

#### 4. 子窗口生命周期管理不一致
**文件**: `mainwindow.cpp`

```cpp
// Tab 中的子窗口：无父对象（裸指针）
m_antennaDeviceWindow = new AntennaDeviceWindow();  // 无 parent！
electronicTab = new ElectronicDeviceWindow();        // 无 parent！
temperatureTab = new TemperatureInfoWindow();        // 无 parent！
powerTab = new PowerVoltageWindow();                 // 无 parent！

// 菜单弹窗子窗口：有父对象
m_autoTestWindow = new AutoTestWindow(this);         // 有 parent
m_instrumentControlWindow = new InstrumentControlWindow(this);  // 有 parent
```

**后果**:
- Tab 子窗口没有父对象，但也没有手动 `delete`，依赖程序退出时的操作系统回收（在长时间运行的测试系统中是隐患）
- 析构函数中只清理了网络和菜单窗口，没有清理 Tab 子窗口
- `AutoTestWindow` 在 `onActionAutoTestWindow()` 中延迟创建，但从未被 `delete`

---

### P1 — 高（设计级缺陷）

#### 5. 数据协议层职责边界不清
**文件**: `dataprotocol.h/cpp`

`ProtocolCodec` 只负责帧的打包/解析，但：
- 校验和计算与具体的校验算法（累加和）硬绑定
- 指令类型的合法性检查在解析层做（`switch(command)`），但业务层也有类似检查
- 没有抽象出"协议处理器"接口，新增指令需要同时修改 `dataprotocol.cpp` 和 `mainwindow.cpp`

#### 6. 指令构造代码大量重复
**文件**: `antennadevicewindow.cpp`, `temperatureinfowindow.cpp`

多处出现硬编码的指令构造：
```cpp
// antennadevicewindow.cpp
command.append(0xEB);
command.append(0x90);
command.append(0x03);
command.append(0xB2);
// ...

// temperatureinfowindow.cpp
command.append(static_cast<char>(0xEB));
command.append(static_cast<char>(0x90));
command.append(static_cast<char>(0x05));
command.append(static_cast<char>(0xAB));
```

**后果**:
- 帧头、帧类型等常量散落在业务代码中
- 修改协议格式需要全局搜索替换
- 极易出现手误导致通信故障

#### 7. 遥测数据解析与 UI 更新强耦合
**文件**: `mainwindow.cpp`, `antennadevicewindow.cpp`, `temperatureinfowindow.cpp`

```cpp
// mainwindow.cpp
void MainWindow::handleTelemetryData(...) {
    if (m_antennaDeviceWindow) {
        m_antennaDeviceWindow->parseWaveControlTelemetry(frame.payload);
    }
    if (temperatureTab) {
        temperatureTab->parseThermalTelemetry(frame.payload);
    }
}
```

- `parseWaveControlTelemetry` 和 `parseThermalTelemetry` 的解析逻辑与 UI 更新（`updateWaveControlTelemetry`, `updateTemperature`）在同一个函数调用链中
- 没有数据模型层，无法支持数据回放、数据导出、离线分析等功能

#### 8. 环形缓冲区存在线程安全问题
**文件**: `ringbuffer.h/cpp`

虽然 `RingBuffer` 声明了线程安全（`QMutex`），但实际使用场景是单线程的（Qt 的信号槽在主线程执行）。更严重的问题是：
- `readableBytes()` 同时加读锁和写锁，但 `const_cast<QMutex*>(&m_writeMutex)` 是危险操作
- 扩容逻辑 `ensureSpace()` 在写锁内执行，但可能触发内存重新分配，如果此时有读操作会崩溃
- 没有考虑 Qt 的事件循环与锁的交互（在 Qt 中通常不鼓励在 GUI 线程使用重锁）

#### 9. 自动测试流程状态机缺失
**文件**: `autotestwindow.cpp`

`AutoTestWindow` 用多个布尔变量维护状态：
```cpp
bool m_connected = false;
bool m_powerOn = false;
```

- 状态转换没有统一的管理，容易进入非法组合（如 `m_powerOn=true` 但 `m_connected=false`）
- 前置条件检查用 `if-else` 链实现，新增条件时代码膨胀
- 没有状态进入/退出的钩子，无法做统一日志或校验

---

### P2 — 中（代码级缺陷）

#### 10. 命名规范不统一
| 文件 | 类名 | UI 类名 | 问题 |
|------|------|---------|------|
| `antennadevicewindow.h` | `AntennaDeviceWindow` | `antennadevicewindow` | 类名首字母大写，UI 类名全小写 |
| `temperatureinfowindow.h` | `TemperatureInfoWindow` | `temperatureinfowindow` | 同上 |
| `electronicdevicewindow.h` | `ElectronicDeviceWindow` | `ElectronicDeviceWindow` | 已修复 |

- 部分文件使用驼峰命名，部分使用下划线
- 信号名风格不一致：`antennaPowerOnRequested` vs `sendRawCommandToServer`

#### 11. 内存管理隐患
- `new` 了大量 `QTableWidgetItem` 但没有 `delete`（Qt 的父子对象机制理论上会处理，但代码中未显式设置 parent）
- `m_autoTestWindow` 和 `m_instrumentControlWindow` 在 `MainWindow` 析构时没有被 `delete`
- `temperatureinfowindow.cpp` 中 `m_customPlot` 等组件设置了 parent，但 `m_replotThrottleTimer` 的 parent 是 `this`，而 `this` 是 `TemperatureInfoWindow`，析构时会自动处理

#### 12. 日志系统过于简单
- 所有日志直接输出到 `QPlainTextEdit`，没有分级（DEBUG/INFO/WARN/ERROR）
- 没有日志文件持久化
- 长时间运行后日志行数限制（5000行）可能导致关键信息丢失

#### 13. 异常处理不足
- 网络连接失败只有 `QMessageBox` 弹窗，没有重试机制
- 文件操作（CSV 加载、配置保存）没有异常回滚
- 协议解析失败只打印日志，没有统计或告警

---

## 二、推荐的目标架构

### 分层架构图

```
┌─────────────────────────────────────────────────────────────┐
│                        UI 层 (Presentation)                  │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │ MainWindow  │ │ AutoTestWin │ │ InstrumentControlWin│   │
│  │ (主窗口)     │ │ (自动测试)   │ │ (仪器控制)           │   │
│  └──────┬──────┘ └──────┬──────┘ └──────────┬──────────┘   │
│         │               │                    │              │
│  ┌──────┴──────┐ ┌──────┴──────┐ ┌──────────┴──────────┐   │
│  │AntennaDevice│ │Temperature  │ │   PowerVoltage      │   │
│  │   Window    │ │ Info Window │ │      Window         │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
└──────────────────────────┬──────────────────────────────────┘
                           │ 信号/槽 (Qt::QueuedConnection)
┌──────────────────────────┴──────────────────────────────────┐
│                      业务逻辑层 (Business)                     │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │TestWorkflow │ │TelemetryMgr │ │  InstrumentManager  │   │
│  │(测试状态机)  │ │(遥测数据管理)│ │   (仪器抽象接口)     │   │
│  └──────┬──────┘ └──────┬──────┘ └──────────┬──────────┘   │
│         │               │                    │              │
│  ┌──────┴──────┐ ┌──────┴──────┐ ┌──────────┴──────────┐   │
│  │  DataModel  │ │  DataModel  │ │    DataModel        │   │
│  │(天线遥测模型)│ │(温度遥测模型)│ │   (电源电压模型)     │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
└──────────────────────────┬──────────────────────────────────┘
                           │ 信号/槽 / 回调
┌──────────────────────────┴──────────────────────────────────┐
│                      服务层 (Service)                         │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐   │
│  │NetworkMgr   │ │ProtocolStack│ │   CommandBuilder    │   │
│  │(网络连接管理)│ │(协议编解码)  │ │   (指令构造工厂)     │   │
│  │             │ │             │ │                     │   │
│  │- TcpServer  │ │- FrameCodec │ │- buildPowerOn()     │   │
│  │- TcpClient  │ │- Checksum   │ │- buildTelemetryReq()│   │
│  │- VnaClient  │ │- Parser     │ │- ...                │   │
│  └─────────────┘ └─────────────┘ └─────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### 核心设计原则

1. **单一职责**：每个类只做一件事
2. **依赖倒置**：UI 层依赖业务层接口，不直接依赖服务层
3. **观察者模式**：数据更新通过信号/槽传递，避免直接调用
4. **状态机**：测试流程用显式状态机管理，替代布尔变量组合

---

## 三、分阶段重构路线图

### 第一阶段：紧急修复（1-2 天）
**目标**：消除 P0 级缺陷，防止系统崩溃

| 序号 | 任务 | 文件 | 改动点 |
|------|------|------|--------|
| 1.1 | 修复子窗口内存泄漏 | `mainwindow.cpp` | 给 Tab 子窗口添加 `this` 为 parent，或在析构中 `delete` |
| 1.2 | 统一 socket 状态管理 | `mainwindow.h/cpp` | 提取 `ConnectionState` 枚举，用状态机替代布尔变量 |
| 1.3 | 分离 VNA socket 管理 | `mainwindow.h/cpp` | 将 `m_vnaSocket` 相关逻辑提取到独立类 `VnaConnection` |
| 1.4 | 添加空指针检查 | `mainwindow.cpp` | 所有 `m_autoTestWindow` 调用前加空指针保护 |

### 第二阶段：网络层解耦（3-5 天）
**目标**：提取网络管理层，MainWindow 不再直接操作 socket

| 序号 | 任务 | 文件 | 改动点 |
|------|------|------|--------|
| 2.1 | 创建 `NetworkManager` 类 | 新增 | 统一管理 TCP Server/Client/VNA 三种连接 |
| 2.2 | 创建 `ConnectionBase` 抽象类 | 新增 | 定义连接接口：`connect()`/`disconnect()`/`send()`/`isConnected()` |
| 2.3 | 实现三种连接子类 | 新增 | `TcpServerConnection`/`TcpClientConnection`/`VnaConnection` |
| 2.4 | 迁移 MainWindow 网络逻辑 | `mainwindow.cpp` | 将 socket 操作迁移到 `NetworkManager`，MainWindow 只接收信号 |
| 2.5 | 环形缓冲区简化 | `ringbuffer.h/cpp` | 移除不必要的双锁，改为单线程使用（Qt 事件循环保证） |

### 第三阶段：业务层重构（5-7 天）
**目标**：建立数据模型层，解耦 UI 与数据解析

| 序号 | 任务 | 文件 | 改动点 |
|------|------|------|--------|
| 3.1 | 创建 `TelemetryDataModel` | 新增 | 统一遥测数据模型，支持天线/温度/电源多种数据 |
| 3.2 | 创建 `TestWorkflow` 状态机 | 新增 | 用 QStateMachine 管理自动测试流程 |
| 3.3 | 创建 `CommandBuilder` | 新增 | 统一指令构造，消除硬编码的 `append(0xEB)` |
| 3.4 | 迁移遥测解析逻辑 | `antennadevicewindow.cpp` | `parseWaveControlTelemetry` 改为更新 DataModel，UI 通过信号刷新 |
| 3.5 | 迁移温度解析逻辑 | `temperatureinfowindow.cpp` | 同上 |

### 第四阶段：质量提升（3-5 天）
**目标**：完善日志、异常处理、单元测试

| 序号 | 任务 | 文件 | 改动点 |
|------|------|------|--------|
| 4.1 | 引入分级日志系统 | 新增 `Logger` | 支持 DEBUG/INFO/WARN/ERROR，输出到文件 + UI |
| 4.2 | 添加网络重试机制 | `NetworkManager` | 连接失败自动重试 3 次，指数退避 |
| 4.3 | 添加协议统计 | `ProtocolStack` | 统计帧接收/丢弃/错误率 |
| 4.4 | 编写单元测试 | 新增 `tests/` | 对 `CommandBuilder`、`ProtocolCodec`、`TestWorkflow` 写测试 |
| 4.5 | 统一命名规范 | 全局 | 所有类名、函数名、变量名统一风格 |

---

## 四、关键重构示例

### 示例 1：提取 `NetworkManager`

**当前代码**（`mainwindow.h`）:
```cpp
QTcpServer *m_tcpServer;
QVector<QTcpSocket*> m_clients;
QTcpSocket *m_clientSocket;
QTcpSocket *m_vnaSocket;
bool m_serverRunning;
bool m_clientConnected;
bool m_vnaConnected;
```

**目标代码**:
```cpp
// networkmanager.h
class NetworkManager : public QObject {
    Q_OBJECT
public:
    explicit NetworkManager(QObject *parent = nullptr);
    
    // 服务器模式
    bool startServer(const QString &ip, quint16 port, int maxClients);
    void stopServer();
    
    // 客户端模式
    bool connectToServer(const QString &ip, quint16 port);
    void disconnectFromServer();
    
    // 矢网仪器
    bool connectToVna(const QString &ip, quint16 port);
    void disconnectFromVna();
    
    // 发送数据
    bool sendToServer(const QByteArray &data);
    bool sendToVna(const QByteArray &data);
    void broadcastToClients(const QByteArray &data);
    
signals:
    void serverClientConnected(const QString &peerInfo);
    void serverClientDisconnected(const QString &peerInfo);
    void serverDataReceived(const QByteArray &data);
    void clientConnected();
    void clientDisconnected();
    void clientDataReceived(const QByteArray &data);
    void vnaConnected();
    void vnaDisconnected();
    void vnaDataReceived(const QByteArray &data);
    void errorOccurred(const QString &error);
    
private:
    class Impl;
    std::unique_ptr<Impl> d;
};
```

### 示例 2：提取 `CommandBuilder`

**当前代码**（`antennadevicewindow.cpp`）:
```cpp
QByteArray command;
command.append(0xEB);
command.append(0x90);
command.append(0x01);
command.append(0xA1);
command.append(0x05);
emit sendRawCommandToServer(command);
```

**目标代码**:
```cpp
// commandbuilder.h
class CommandBuilder {
public:
    static QByteArray antennaPowerOn();
    static QByteArray antennaPowerOff();
    static QByteArray thermalPowerOn();
    static QByteArray thermalPowerOff();
    static QByteArray prfDownload(quint32 prf, quint32 totalCount, quint16 pulseWidth, quint8 protection);
    static QByteArray testParamDownload(quint8 waveCount, quint8 polarization, ...);
    // ...
};

// 使用
emit sendRawCommandToServer(CommandBuilder::antennaPowerOn());
```

### 示例 3：`TestWorkflow` 状态机

**当前代码**（`autotestwindow.cpp`）:
```cpp
bool canStartTest() const {
    return m_connected && !m_waveDirPath.isEmpty() && !m_configFilePath.isEmpty() && m_powerOn;
}
```

**目标代码**:
```cpp
// testworkflow.h
class TestWorkflow : public QStateMachine {
    Q_OBJECT
public:
    enum State {
        Idle,
        Connecting,
        Connected,
        WaveDirSelected,
        ConfigLoaded,
        PowerOn,
        Testing,
        Completed,
        Error
    };
    
    explicit TestWorkflow(QObject *parent = nullptr);
    
    void setVnaConnected(bool connected);
    void setWaveDir(const QString &dir);
    void setConfigFile(const QString &file);
    void setPowerOn(bool on);
    void startTest();
    void stopTest();
    
signals:
    void stateChanged(TestWorkflow::State state);
    void canStartChanged(bool canStart);
    void error(const QString &message);
    
private:
    State m_currentState;
};
```

---

## 五、实施建议

### 优先级
1. **立即执行**：P0 级修复（内存泄漏、socket 状态统一）
2. **本周执行**：P1 级中的 `CommandBuilder` 提取（风险低、收益高）
3. **下周执行**：`NetworkManager` 提取（需要充分测试）
4. **后续迭代**：`TestWorkflow` 状态机、日志系统、单元测试

### 风险控制
- **不要一次性重构所有代码**：采用"绞杀者模式"，逐步替换旧逻辑
- **保持可编译**：每次重构后确保项目能编译通过
- **保留旧代码备份**：重构前创建 Git 分支
- **充分测试网络通信**：重构后必须进行实际设备联调

### 团队协作
- 建议分配专人负责 `NetworkManager` 和 `CommandBuilder`
- UI 层的改动与业务层解耦后可以并行开发
- 建立代码审查机制，防止上帝类重新出现

---

## 六、总结

当前代码的主要问题是 **"所有逻辑都堆在 MainWindow"**，导致：
- 代码难以理解和维护
- 改动影响范围不可控
- 无法单元测试
- 多人协作困难

通过 **分层架构**（UI → 业务 → 服务）和 **职责分离**（网络、协议、指令、状态机各自独立），可以将代码质量提升到可维护、可测试、可扩展的水平。

建议按照 **"先止血（P0 修复）→ 再解耦（网络层提取）→ 再重构（业务层抽象）→ 最后完善（质量提升）"** 的顺序逐步推进。
