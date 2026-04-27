#ifndef AUTOTESTWINDOW_H
#define AUTOTESTWINDOW_H

#include <QWidget>
#include <QTimer>
#include <QStringList>

namespace Ui {
class AutoTestWindow;
}

/**
 * @brief 自动化测试窗口
 *
 * 测试流程（状态机驱动，QTimer 非阻塞延迟）：
 *   1. 读取 configTableWidget 第二列（通道名称）
 *   2. 根据 waveCodeLineEdit 路径 + 通道名 找到对应波控码文件
 *   3. 下发波控码（间隔50ms）
 *   4. 发送开始 PRF 指令（间隔50ms）
 *   5. 矢网 SCPI 操作（测量+保存 S2P 文件+截图）
 */
class AutoTestWindow : public QWidget
{
    Q_OBJECT

public:
    explicit AutoTestWindow(QWidget *parent = nullptr);
    ~AutoTestWindow();

signals:
    // ========== 连接控制 ==========
    void antennaPowerOnRequested();      // 请求天线加电
    void antennaPowerOffRequested();     // 请求天线断电
    void connectToHostRequested(const QString &ip, quint16 port);   // 请求连接矢网仪器
    void disconnectFromHostRequested();  // 请求断开矢网仪器

    // ========== 自动测试流程信号 ==========
    /**
     * @brief 请求从指定路径下发波控码（单个文件或文件夹）
     * @param filePath 波控码 txt 文件的完整路径
     */
    void waveCodeDownloadRequested(const QString &filePath);

    /**
     * @brief 请求发送开始 PRF 指令
     */
    void startPrfRequested();

    /**
     * @brief 请求发送停止 PRF 指令
     */
    void stopPrfRequested();

    /**
     * @brief 向矢网仪器发送 SCPI 命令（原始字符串）
     * @param scpiCommand SCPI 命令文本（如 ":CALC:PAR:SEL 'Tr_S32'\n"）
     */
    void vnaScpiCommandRequested(const QString &scpiCommand);

public slots:
    void onConnected();     // 矢网连接成功回调
    void onDisconnected();  // 矢网断开回调

private slots:
    // UI 控件槽函数
    void on_connectButton_clicked();
    void on_powerOnButton_clicked();
    void on_powerOffButton_clicked();
    void on_selectWaveDirButton_clicked();
    void on_selectConfigFileButton_clicked();
    void on_startAutoTestButton_clicked();

    // ★★★ 测试流程状态机驱动 ★★★
    void onTestStepTimer();          // 定时器回调：执行下一步

private:
    // ========== UI 辅助 ==========
    void appendResult(const QString &text);
    void updateTestStatus(bool running);
    void loadConfigCsv(const QString &filePath);
    bool canStartTest() const;
    void updateStartButtonState();

    // ========== 测试流程核心 ==========
    enum class TestStep {
        Idle,               // 空闲
        ReadConfig,         // 读取配置表
        DownloadWaveCode,   // 下发波控码
        WaitAfterWaveCode,  // 波控码下发后等待 50ms
        StartPrf,           // 发送开始PRF指令
        WaitAfterPrf,       // PRF启动后等待 50ms
        VnaMeasure,         // 矢网测量操作
        VnaSaveFile,        // 矢网保存文件
        VnaSaveScreen,      // 矢网保存截图
        NextChannel,        // 处理下一通道
        StopPrf,            // 发送停止PRF指令
        Complete            // 完成
    };

    void startAutoTestFlow();              // 启动测试流程
    void stopAutoTestFlow();               // 停止测试流程
    void executeCurrentStep();             // 执行当前步骤
    void advanceToStep(TestStep step);     // 进入下一步骤
    void scheduleNextStep(TestStep nextStep, int delayMs = 50);  // 安排下一步（带延迟）

    // 步骤实现
    void doReadConfig();                   // 读取配置表获取通道列表
    void doDownloadWaveCode();             // 读取并下发波控码
    void doVnaMeasure();                  // 矢网测量（SCPI命令序列）
    void doVnaSaveFile();                 // 矢网保存S2P文件
    void doVnaSaveScreen();               // 矢网保存截图

    // 辅助：读取单个波控码文件
    QByteArray readWaveCodeFile(const QString &filePath);

    Ui::AutoTestWindow *ui;

    // ========== 流程状态变量 ==========
    TestStep m_currentStep;               // 当前测试步骤
    QTimer *m_testStepTimer;              // 非阻塞定时器驱动步骤
    QStringList m_channelList;            // 从配置表第二列提取的通道名列表
    int m_currentChannelIndex;            // 当前处理的通道索引
    QString m_currentChannelName;         // 当前通道名称
    QString m_waveDirPath;                // 波位文件夹根目录
    QString m_configFilePath;             // 配置文件路径
    bool m_connected;                     // 矢网是否已连接
    bool m_powerOn;                       // 天线是否已加电
    bool m_testing;                       // 是否正在测试中
};

#endif // AUTOTESTWINDOW_H
