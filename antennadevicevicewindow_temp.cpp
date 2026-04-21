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
}

AntennaDeviceWindow::~AntennaDeviceWindow()
{
    delete ui;
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

void AntennaDeviceWindow::parseThermalTelemetry(const QByteArray &payload)
{
    if (payload.size() < 636) return;

    const char *data = payload.constData() + 512;

    quint8 header = static_cast<quint8>(data[0]);
    if (header != 0x55 || data[123] != 0xAA) return;

    QVector<QString> tempValues(112);
    for (int i = 0; i < 112; i++) {
        tempValues[i] = QString::number(static_cast<quint8>(data[3 + i]) / 256.0, 'f', 3);
    }

    QString heaterStatus;
    for (int byteIdx = 0; byteIdx < 7; byteIdx++) {
        quint8 statusByte = static_cast<quint8>(data[115 + byteIdx]);
        for (int bitIdx = 0; bitIdx < 8; bitIdx++) {
            int channelNum = byteIdx * 8 + bitIdx;
            if (channelNum >= 50) break;
            heaterStatus += QString("%1:%2 ")
                .arg(channelNum + 1)
                .arg((statusByte >> bitIdx) & 1 ? "开" : "关");
        }
    }

    Q_UNUSED(tempValues);
    Q_UNUSED(heaterStatus);
}

void AntennaDeviceWindow::on_connectButton_clicked() {}
void AntennaDeviceWindow::on_monitorPowerOnButton_clicked() {}
void AntennaDeviceWindow::on_monitorPowerOffButton_clicked() {}
void AntennaDeviceWindow::on_waveControlFileButton_clicked() {}
void AntennaDeviceWindow::on_waveControlUploadButton_clicked() {}
void AntennaDeviceWindow::on_waveControlReadButton_clicked() {}

void AntennaDeviceWindow::on_antennaPowerOnButton_clicked()
{
    QByteArray command = QByteArray::fromRawData("\xEB\x90\x01\xA1\x05", 5);
    emit sendRawCommandToServer(command);
}

void AntennaDeviceWindow::on_antennaPowerOffButton_clicked()
{
    QByteArray command = QByteArray::fromRawData("\xEB\x90\x01\xA0\x05", 5);
    emit sendRawCommandToServer(command);
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

void AntennaDeviceWindow::on_wave2DUploadButton_clicked()
{
    if (m_wave2DFilePath.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择二维波控文件夹。");
        return;
    }

    QDir dir(m_wave2DFilePath);
    if (!dir.exists()) {
        QMessageBox::warning(this, "错误", "文件夹不存在。");
        return;
    }

    QStringList filters;
    filters << "*.*";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name);

    if (fileList.isEmpty()) {
        QMessageBox::warning(this, "提示", "文件夹中没有文件。");
        return;
    }

    QByteArray combinedData;

    for (const QFileInfo &fileInfo : fileList) {
        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

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

        combinedData.append(currentFileBytes);
    }

    if (combinedData.isEmpty()) {
        QMessageBox::warning(this, "错误", "没有有效的波控数据。");
        return;
    }

    short page = 1;
    int totalPages = qCeil(static_cast<double>(combinedData.size()) / 1024.0);

    sendBokongCode(totalPages, 0xB0);

    QThread::msleep(50);

    int offset = 0;
    while (offset < combinedData.size()) {
        int remaining = combinedData.size() - offset;
        int copySize = qMin(remaining, 1024);
        QByteArray dataPage = combinedData.mid(offset, copySize);

        bokongMaSend(dataPage, page);
        page++;
        offset += copySize;
        QThread::msleep(50);
    }

    QMessageBox::information(this, "完成", "波控码已上注。");
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

    QDir dir(m_basicParamFilePath);
    if (!dir.exists()) {
        QMessageBox::warning(this, "错误", "文件夹不存在。");
        return;
    }

    QStringList filters;
    filters << "*.*";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name);

    if (fileList.isEmpty()) {
        QMessageBox::warning(this, "提示", "文件夹中没有文件。");
        return;
    }

    QByteArray combinedData;

    for (const QFileInfo &fileInfo : fileList) {
        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

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

        combinedData.append(currentFileBytes);
    }

    if (combinedData.isEmpty()) {
        QMessageBox::warning(this, "错误", "没有有效的基本参数数据。");
        return;
    }

    short page = 1;
    int totalPages = qCeil(static_cast<double>(combinedData.size()) / 1024.0);

    sendBokongCode(totalPages, 0xF3);

    QThread::msleep(50);

    int offset = 0;
    while (offset < combinedData.size()) {
        int remaining = combinedData.size() - offset;
        int copySize = qMin(remaining, 1024);
        QByteArray dataPage = combinedData.mid(offset, copySize);

        basicParamSend(dataPage, page);
        page++;
        offset += copySize;
        QThread::msleep(50);
    }

    QMessageBox::information(this, "完成", "全计算基本参数已上注。");
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

    QDir dir(m_layoutCtrlFilePath);
    if (!dir.exists()) {
        QMessageBox::warning(this, "错误", "文件夹不存在。");
        return;
    }

    QStringList filters;
    filters << "*.*";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name);

    if (fileList.isEmpty()) {
        QMessageBox::warning(this, "提示", "文件夹中没有文件。");
        return;
    }

    QByteArray combinedData;

    for (const QFileInfo &fileInfo : fileList) {
        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

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

        combinedData.append(currentFileBytes);
    }

    if (combinedData.isEmpty()) {
        QMessageBox::warning(this, "错误", "没有有效的布控数据。");
        return;
    }

    short page = 1;
    int totalPages = qCeil(static_cast<double>(combinedData.size()) / 1024.0);

    sendBokongCode(totalPages, 0xD0);

    QThread::msleep(50);

    int offset = 0;
    while (offset < combinedData.size()) {
        int remaining = combinedData.size() - offset;
        int copySize = qMin(remaining, 1024);
        QByteArray dataPage = combinedData.mid(offset, copySize);

        layoutCtrlSend(dataPage, page);
        page++;
        offset += copySize;
        QThread::msleep(50);
    }

    QMessageBox::information(this, "完成", "全计算布控数据已上注。");
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
