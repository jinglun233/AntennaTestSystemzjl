#include "autotestwindow.h"
#include "ui_autotestwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QHeaderView>
#include <QDir>

// ============================================================================
//                              构造 / 析构
// ============================================================================

AutoTestWindow::AutoTestWindow(QWidget *parent)
    : QWidget(parent, Qt::Window)
    , ui(new Ui::AutoTestWindow)
    , m_currentStep(TestStep::Idle)
    , m_connected(false)
    , m_powerOn(false)
    , m_testing(false)
    , m_currentChannelIndex(0)
    , m_vnaFilePath()
{
    ui->setupUi(this);
    ui->statusIconLabel->setPixmap(QPixmap(":/pic/redLED.png"));
    ui->statusTextLabel->setText("空闲");
    ui->startAutoTestButton->setEnabled(false);

    // 测试步骤定时器（单次触发，非阻塞延迟）
    m_testStepTimer = new QTimer(this);
    m_testStepTimer->setSingleShot(true);
    connect(m_testStepTimer, &QTimer::timeout, this, &AutoTestWindow::onTestStepTimer);
}

AutoTestWindow::~AutoTestWindow()
{
    if (m_testStepTimer) {
        m_testStepTimer->stop();
    }
    delete ui;
}

// ============================================================================
//                          UI 状态管理
// ============================================================================

void AutoTestWindow::updateTestStatus(bool running)
{
    if (running) {
        ui->statusIconLabel->setPixmap(QPixmap(":/pic/greenLED.png"));
        ui->statusTextLabel->setText("测试中");
    } else {
        ui->statusIconLabel->setPixmap(QPixmap(":/pic/redLED.png"));
        ui->statusTextLabel->setText("空闲");
    }
}

bool AutoTestWindow::canStartTest() const
{
    return m_connected && !m_waveDirPath.isEmpty() && !m_configFilePath.isEmpty() && m_powerOn;
}

void AutoTestWindow::updateStartButtonState()
{
    ui->startAutoTestButton->setEnabled(canStartTest() && !m_testing);
}

// ============================================================================
//                        UI 控件槽函数
// ============================================================================

void AutoTestWindow::on_connectButton_clicked()
{
    QString ip = ui->ipLineEdit->text().trimmed();

    if (ip.isEmpty()) {
        QMessageBox::warning(this, "提示", "请填写网络地址！");
        return;
    }

    if (m_connected) {
        emit disconnectFromHostRequested();
        return;
    }

    emit connectToHostRequested(ip, 5025);
}

void AutoTestWindow::onConnected()
{
    m_connected = true;
    ui->connectButton->setText("断开");
    appendResult(QString("[%1] 已连接到 %2:5025")
                 .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                 .arg(ui->ipLineEdit->text().trimmed()));
    updateStartButtonState();
}

void AutoTestWindow::onDisconnected()
{
    m_connected = false;
    ui->connectButton->setText("连接");
    appendResult(QString("[%1] 已断开连接")
                 .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
    updateStartButtonState();

    // 如果正在测试，强制停止
    if (m_testing) {
        stopAutoTestFlow();
        appendResult("[警告] 矢网连接丢失，测试已中止！");
    }
}

void AutoTestWindow::on_powerOnButton_clicked()
{
    m_powerOn = true;
    appendResult(QString("[%1] 天线加电")
                 .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
    updateStartButtonState();
    emit antennaPowerOnRequested();
}

void AutoTestWindow::on_powerOffButton_clicked()
{
    m_powerOn = false;
    appendResult(QString("[%1] 天线断电")
                 .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
    updateStartButtonState();
    emit antennaPowerOffRequested();
}

void AutoTestWindow::on_selectWaveDirButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择波位文件夹", m_waveDirPath);
    if (!dir.isEmpty()) {
        m_waveDirPath = dir;
        ui->waveCodeLineEdit->setText(dir);
        appendResult(QString("[%1] 已选择波位文件夹：%2")
                     .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                     .arg(dir));
        updateStartButtonState();
    }
}

void AutoTestWindow::on_selectConfigFileButton_clicked()
{
    QString file = QFileDialog::getOpenFileName(this, "选择配置文件", QString(),
                                                "CSV文件 (*.csv);;所有文件 (*)");
    if (!file.isEmpty()) {
        m_configFilePath = file;
        loadConfigCsv(file);
        appendResult(QString("[%1] 已选择配置文件：%2")
                     .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                     .arg(file));
        updateStartButtonState();
    }
}

// ============================================================================
//                     ★★★ 自动化测试流程入口 ★★★
// ============================================================================

/**
 * @brief 点击"开始自动化测试"按钮
 *
 * 流程概览：
 *   for each 通道 in configTableWidget第二列:
 *     1. 读取 波位文件夹/通道名.txt → 下发波控码（分页发送，每页间隔50ms）
 *     2. 等待 50ms
 *     3. 发送开始 PRF 指令
 *     4. 等待 50ms
 *     5. 矢网 SCPI 操作：
 *        a. :CALC:PAR:SEL + :CALC:PAR:MOD:EXT + :DISP:WIND:TRAC:FEED
 *        b. 创建保存目录 + 切换目录
 *        c. :CALC:DATA:SNP:PORTs:Save （保存 S2P 文件）
 *        d. :HCOPy:FILE （保存屏幕截图）
 */
void AutoTestWindow::on_startAutoTestButton_clicked()
{
    if (!canStartTest()) {
        QString msg;
        if (!m_connected) msg = "请先连接矢网仪器！";
        else if (m_waveDirPath.isEmpty()) msg = "请先选择波位文件夹！";
        else if (m_configFilePath.isEmpty()) msg = "请先选择配置文件！";
        else if (!m_powerOn) msg = "请先点击天线加电！";
        QMessageBox::warning(this, "提示", msg);
        return;
    }

    startAutoTestFlow();
}

// ============================================================================
//                   ★★★ 测试流程状态机实现 ★★★
// ============================================================================

void AutoTestWindow::startAutoTestFlow()
{
    m_testing = true;
    m_currentChannelIndex = 0;
    updateTestStatus(true);
    ui->startAutoTestButton->setEnabled(false);

    appendResult(QString("=").repeated(60));
    appendResult(QString("[%1] ===== 自动测试流程开始 =====")
                 .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));

    // 第一步：读取配置表
    advanceToStep(TestStep::ReadConfig);
}

void AutoTestWindow::stopAutoTestFlow()
{
    if (m_testStepTimer->isActive()) {
        m_testStepTimer->stop();
    }

    // 发送停止 PRF
    emit stopPrfRequested();

    m_testing = false;
    m_currentStep = TestStep::Idle;
    updateTestStatus(false);
    updateStartButtonState();

    appendResult(QString("[%1] ===== 自动测试流程停止 =====")
                 .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
    appendResult(QString("=").repeated(60));
}

/**
 * @brief 定时器回调：执行当前步骤
 *
 * 这是状态机的核心驱动。每步执行完后通过 scheduleNextStep() 安排下一步，
 * 使用 QTimer::singleShot 实现非阻塞延迟。
 */
void AutoTestWindow::onTestStepTimer()
{
    executeCurrentStep();
}

void AutoTestWindow::executeCurrentStep()
{
    switch (m_currentStep) {
    case TestStep::Idle:
        break;

    case TestStep::ReadConfig:
        doReadConfig();
        break;

    case TestStep::DownloadWaveCode:
        doDownloadWaveCode();
        break;

    case TestStep::WaitAfterWaveCode:
        // 延迟后进入 StartPrf
        advanceToStep(TestStep::StartPrf);
        break;

    case TestStep::StartPrf:
        appendResult("  [步骤] 发送开始 PRF 指令...");
        emit startPrfRequested();
        scheduleNextStep(TestStep::WaitAfterPrf, 50);
        break;

    case TestStep::WaitAfterPrf:
        // 延迟后进入矢网测量
        advanceToStep(TestStep::VnaMeasure);
        break;

    case TestStep::VnaMeasure:
        doVnaMeasure();
        break;

    case TestStep::VnaSaveFile:
        doVnaSaveFile();
        break;

    case TestStep::VnaSaveScreen:
        doVnaSaveScreen();
        break;

    case TestStep::NextChannel:
        m_currentChannelIndex++;
        if (m_currentChannelIndex < m_channelList.size()) {
            // 还有下一通道，继续下发波控码
            m_currentChannelName = m_channelList.at(m_currentChannelIndex);
            appendResult("");
            appendResult(QString("--- 通道 %1/%2: %3 ---")
                         .arg(m_currentChannelIndex + 1)
                         .arg(m_channelList.size())
                         .arg(m_currentChannelName));
            advanceToStep(TestStep::DownloadWaveCode);
        } else {
            // 所有通道完成，发送停止 PRF 并结束
            advanceToStep(TestStep::StopPrf);
        }
        break;

    case TestStep::StopPrf:
        appendResult("  [步骤] 发送停止 PRF 指令...");
        emit stopPrfRequested();
        scheduleNextStep(TestStep::Complete, 100);
        break;

    case TestStep::Complete:
        m_testing = false;
        m_currentStep = TestStep::Idle;
        updateTestStatus(false);
        updateStartButtonState();
        appendResult("");
    appendResult(QString("[%1] ===== 自动测试流程全部完成 (%2 个通道) =====")
                 .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                 .arg(m_channelList.size()));
    appendResult(QString("=").repeated(60));
        break;
    }
}

void AutoTestWindow::advanceToStep(TestStep step)
{
    m_currentStep = step;
    executeCurrentStep();
}

void AutoTestWindow::scheduleNextStep(TestStep nextStep, int delayMs)
{
    m_currentStep = nextStep;
    m_testStepTimer->start(delayMs);  // 单次触发，delayMs 后调用 onTestStepTimer()
}

// ============================================================================
//                      各步骤的具体实现
// ============================================================================

/**
 * @brief 步骤1：从 configTableWidget 第一列读取所有波控码文件名
 *
 * 配置表第一列格式示例："CS01HR\模块1组件1通道1"
 * 波控码文件路径 = waveCodeLineEdit路径 / 第一列文本
 */
void AutoTestWindow::doReadConfig()
{
    m_channelList.clear();

    int rowCount = ui->configTableWidget->rowCount();
    if (rowCount <= 0) {
        appendResult("[错误] 配置表为空，无法启动测试！");
        stopAutoTestFlow();
        return;
    }

    // 读取第一列（索引=0）的每个单元格作为波控码文件名
    for (int row = 0; row < rowCount; ++row) {
        QTableWidgetItem *item = ui->configTableWidget->item(row, 0);
        if (item && !item->text().trimmed().isEmpty()) {
            m_channelList.append(item->text().trimmed());
        }
    }

    if (m_channelList.isEmpty()) {
        appendResult("[错误] 配置表第一列为空！");
        stopAutoTestFlow();
        return;
    }

    m_currentChannelIndex = 0;
    m_currentChannelName = m_channelList.at(0);

    appendResult(QString("[配置] 共 %1 个通道").arg(m_channelList.size()));
    appendResult(QString("--- 通道 1/%1: %2 ---")
                 .arg(m_channelList.size())
                 .arg(m_currentChannelName));

    // 进入波控码下发阶段
    scheduleNextStep(TestStep::DownloadWaveCode, 10);
}

/**
 * @brief 步骤2：读取并下发当前通道的波控码文件（异步）
 *
 * 文件路径 = waveCodeLineEdit路径 / 当前通道名称
 * 例如：d:\CS01HR\模块1组件1通道1
 *
 * 通过信号 waveCodeDownloadRequested 将文件路径传递给 AntennaDeviceWindow，
 * AntennaDeviceWindow 异步分页发送，完成后通过 waveCodeDownloadCompleted 信号
 * 通知本窗口继续推进到 WaitAfterWaveCode。
 */
void AutoTestWindow::doDownloadWaveCode()
{
    if (m_waveDirPath.isEmpty() || m_currentChannelName.isEmpty()) {
        appendResult("[错误] 波位文件夹或通道名为空！");
        stopAutoTestFlow();
        return;
    }

    // 构造完整路径：根目录 / 通道名
    // 配置表中的通道名可能含反斜杠（如 "CS01HR\模块1组件1通道1"），
    // 这是文件名的一部分，必须保留原样。
    // 用 QDir::cleanPath 确保根目录部分以分隔符结尾，然后直接拼文件名。
    QString dirPart = QDir::cleanPath(m_waveDirPath);
    if (!dirPart.endsWith('/') && !dirPart.endsWith('\\')) {
        dirPart += '/';
    }
    QString fullPath = dirPart + m_currentChannelName;

    // 检查文件是否存在
    if (!QFile::exists(fullPath)) {
        // 尝试加上 .txt 后缀
        QString txtPath = fullPath + ".txt";
        if (QFile::exists(txtPath)) {
            fullPath = txtPath;
        } else {
            appendResult(QString("[错误] 波控码文件不存在: %1").arg(fullPath));
            stopAutoTestFlow();
            return;
        }
    }

    appendResult(QString("  [步骤] 下发波控码: %1").arg(fullPath));

    // 发出请求信号（AntennaDeviceWindow 接收后开始异步分页发送）
    emit waveCodeDownloadRequested(fullPath);

    // ★ 不再固定等待 ★ 状态机停在 DownloadWaveCode，
    // 等待 onWaveCodeDownloadCompleted 信号到达后再推进
}

/**
 * @brief 波控码异步下发完成回调
 *
 * 由 AntennaDeviceWindow::waveCodeDownloadCompleted 信号触发。
 * 收到后推进状态机到 WaitAfterWaveCode → StartPrf。
 */
void AutoTestWindow::onWaveCodeDownloadCompleted(bool ok, int totalPages)
{
    if (!m_testing || m_currentStep != TestStep::DownloadWaveCode) {
        return;  // 不在测试流程中，忽略
    }

    if (ok) {
        appendResult(QString("  [完成] 波控码下发完毕 (%1 页)").arg(totalPages));
        scheduleNextStep(TestStep::WaitAfterWaveCode, 50);
    } else {
        appendResult("[错误] 波控码下发失败！");
        stopAutoTestFlow();
    }
}

/**
 * @brief 步骤5a：矢网测量操作 — 选择参数+设置模式+显示轨迹
 *
 * 对应 C# 的 Sendcommend() 方法核心逻辑：
 *   1. :CALC1:PAR:CAT? — 查询已有轨迹
 *   2. :CALC:PAR:SEL 'Tr_S32' — 选择轨迹
 *   3. :CALC:PAR:MOD:EXT 'S12' — 设置测量模式
 *   4. :DISP:WIND:TRAC1:FEED 'Tr_S32' — 显示轨迹
 */
void AutoTestWindow::doVnaMeasure()
{
    QString currentTime = QDateTime::currentDateTime().toString("yyyyMMddHH");

    // 提取通道名中的关键信息作为 name 参数（如 "CS01HR-1"）
    QString name = m_currentChannelName;
    name.replace("\\", "-");  // 替换反斜杠为横杠

    appendResult(QString("  [矢网] 开始测量: %1").arg(m_currentChannelName));

    // --- SCPI 指令序列（对应 C# Sendcommend）---

    // 1. 查询已有轨迹参数
    emit vnaScpiCommandRequested(":CALC1:PAR:CAT?\n");

    // 2. 选择轨迹（Tr_S32 为默认值，实际应从查询结果解析）
    emit vnaScpiCommandRequested(":CALC:PAR:SEL 'Tr_S32'\n");

    // 3. 设置测量模式为 S12 参数（可根据需要改为 S11/S21/S22 等）
    emit vnaScpiCommandRequested(":CALC:PAR:MOD:EXT 'S12'\n");

    // 4. 显示该轨迹到窗口
    emit vnaScpiCommandRequested(":DISP:WIND:TRAC1:FEED 'Tr_S32'\n");

    // 进入保存文件阶段
    scheduleNextStep(TestStep::VnaSaveFile, 300);  // 等待矢网稳定
}

/**
 * @brief 步骤5b：矢网保存 S2P 文件
 *
 * 对应 C# 的 SaveVnaFile() 方法：
 *   1. :MMEM:MDIR — 创建时间子目录
 *   2. :MMEM:CDIR — 切换目录
 *   3. :CALC:DATA:SNP:PORTs:Save — 保存 S2P 数据
 */
void AutoTestWindow::doVnaSaveFile()
{
    QString currentTime = QDateTime::currentDateTime().toString("yyyyMMddHH");

    // 构造文件名：CS01HR-S12-模块1组件1通道1
    QString baseName = m_currentChannelName.replace("\\", "-");
    QString fileName = QString("%1-S12-%2.s2p").arg(baseName.split("-").first()).arg(baseName);

    appendResult(QString("  [矢网] 保存 S2P 文件..."));

    // 1. 创建以当前时间命名的子目录
    emit vnaScpiCommandRequested(
        QString(":MMEM:MDIR '%1'\n").arg(currentTime));

    // 2. 切换到该目录
    emit vnaScpiCommandRequested(
        QString(":MMEM:CDIR '%1'\n").arg(currentTime));

    // 3. 重新选择轨迹确保在正确上下文
    emit vnaScpiCommandRequested(":CALC:PAR:SEL 'Tr_S32'\n");

    // 4. 保存 S2P 文件（端口1,2）
    emit vnaScpiCommandRequested(
        QString(":CALC:DATA:SNP:PORTs:Save '1,2', '%1'\n").arg(fileName));

    // 进入截图阶段
    scheduleNextStep(TestStep::VnaSaveScreen, 500);
}

/**
 * @brief 步骤5c：矢网保存屏幕截图
 *
 * 对应 C# 的 :HCOPy:FILE 指令
 */
void AutoTestWindow::doVnaSaveScreen()
{
    QString currentTime = QDateTime::currentDateTime().toString("yyyyMMddHH");
    QString baseName = m_currentChannelName;
    baseName.replace("\\", "-");
    QString screenFileName = QString("%1-S12-%2.jpg")
                               .arg(baseName.split("-").first())
                               .arg(baseName.split("-", QString::SkipEmptyParts).last());

    emit vnaScpiCommandRequested(
        QString(":HCOPy:FILE '%1/%2/%3'\n")
        .arg(m_vnaFilePath.isEmpty() ? "." : m_vnaFilePath)
        .arg(currentTime)
        .arg(screenFileName));

    appendResult(QString("  [完成] 通道 %1 测试完成").arg(m_currentChannelName));

    // 进入下一通道
    scheduleNextStep(TestStep::NextChannel, 200);
}

// ============================================================================
//                            辅助方法
// ============================================================================

void AutoTestWindow::loadConfigCsv(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "提示", "无法打开配置文件！");
        return;
    }

    QTextStream in(&file);
    QStringList headers;
    QList<QStringList> rows;

    bool firstLine = true;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        QStringList cols = line.split(",");
        if (firstLine) {
            headers = cols;
            firstLine = false;
        } else {
            rows.append(cols);
        }
    }
    file.close();

    // 隐藏第一列（序号列），用行号代替
    if (!headers.isEmpty()) {
        headers.removeFirst();
    }
    for (auto &row : rows) {
        if (!row.isEmpty()) row.removeFirst();
    }

    ui->configTableWidget->clear();
    ui->configTableWidget->setColumnCount(headers.size());
    ui->configTableWidget->setHorizontalHeaderLabels(headers);
    ui->configTableWidget->setRowCount(rows.size());

    for (int r = 0; r < rows.size(); ++r) {
        for (int c = 0; c < rows[r].size() && c < headers.size(); ++c) {
            ui->configTableWidget->setItem(r, c, new QTableWidgetItem(rows[r][c].trimmed()));
        }
    }

    ui->configTableWidget->horizontalHeader()->setStretchLastSection(true);
    ui->configTableWidget->resizeColumnsToContents();
}

void AutoTestWindow::appendResult(const QString &text)
{
    ui->resultPlainTextEdit->appendPlainText(text);
}
