#include "antennadevicewindow.h"
#include "ui_antennadevicewindow.h"
#include <QResizeEvent>
#include <QDataStream>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QMessageBox>
#include <QThread>
#include <QtMath>

AntennaDeviceWindow::AntennaDeviceWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::antennadevicewindow)
    , m_wave2DFilePath()
    , m_basicParamFilePath()
    , m_layoutCtrlFilePath()
{
    ui->setupUi(this);

    initTelemetryTables();

    connect(ui->prfValueLineEdit, &QLineEdit::textChanged, this, &AntennaDeviceWindow::updateDutyCycle);
    connect(ui->pulseWidthLineEdit, &QLineEdit::textChanged, this, &AntennaDeviceWindow::updateDutyCycle);

    // 布控模式切换时控制全计算相关控件的可用状态
    connect(ui->bukongSettingComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AntennaDeviceWindow::onBukongSettingChanged);
    onBukongSettingChanged(ui->bukongSettingComboBox->currentIndex());
}

AntennaDeviceWindow::~AntennaDeviceWindow()
{
    delete ui;
}

void AntennaDeviceWindow::updateDutyCycle()
{
    bool ok1, ok2;
    double prf = ui->prfValueLineEdit->text().toDouble(&ok1);
    double pulseWidth = ui->pulseWidthLineEdit->text().toDouble(&ok2);

    if (ok1 && ok2 && prf > 0) {
        double dutyCycle = (pulseWidth * prf) / 10000.0;
        ui->dutyCycleLineEdit->setText(QString::number(dutyCycle, 'f', 2));
    } else {
        ui->dutyCycleLineEdit->setText("");
    }
}

void AntennaDeviceWindow::onBukongSettingChanged(int index)
{
    bool fullCompute = (index != 0);  // true=全计算模式, false=二维波控模式

    // 二维波控控件：仅在二维波控模式(index==0)下可用
    ui->wave2DFileButton->setEnabled(!fullCompute);
    ui->wave2DFilePathEdit->setEnabled(!fullCompute);
    ui->wave2DUploadButton->setEnabled(!fullCompute);

    // 全计算控件：仅在全计算模式(index!=0)下可用
    ui->basicParamFileButton->setEnabled(fullCompute);
    ui->basicParamFilePathEdit->setEnabled(fullCompute);
    ui->basicParamUploadButton->setEnabled(fullCompute);
    ui->layoutCtrlFileButton->setEnabled(fullCompute);
    ui->layoutCtrlFilePathEdit->setEnabled(fullCompute);
    ui->layoutCtrlUploadButton->setEnabled(fullCompute);
}

void AntennaDeviceWindow::initTelemetryTables()
{
    QStringList names1 = { "波控1 +5V1", "波控1 -5V1", "波控1 +5V2", "波控1 -5V2",
        "波控1 28V1", "波控1 28V2", "波控1 TR组件1", "波控1 TR组件2",
        "波控1 TR组件3", "波控1 TR组件4", "波控1 TR组件5", "波控1 TR组件6",
        "波控1 TR组件7", "波控1 TR组件8", "波控1 数字遥测" };
    QStringList names2 = { "波控2 +5V1", "波控2 -5V1", "波控2 +5V2", "波控2 -5V2", "波控2 28V1", "波控2 28V2", "波控2 TR组件1", "波控2 TR组件2", "波控2 TR组件3", "波控2 TR组件4", "波控2 TR组件5", "波控2 TR组件6", "波控2 TR组件7", "波控2 TR组件8", "波控2 数字遥测" };
    QStringList names3 = { "波控3 +5V1", "波控3 -5V1", "波控3 +5V2", "波控3 -5V2", "波控3 28V1", "波控3 28V2", "波控3 TR组件1", "波控3 TR组件2", "波控3 TR组件3", "波控3 TR组件4", "波控3 TR组件5", "波控3 TR组件6", "波控3 TR组件7", "波控3 TR组件8", "波控3 数字遥测" };
    QStringList names4 = { "波控4 +5V1", "波控4 -5V1", "波控4 +5V2", "波控4 -5V2", "波控4 28V1", "波控4 28V2", "波控4 TR组件1", "波控4 TR组件2", "波控4 TR组件3", "波控4 TR组件4", "波控4 TR组件5", "波控4 TR组件6", "波控4 TR组件7", "波控4 TR组件8", "波控4 数字遥测" };
    QStringList names5 = { "波控5 +5V1", "波控5 -5V1", "波控5 +5V2", "波控5 -5V2", "波控5 28V1", "波控5 28V2", "波控5 TR组件1", "波控5 TR组件2", "波控5 TR组件3", "波控5 TR组件4", "波控5 TR组件5", "波控5 TR组件6", "波控5 TR组件7", "波控5 TR组件8", "波控5 数字遥测" };
    QStringList names6 = { "波控6 +5V1", "波控6 -5V1", "波控6 +5V2", "波控6 -5V2", "波控6 28V1", "波控6 28V2", "波控6 TR组件1", "波控6 TR组件2", "波控6 TR组件3", "波控6 TR组件4", "波控6 TR组件5", "波控6 TR组件6", "波控6 TR组件7", "波控6 TR组件8", "波控6 数字遥测" };
    QStringList names7 = { "波控7 +5V1", "波控7 -5V1", "波控7 +5V2", "波控7 -5V2", "波控7 28V1", "波控7 28V2", "波控7 TR组件1", "波控7 TR组件2", "波控7 TR组件3", "波控7 TR组件4", "波控7 TR组件5", "波控7 TR组件6", "波控7 TR组件7", "波控7 TR组件8", "波控7 数字遥测" };
    QStringList names8 = { "波控8 +5V1", "波控8 -5V1", "波控8 +5V2", "波控8 -5V2", "波控8 28V1", "波控8 28V2", "波控8 TR组件1", "波控8 TR组件2", "波控8 TR组件3", "波控8 TR组件4", "波控8 TR组件5", "波控8 TR组件6", "波控8 TR组件7", "波控8 TR组件8", "波控8 数字遥测" };
    QStringList names9 = { "波控9 +5V1", "波控9 -5V1", "波控9 +5V2", "波控9 -5V2", "波控9 28V1", "波控9 28V2", "波控9 TR组件1", "波控9 TR组件2", "波控9 TR组件3", "波控9 TR组件4", "波控9 TR组件5", "波控9 TR组件6", "波控9 TR组件7", "波控9 TR组件8", "波控9 数字遥测" };
    QStringList names10 = { "波控10 +5V1", "波控10 -5V1", "波控10 +5V2", "波控10 -5V2", "波控10 28V1", "波控10 28V2", "波控10 TR组件1", "波控10 TR组件2", "波控10 TR组件3", "波控10 TR组件4", "波控10 TR组件5", "波控10 TR组件6", "波控10 TR组件7", "波控10 TR组件8", "波控10 数字遥测" };
    QStringList names11 = { "波控11 +5V1", "波控11 -5V1", "波控11 +5V2", "波控11 -5V2", "波控11 28V1", "波控11 28V2", "波控11 TR组件1", "波控11 TR组件2", "波控11 TR组件3", "波控11 TR组件4", "波控11 TR组件5", "波控11 TR组件6", "波控11 TR组件7", "波控11 TR组件8", "波控11 数字遥测" };
    QStringList names12 = { "波控12 +5V1", "波控12 -5V1", "波控12 +5V2", "波控12 -5V2", "波控12 28V1", "波控12 28V2", "波控12 TR组件1", "波控12 TR组件2", "波控12 TR组件3", "波控12 TR组件4", "波控12 TR组件5", "波控12 TR组件6", "波控12 TR组件7", "波控12 TR组件8", "波控12 数字遥测" };

    setupTelemetryTable(ui->telemetryTable1_1, names1);
    setupTelemetryTable(ui->telemetryTable1_2, names2);
    setupTelemetryTable(ui->telemetryTable1_3, names3);
    setupTelemetryTable(ui->telemetryTable1_4, names4);
    setupTelemetryTable(ui->telemetryTable1_5, names5);
    setupTelemetryTable(ui->telemetryTable1_6, names6);
    setupTelemetryTable(ui->telemetryTable2_1, names7);
    setupTelemetryTable(ui->telemetryTable2_2, names8);
    setupTelemetryTable(ui->telemetryTable2_3, names9);
    setupTelemetryTable(ui->telemetryTable2_4, names10);
    setupTelemetryTable(ui->telemetryTable2_5, names11);
    setupTelemetryTable(ui->telemetryTable2_6, names12);
}

void AntennaDeviceWindow::setupTelemetryTable(QTableWidget* table, const QStringList& names)
{
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);
    table->setStyleSheet("QTableWidget { alternate-background-color: #F2F2F2; }");
    table->setRowCount(15);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    table->setTextElideMode(Qt::ElideNone);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    table->setColumnWidth(0, 100);
    table->setColumnWidth(1, 67);

    int rowHeight = 30;
    for (int i = 0; i < 15; i++) {
        table->setRowHeight(i, rowHeight);
        QTableWidgetItem *nameItem = new QTableWidgetItem(names.at(i));
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(i, 0, nameItem);

        QTableWidgetItem *valueItem = new QTableWidgetItem("");
        valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
        valueItem->setTextAlignment(Qt::AlignCenter);
        valueItem->setForeground(QColor("#333333"));
        table->setItem(i, 1, valueItem);
    }
}

void AntennaDeviceWindow::updateAntennaTelemetry(double angle, double speed, int status, double accuracy)
{
    Q_UNUSED(angle);
    Q_UNUSED(speed);
    Q_UNUSED(status);
    Q_UNUSED(accuracy);
}

void AntennaDeviceWindow::updateWaveControlTelemetry(int waveId, const QVector<QString> &values)
{
    if (waveId < 1 || waveId > 12) return;

    QTableWidget *table = nullptr;
    if (waveId <= 6) {
        switch (waveId) {
        case 1: table = ui->telemetryTable1_1; break;
        case 2: table = ui->telemetryTable1_2; break;
        case 3: table = ui->telemetryTable1_3; break;
        case 4: table = ui->telemetryTable1_4; break;
        case 5: table = ui->telemetryTable1_5; break;
        case 6: table = ui->telemetryTable1_6; break;
        }
    } else {
        switch (waveId) {
        case 7:  table = ui->telemetryTable2_1; break;
        case 8:  table = ui->telemetryTable2_2; break;
        case 9:  table = ui->telemetryTable2_3; break;
        case 10: table = ui->telemetryTable2_4; break;
        case 11: table = ui->telemetryTable2_5; break;
        case 12: table = ui->telemetryTable2_6; break;
        }
    }

    if (!table) return;

    int count = qMin(values.size(), 15);
    for (int i = 0; i < count; i++) {
        QTableWidgetItem *item = table->item(i, 1);
        if (item) {
            item->setText(values.at(i));
        }
    }
}

void AntennaDeviceWindow::parseWaveControlTelemetry(const QByteArray &payload)
{
    if (payload.size() < 288) return;

    const char *data = payload.constData();

    for (int waveId = 1; waveId <= 12; waveId++) {
        QVector<QString> values(15);

        int offsetPlus5V1 = (waveId - 1) * 2;
        qint16 plus5V1 = (static_cast<quint8>(data[offsetPlus5V1 + 1]) << 8)
                         | static_cast<quint8>(data[offsetPlus5V1]);
        values[0] = QString::number(plus5V1 / 65536.0, 'f', 2);

        int baseOffset = 96 + (waveId - 1) * 16;
        values[1] = QString::number(static_cast<quint8>(data[baseOffset + 0]) / 256.0, 'f', 2);
        values[2] = QString::number(static_cast<quint8>(data[baseOffset + 1]) / 256.0, 'f', 2);
        values[3] = QString::number(static_cast<quint8>(data[baseOffset + 2]) / 256.0, 'f', 2);
        values[4] = QString::number(static_cast<quint8>(data[baseOffset + 3]) / 256.0, 'f', 2);
        values[5] = QString::number(static_cast<quint8>(data[baseOffset + 4]) / 256.0, 'f', 2);

        for (int i = 0; i < 8; i++) {
            values[6 + i] = QString::number(static_cast<quint8>(data[baseOffset + 5 + i]) / 256.0, 'f', 2);
        }

        qint32 digitalRaw = (static_cast<quint8>(data[baseOffset + 15]) << 16)
                          | (static_cast<quint8>(data[baseOffset + 14]) << 8)
                          | static_cast<quint8>(data[baseOffset + 13]);
        values[14] = QString("0x%1").arg(digitalRaw, 6, 16, QChar('0'));

        updateWaveControlTelemetry(waveId, values);
    }
}

// ============================================================================
//                          热控遥测数据解析
// ============================================================================



void AntennaDeviceWindow::on_startPRFButton_clicked() {

    QByteArray command = QByteArray::fromRawData("\xEB\x90\x01\xF1\x05", 5);
    emit sendRawCommandToServer(command);

}
void AntennaDeviceWindow::on_stopPRFButton_clicked() {
    QByteArray command = QByteArray::fromRawData("\xEB\x90\x01\xF0\x05", 5);
    emit sendRawCommandToServer(command);
}

void AntennaDeviceWindow::on_waveControlSendButton_clicked() {}

void AntennaDeviceWindow::on_prfDownloadButton_clicked()
{
    bool ok;

    quint32 prfValue = ui->prfValueLineEdit->text().toUInt(&ok);
    if (!ok) {
        QMessageBox::warning(this, "错误", "PRF 值无效。");
        return;
    }

    quint32 totalCount = ui->pulseWidthLineEdit_2->text().toUInt(&ok);
    if (!ok) {
        QMessageBox::warning(this, "错误", "PRF 总数无效。");
        return;
    }

    quint16 pulseWidth = ui->pulseWidthLineEdit->text().toUShort(&ok);
    if (!ok) {
        QMessageBox::warning(this, "错误", "脉宽无效。");
        return;
    }

    quint8 protection = static_cast<quint8>(ui->protectionComboBox->currentIndex());
    quint8 protectionValue = (protection == 0) ? 0x01 : 0x10;

    QByteArray command;

    command.append(0xEB);
    command.append(0x90);
    command.append(0x05);
    command.append(0xF0);

    command.append(static_cast<char>((prfValue >> 24) & 0xFF));
    command.append(static_cast<char>((prfValue >> 16) & 0xFF));
    command.append(static_cast<char>((prfValue >> 8) & 0xFF));
    command.append(static_cast<char>(prfValue & 0xFF));

    command.append(static_cast<char>((totalCount >> 24) & 0xFF));
    command.append(static_cast<char>((totalCount >> 16) & 0xFF));
    command.append(static_cast<char>((totalCount >> 8) & 0xFF));
    command.append(static_cast<char>(totalCount & 0xFF));

    command.append(static_cast<char>((pulseWidth >> 8) & 0xFF));
    command.append(static_cast<char>(pulseWidth & 0xFF));

    command.append(protectionValue);

    command.append(0x0F);

    emit sendRawCommandToServer(command);

    QMessageBox::information(this, "完成", "PRF 参数已下发。");
}

void AntennaDeviceWindow::on_testParamDownloadButton_clicked()
{
    bool ok;

    // 获取波位数量（8 位）
    quint8 waveCount = ui->wavePoslineEdit->text().toUInt(&ok);
    if (!ok || waveCount == 0) {
        QMessageBox::warning(this, "错误", "波位数量无效。");
        return;
    }

    // 获取极化模式（ComboBox 索引 0-8 对应值 0x01-0x09）
    quint8 polarization = static_cast<quint8>(ui->polarizationComboBox->currentIndex() + 1);

    // 获取收发测试（ComboBox 索引 0=发射测试 0x01, 1=接收测试 0x10）
    quint8 testMode = (ui->testModeComboBox->currentIndex() == 0) ? 0x01 : 0x10;

    // 获取测试模式（ComboBox 索引对应值）
    quint8 testModeValue;
    switch (ui->testModeComboBox_2->currentIndex()) {
    case 0: testModeValue = 0x01; break; // 单板模式
    case 1: testModeValue = 0x10; break; // 暗室多波位 PRX 模式
    case 2: testModeValue = 0x11; break; // 暗室单波位 PRX 模式
    case 3: testModeValue = 0x20; break; // 暗室多波位 PTX 模式
    case 4: testModeValue = 0x21; break; // 暗室单波位 PTX 模式
    case 5: testModeValue = 0x02; break; // 轻小型 DTR 定标模式
    case 6: testModeValue = 0x03; break; // 轻小型普通模式
    default: testModeValue = 0x01; break;
    }

    // 获取波控时钟设置（ComboBox 索引 0-3 对应值 0x00-0x03）
    quint8 clockSetting = static_cast<quint8>(ui->clockSettingComboBox->currentIndex());

    // 获取布控模式（ComboBox 索引 0=二维波控 0x00, 1=全计算模式 0x01）
    quint8 bukongMode = static_cast<quint8>(ui->bukongSettingComboBox->currentIndex());

    // 构建测试参数下发指令帧
    // 帧格式：帧头 (2 字节) + 帧类型 (1 字节) + 指令类型 (1 字节) +
    //        波位数量 (1 字节) + 极化模式 (1 字节) + 收发测试 (1 字节) +
    //        测试模式 (1 字节) + 波控时钟设置 (1 字节) + 布控模式 (1 字节) + 帧长 (1 字节)
    QByteArray command;

    // 1. 帧头：0xEB90
    command.append(0xEB);
    command.append(0x90);

    // 2. 帧类型：0x05
    command.append(0x05);

    // 3. 指令类型：0xE0 (测试参数)
    command.append(0xE0);

    // 4. 波位数量（8 位）
    command.append(static_cast<char>(waveCount & 0xFF));

    // 5. 极化模式（8 位）：0x01-0x09
    command.append(static_cast<char>(polarization));

    // 6. 收发测试（8 位）：0x01=发射测试，0x10=接收测试
    command.append(static_cast<char>(testMode));

    // 7. 测试模式（8 位）
    command.append(static_cast<char>(testModeValue));

    // 8. 波控时钟设置（8 位）：0x00=3.125MHz, 0x01=5MHz, 0x02=6.25MHz, 0x03=10MHz
    command.append(static_cast<char>(clockSetting));

    // 9. 布控模式（8 位）：0x00=二维波控，0x01=全计算模式
    command.append(static_cast<char>(bukongMode));

    // 10. 帧长：0x0A
    command.append(0x0A);

    // 发送指令
    emit sendRawCommandToServer(command);

    QMessageBox::information(this, "完成", "测试参数已下发。");
}

void AntennaDeviceWindow::antennaPowerOn()
{
    QByteArray command = QByteArray::fromRawData("\xEB\x90\x01\xA1\x05", 5);
    emit sendRawCommandToServer(command);
}

void AntennaDeviceWindow::antennaPowerOff()
{
    QByteArray command = QByteArray::fromRawData("\xEB\x90\x01\xA0\x05", 5);
    emit sendRawCommandToServer(command);
}

void AntennaDeviceWindow::startPrf()
{
    QByteArray command = QByteArray::fromRawData("\xEB\x90\x01\xF1\x05", 5);
    emit sendRawCommandToServer(command);
}

void AntennaDeviceWindow::stopPrf()
{
    QByteArray command = QByteArray::fromRawData("\xEB\x90\x01\xF0\x05", 5);
    emit sendRawCommandToServer(command);
}

/**
 * @brief 从单个文件下发波控码（自动测试流程调用）
 *
 * 读取单个 txt 文件，解析二进制波控码数据，
 * 分页发送（每页1024字节），页间间隔50ms。
 */
void AntennaDeviceWindow::downloadSingleWaveCode(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit sendRawCommandToServer(QByteArray()); // 空信号表示错误
        return;
    }

    QString strContent = QString::fromUtf8(file.readAll());
    file.close();

    // 去除空格
    strContent = strContent.replace(" ", "");

    // 补齐到8的倍数
    int remainder = strContent.length() % 8;
    if (remainder != 0) {
        strContent = strContent.leftJustified(strContent.length() + (8 - remainder), '0');
    }

    // 二进制字符串 → 字节数组
    QByteArray data;
    for (int i = 0; i < strContent.length() / 8; i++) {
        QString tempBinary = strContent.mid(i * 8, 8);
        bool ok;
        unsigned short value = tempBinary.toUShort(&ok, 2);
        if (ok) {
            data.append(static_cast<char>(value));
        }
    }

    if (data.isEmpty()) {
        return;
    }

    // 分页发送（与 uploadPagedData 相同逻辑）
    int totalPages = qCeil(static_cast<double>(data.size()) / 1024.0);

    // 先发总页数指令
    sendBokongCode(totalPages, 0xB0);

    QThread::msleep(50);

    int page = 1;
    int offset = 0;
    while (offset < data.size()) {
        int remaining = data.size() - offset;
        int copySize = qMin(remaining, 1024);
        QByteArray dataPage = data.mid(offset, copySize);

        bokongMaSend(dataPage, page);
        page++;
        offset += copySize;

        QThread::msleep(50);  // 页间间隔 50ms
    }
}

void AntennaDeviceWindow::on_antennaPowerOnButton_clicked()
{
    antennaPowerOn();
}

void AntennaDeviceWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}

void AntennaDeviceWindow::on_antennaPowerOffButton_clicked()
{
    antennaPowerOff();
}

void AntennaDeviceWindow::on_thermalPowerOnButton_clicked()
{
    QByteArray command = QByteArray::fromRawData("\xEB\x90\x01\x31\x05", 5);
    emit sendRawCommandToServer(command);
}

void AntennaDeviceWindow::on_thermalPowerOffButton_clicked()
{
    QByteArray command = QByteArray::fromRawData("\xEB\x90\x01\x30\x05", 5);
    emit sendRawCommandToServer(command);
}

void AntennaDeviceWindow::on_startButton_clicked()
{
    QByteArray command = QByteArray::fromRawData("\xEB\x90\x01\xF1\x05", 5);
    emit sendRawCommandToServer(command);
}

void AntennaDeviceWindow::on_stopButton_clicked()
{
    QByteArray command = QByteArray::fromRawData("\xEB\x90\x01\xF0\x05", 5);
    emit sendRawCommandToServer(command);
}

void AntennaDeviceWindow::on_wave2DFileButton_clicked()
{
    QString dirPath = QFileDialog::getExistingDirectory(
        this,
        "选择二维波控文件夹",
        m_wave2DFilePath.isEmpty() ? "." : m_wave2DFilePath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dirPath.isEmpty()) {
        return;
    }

    m_wave2DFilePath = dirPath;
    ui->wave2DFilePathEdit->setText(dirPath);
}

bool AntennaDeviceWindow::readDirectoryBinaryFiles(const QString &dirPath, QByteArray &outData)
{
    QDir dir(dirPath);
    if (!dir.exists()) return false;

    QStringList filters;
    filters << "*.*";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name);

    for (const QFileInfo &fileInfo : fileList) {
        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;

        QByteArray fileContent = file.readAll();
        file.close();

        QString strContent = QString::fromUtf8(fileContent);
        strContent = strContent.replace(" ", "");

        int remainder = strContent.length() % 8;
        if (remainder != 0) {
            strContent = strContent.leftJustified(strContent.length() + (8 - remainder), '0');
        }

        QByteArray currentFileBytes;
        for (int i = 0; i < strContent.length() / 8; i++) {
            QString tempBinary = strContent.mid(i * 8, 8);
            bool ok;
            unsigned short value = tempBinary.toUShort(&ok, 2);
            if (ok) {
                currentFileBytes.append(static_cast<char>(value));
            }
        }
        outData.append(currentFileBytes);
    }
    return !outData.isEmpty();
}

void AntennaDeviceWindow::uploadPagedData(const QByteArray &data, char paramId,
                                          const QString &successMsg,
                                          void (AntennaDeviceWindow::*pageSender)(const QByteArray&, int))
{
    short page = 1;
    int totalPages = qCeil(static_cast<double>(data.size()) / 1024.0);

    sendBokongCode(totalPages, paramId);

    QThread::msleep(50);

    int offset = 0;
    while (offset < data.size()) {
        int remaining = data.size() - offset;
        int copySize = qMin(remaining, 1024);
        QByteArray dataPage = data.mid(offset, copySize);

        (this->*pageSender)(dataPage, page);
        page++;
        offset += copySize;
        QThread::msleep(50);
    }

    QMessageBox::information(this, "完成", successMsg);
}

void AntennaDeviceWindow::on_wave2DUploadButton_clicked()
{
    if (m_wave2DFilePath.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择二维波控文件夹。");
        return;
    }

    QByteArray combinedData;
    if (!readDirectoryBinaryFiles(m_wave2DFilePath, combinedData)) {
        QMessageBox::warning(this, "错误", "没有有效的波控数据。");
        return;
    }

    uploadPagedData(combinedData, 0xB0, "波控码已上注。",
                    &AntennaDeviceWindow::bokongMaSend);
}

void AntennaDeviceWindow::on_basicParamFileButton_clicked()
{
    QString dirPath = QFileDialog::getExistingDirectory(
        this,
        "选择全计算基本参数文件夹",
        m_basicParamFilePath.isEmpty() ? "." : m_basicParamFilePath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dirPath.isEmpty()) {
        return;
    }

    m_basicParamFilePath = dirPath;
    ui->basicParamFilePathEdit->setText(dirPath);
}

void AntennaDeviceWindow::on_basicParamUploadButton_clicked()
{
    if (m_basicParamFilePath.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择全计算基本参数文件夹。");
        return;
    }

    QByteArray combinedData;
    if (!readDirectoryBinaryFiles(m_basicParamFilePath, combinedData)) {
        QMessageBox::warning(this, "错误", "没有有效的基本参数数据。");
        return;
    }

    uploadPagedData(combinedData, 0xF3, "全计算基本参数已上注。",
                    &AntennaDeviceWindow::basicParamSend);
}

void AntennaDeviceWindow::on_layoutCtrlFileButton_clicked()
{
    QString dirPath = QFileDialog::getExistingDirectory(
        this,
        "选择全计算布控文件夹",
        m_layoutCtrlFilePath.isEmpty() ? "." : m_layoutCtrlFilePath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dirPath.isEmpty()) {
        return;
    }

    m_layoutCtrlFilePath = dirPath;
    ui->layoutCtrlFilePathEdit->setText(dirPath);
}

void AntennaDeviceWindow::on_layoutCtrlUploadButton_clicked()
{
    if (m_layoutCtrlFilePath.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择全计算布控文件夹。");
        return;
    }

    QByteArray combinedData;
    if (!readDirectoryBinaryFiles(m_layoutCtrlFilePath, combinedData)) {
        QMessageBox::warning(this, "错误", "没有有效的布控数据。");
        return;
    }

    uploadPagedData(combinedData, 0xD0, "全计算布控数据已上注。",
                    &AntennaDeviceWindow::layoutCtrlSend);
}

void AntennaDeviceWindow::bokongMaSend(const QByteArray &data, int page)
{
    QByteArray command;
    command.append(0xEB);
    command.append(0x90);
    command.append(0x03);
    command.append(0xB2);
    command.append(static_cast<char>(page & 0xFF));
    command.append(static_cast<char>((page >> 8) & 0xFF));
    command.append(static_cast<char>(data.size() & 0xFF));
    command.append(static_cast<char>((data.size() >> 8) & 0xFF));
    command.append(data);
    command.append(static_cast<char>(calcSumCheck(data)));

    emit sendRawCommandToServer(command);
}

char AntennaDeviceWindow::calcSumCheck(const QByteArray &data)
{
    char sum = 0;
    for (char byte : data) {
        sum += static_cast<unsigned char>(byte);
    }
    return sum;
}

void AntennaDeviceWindow::sendBokongCode(int length, char paramId)
{
    char b1 = static_cast<char>(length & 0xFF);
    char b2 = static_cast<char>((length >> 8) & 0xFF);

    QByteArray command;
    command.append(0xEB);
    command.append(0x90);
    command.append(0x01);
    command.append(paramId);
    command.append(b1);
    command.append(b2);

    emit sendRawCommandToServer(command);
}

void AntennaDeviceWindow::basicParamSend(const QByteArray &data, int page)
{
    QByteArray command;
    command.append(0xEB);
    command.append(0x90);
    command.append(0x03);
    command.append(0xF4);
    command.append(static_cast<char>(page & 0xFF));
    command.append(static_cast<char>((page >> 8) & 0xFF));
    command.append(static_cast<char>(data.size() & 0xFF));
    command.append(static_cast<char>((data.size() >> 8) & 0xFF));
    command.append(data);
    command.append(static_cast<char>(calcSumCheck(data)));

    emit sendRawCommandToServer(command);
}

void AntennaDeviceWindow::layoutCtrlSend(const QByteArray &data, int page)
{
    QByteArray command;
    command.append(0xEB);
    command.append(0x90);
    command.append(0x03);
    command.append(0xD2);
    command.append(static_cast<char>(page & 0xFF));
    command.append(static_cast<char>((page >> 8) & 0xFF));
    command.append(static_cast<char>(data.size() & 0xFF));
    command.append(static_cast<char>((data.size() >> 8) & 0xFF));
    command.append(data);
    command.append(static_cast<char>(calcSumCheck(data)));

    emit sendRawCommandToServer(command);
}
