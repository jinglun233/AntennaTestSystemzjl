#include "temperatureinfowindow.h"
#include "ui_temperatureinfowindow.h"
#include "qcustomplot.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QMouseEvent>
#include <QPointF>
#include <QScrollBar>

class QCPItemTracer;
class QCPItemText;

TemperatureInfoWindow::TemperatureInfoWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::temperatureinfowindow),
    m_customPlot(nullptr),
    m_timeScrollBar(nullptr),
    m_tracer(nullptr),
    m_tooltipText(nullptr),
    m_scrollBarDragging(false),
    m_timeBase(0.0),
    m_curvePaused(false),
    m_replotThrottleTimer(nullptr),
    m_pendingReplot(false)
{
    ui->setupUi(this);

    m_temperatureData.resize(TEMP_CHANNEL_COUNT);
    m_temperatureData.fill(0.0);

    m_timeSeries.resize(TEMP_CHANNEL_COUNT);
    for (int i = 0; i < TEMP_CHANNEL_COUNT; ++i) {
        m_timeSeries[i].reserve(CURVE_MAX_POINTS);
    }
    m_timeStamps.reserve(CURVE_MAX_POINTS);

    m_elapsedTimer.start();

    initTemperatureTable();
    initSwitchTable();
    initMappingTable();
    initCurveContainer();
}

TemperatureInfoWindow::~TemperatureInfoWindow()
{
    delete ui;
}

// ============================================================================
//                           Table Initialization
// ============================================================================

void TemperatureInfoWindow::initTemperatureTable()
{
    QTableWidget *table = ui->temperatureTable;
    table->setRowCount(TEMP_CHANNEL_COUNT);
    table->setColumnCount(2);

    QStringList headers;
    headers << QString::fromUtf8("测温通道")
            << QString::fromUtf8("温度值 (℃)");
    table->setHorizontalHeaderLabels(headers);

    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->setColumnWidth(0, 100);

    int rowHeight = 22;
    for (int i = 0; i < TEMP_CHANNEL_COUNT; i++) {
        table->setRowHeight(i, rowHeight);

        auto *itemName = new QTableWidgetItem(
            QString::fromUtf8("通道%1").arg(i + 1, 3, 10, QChar('0')));
        itemName->setTextAlignment(Qt::AlignCenter);
        itemName->setFlags(itemName->flags() & ~Qt::ItemIsEditable);
        table->setItem(i, 0, itemName);

        auto *itemTemp = new QTableWidgetItem("--");
        itemTemp->setTextAlignment(Qt::AlignCenter);
        table->setItem(i, 1, itemTemp);
    }

    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
}

void TemperatureInfoWindow::initSwitchTable()
{
    QTableWidget *table = ui->switchTable;
    table->setRowCount(SWITCH_COUNT);
    table->setColumnCount(2);

    QStringList headers;
    headers << QString::fromUtf8("开关回路编号")
            << QString::fromUtf8("状态");
    table->setHorizontalHeaderLabels(headers);

    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->setColumnWidth(0, 100);

    int rowHeight = 22;
    for (int i = 0; i < SWITCH_COUNT; i++) {
        table->setRowHeight(i, rowHeight);

        auto *itemName = new QTableWidgetItem(
            QString::fromUtf8("开关%1").arg(i + 1, 2, 10, QChar('0')));
        itemName->setTextAlignment(Qt::AlignCenter);
        itemName->setFlags(itemName->flags() & ~Qt::ItemIsEditable);
        table->setItem(i, 0, itemName);

        auto *itemStatus = new QTableWidgetItem(
            QString::fromUtf8("关"));
        itemStatus->setTextAlignment(Qt::AlignCenter);
        table->setItem(i, 1, itemStatus);
    }

    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
}

void TemperatureInfoWindow::initMappingTable()
{
    QTableWidget *table = ui->mappingTable;
    table->setRowCount(SWITCH_COUNT);
    table->setColumnCount(5);

    QStringList headers;
    headers << QString::fromUtf8("开关回路编号")
            << QString::fromUtf8("测温通道编号")
            << QString::fromUtf8("低温阈值")
            << QString::fromUtf8("高温阈值")
            << QString::fromUtf8("判断");
    table->setHorizontalHeaderLabels(headers);

    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
    table->setColumnWidth(0, 100);
    table->setColumnWidth(2, 100);
    table->setColumnWidth(3, 100);
    table->setColumnWidth(4, 80);

    m_primaryChannels.resize(SWITCH_COUNT);
    m_backupChannels.resize(SWITCH_COUNT);

    for (int i = 0; i < SWITCH_COUNT; ++i) {
        table->setRowHeight(i, 24);

        int primaryCh = (i % TEMP_CHANNEL_COUNT) + 1;

        m_primaryChannels[i] = primaryCh;
        m_backupChannels[i] = 0;

        auto *itemName = new QTableWidgetItem(
            QString::fromUtf8("开关%1").arg(i + 1, 2, 10, QChar('0')));
        itemName->setTextAlignment(Qt::AlignCenter);
        itemName->setFlags(itemName->flags() & ~Qt::ItemIsEditable);
        table->setItem(i, 0, itemName);

        auto *itemChannel = new QTableWidgetItem(QString::number(primaryCh));
        itemChannel->setTextAlignment(Qt::AlignCenter);
        table->setItem(i, 1, itemChannel);

        auto *itemLow = new QTableWidgetItem("-40.0");
        itemLow->setTextAlignment(Qt::AlignCenter);
        table->setItem(i, 2, itemLow);

        auto *itemHigh = new QTableWidgetItem("60.0");
        itemHigh->setTextAlignment(Qt::AlignCenter);
        table->setItem(i, 3, itemHigh);

        auto *itemJudge = new QTableWidgetItem(
            QString::fromUtf8("平均值"));
        itemJudge->setTextAlignment(Qt::AlignCenter);
        table->setItem(i, 4, itemJudge);
    }

    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
}

// ============================================================================
//                       QCustomPlot Curve Init & Update
// ============================================================================

void TemperatureInfoWindow::initCurveContainer()
{
    QVBoxLayout *layout = new QVBoxLayout(ui->curveContainer);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);

    m_customPlot = new QCustomPlot(ui->curveContainer);
    layout->addWidget(m_customPlot);

    m_scrollBarDragging = false;
    m_timeScrollBar = new QScrollBar(Qt::Horizontal, ui->curveContainer);
    m_timeScrollBar->setEnabled(false);
    layout->addWidget(m_timeScrollBar);

    connect(m_timeScrollBar, &QScrollBar::sliderPressed,
            this, [this]() { m_scrollBarDragging = true; });
    connect(m_timeScrollBar, &QScrollBar::sliderReleased,
            this, [this]() { m_scrollBarDragging = false; });

    static const QColor colorPalette[] = {
        QColor("#e74c3c"), QColor("#e67e22"), QColor("#f1c40f"),
        QColor("#2ecc71"), QColor("#1abc9c"), QColor("#3498db"),
        QColor("#9b59b6"), QColor("#e91e63"), QColor("#00bcd4"),
        QColor("#8bc34a"), QColor("#ff5722"), QColor("#607d8b")
    };
    static const int paletteSize = sizeof(colorPalette) / sizeof(colorPalette[0]);

    for (int i = 0; i < TEMP_CHANNEL_COUNT; ++i) {
        m_customPlot->addGraph();
        QCPGraph *graph = m_customPlot->graph(i);
        graph->setPen(QPen(colorPalette[i % paletteSize], 1.5));
        graph->setName(QString("CH%1").arg(i + 1));
    }

    QSharedPointer<QCPAxisTickerDateTime> dateTimeTicker(new QCPAxisTickerDateTime);
    dateTimeTicker->setDateTimeFormat("MM-dd\nHH:mm:ss");
    dateTimeTicker->setTickCount(6);
    m_customPlot->xAxis->setTicker(dateTimeTicker);
    m_customPlot->xAxis->setTickLabelFont(QFont("Microsoft YaHei UI", 9));
    m_customPlot->yAxis->setTickLabelFont(QFont("Microsoft YaHei UI", 9));
    m_customPlot->xAxis->setLabelFont(QFont("Microsoft YaHei UI", 9));
    m_customPlot->yAxis->setLabelFont(QFont("Microsoft YaHei UI", 9));
    m_customPlot->xAxis->setLabel(QString::fromUtf8("时间"));
    m_customPlot->yAxis->setLabel(QString::fromUtf8("温度 (℃)"));

    m_customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);
    m_customPlot->axisRect()->setRangeDrag(Qt::Vertical);
    m_customPlot->axisRect()->setRangeZoom(Qt::Vertical);

    m_customPlot->legend->setVisible(false);
    m_customPlot->setBackground(QColor("#fafafa"));

    m_tracer = new QCPItemTracer(m_customPlot);
    m_tracer->setStyle(QCPItemTracer::tsCrosshair);
    m_tracer->setPen(QPen(QColor("#333"), 1, Qt::DashLine));
    m_tracer->setSize(10);

    m_tooltipText = new QCPItemText(m_customPlot);
    m_tooltipText->setColor(QColor("#333"));
    m_tooltipText->setFont(QFont("Microsoft YaHei UI", 9));
    m_tooltipText->setBrush(QBrush(QColor(255, 255, 220, 230)));
    m_tooltipText->setPadding(QMargins(4, 4, 4, 4));
    m_tooltipText->setPositionAlignment(Qt::AlignTop | Qt::AlignHCenter);
    m_tooltipText->setVisible(false);

    connect(m_customPlot, &QCustomPlot::mouseMove,
            this, &TemperatureInfoWindow::onCurveMouseMoved);

    connect(m_timeScrollBar, &QScrollBar::valueChanged,
            this, &TemperatureInfoWindow::onTimeScrollChanged);

    connect(ui->button_pauseCurve, &QPushButton::clicked, this, [this]() {
        m_curvePaused = !m_curvePaused;
        ui->button_pauseCurve->setText(m_curvePaused
            ? QString::fromUtf8("继续")
            : QString::fromUtf8("暂停"));
    });
    connect(ui->button_clearCurve, &QPushButton::clicked, this, [this]() {
        for (int ch = 0; ch < TEMP_CHANNEL_COUNT; ++ch) {
            m_timeSeries[ch].clear();
        }
        m_timeStamps.clear();
        m_customPlot->replot();
    });

    // 初始化重绘节流定时器（100ms，避免高频遥测导致主线程卡顿）
    m_replotThrottleTimer = new QTimer(this);
    m_replotThrottleTimer->setSingleShot(true);
    m_replotThrottleTimer->setInterval(100);
    connect(m_replotThrottleTimer, &QTimer::timeout, this, [this]() {
        if (m_pendingReplot) {
            m_pendingReplot = false;
            m_customPlot->replot();
        }
    });
}

void TemperatureInfoWindow::updateCurves()
{
    if (!m_customPlot || m_curvePaused) return;

    double nowEpoch = QDateTime::currentSecsSinceEpoch();

    m_timeStamps.append(nowEpoch);
    for (int ch = 0; ch < TEMP_CHANNEL_COUNT; ++ch) {
        m_timeSeries[ch].append(m_temperatureData[ch]);
    }

    while (m_timeStamps.size() > CURVE_MAX_POINTS) {
        m_timeStamps.removeFirst();
        for (int ch = 0; ch < TEMP_CHANNEL_COUNT; ++ch) {
            m_timeSeries[ch].removeFirst();
        }
    }

    for (int ch = 0; ch < TEMP_CHANNEL_COUNT; ++ch) {
        m_customPlot->graph(ch)->setData(m_timeStamps, m_timeSeries[ch]);
    }

    static const double WINDOW_SIZE = 60.0;

    if (!m_timeStamps.isEmpty()) {
        double dataStart = m_timeStamps.first();
        double dataEnd   = m_timeStamps.last();
        double dataSpan   = dataEnd - dataStart;

        if (dataSpan < WINDOW_SIZE) {
            m_customPlot->xAxis->setRange(dataStart, qMax(dataEnd, dataStart + 1.0));
            m_timeScrollBar->setEnabled(false);
        } else {
            m_timeScrollBar->setEnabled(true);

            double maxScroll = dataSpan - WINDOW_SIZE;
            int maxVal = qCeil(maxScroll * 10);
            m_timeScrollBar->setRange(0, maxVal);
            m_timeScrollBar->setPageStep(qCeil(WINDOW_SIZE * 10));
            m_timeScrollBar->setSingleStep(1);

            int currentVal = m_timeScrollBar->value();
            bool atEnd = (currentVal >= maxVal - 2);

            if (atEnd || !m_scrollBarDragging) {
                m_customPlot->xAxis->setRange(dataEnd - WINDOW_SIZE, dataEnd);
                m_timeScrollBar->blockSignals(true);
                m_timeScrollBar->setValue(maxVal);
                m_timeScrollBar->blockSignals(false);
            } else {
                double offset = currentVal / 10.0;
                double viewStart = dataStart + offset;
                double viewEnd = viewStart + WINDOW_SIZE;
                if (viewEnd > dataEnd) { viewEnd = dataEnd; viewStart = dataEnd - WINDOW_SIZE; }
                m_customPlot->xAxis->setRange(viewStart, viewEnd);
            }
        }
    }

    bool hasData = false;
    double globalMin = 999, globalMax = -999;
    for (int ch = 0; ch < TEMP_CHANNEL_COUNT; ++ch) {
        for (double v : m_timeSeries[ch]) {
            if (v < globalMin) globalMin = v;
            if (v > globalMax) globalMax = v;
            hasData = true;
        }
    }
    if (hasData && globalMax > globalMin) {
        double margin = (globalMax - globalMin) * 0.08;
        if (margin < 1.0) margin = 1.0;
        m_customPlot->yAxis->setRange(globalMin - margin, globalMax + margin);
    } else if (hasData && globalMax == globalMin) {
        double center = globalMax;
        m_customPlot->yAxis->setRange(center - 5, center + 5);
    }

    // 节流重绘：100ms 内多次更新只触发一次 replot，避免主线程卡顿
    if (!m_replotThrottleTimer->isActive()) {
        m_customPlot->replot();
        m_replotThrottleTimer->start();
    } else {
        m_pendingReplot = true;
    }
}

// ============================================================================
//                          Thermal Telemetry Parsing
// ============================================================================

void TemperatureInfoWindow::parseThermalTelemetry(const QByteArray &payload)
{
    if (payload.size() < 636) return;

    const char *data = payload.constData() + 512;

    quint8 header = static_cast<quint8>(data[0]);
    if (header != 0x55) return;

    for (int i = 0; i < 112; ++i) {
        double temp = static_cast<quint8>(data[3 + i]);
        updateTemperature(i + 1, temp);
    }

    for (int byteIdx = 0; byteIdx < 7; byteIdx++) {
        quint8 statusByte = static_cast<quint8>(data[115 + byteIdx]);
        for (int bitIdx = 0; bitIdx < 8; bitIdx++) {
            int switchNum = byteIdx * 8 + bitIdx;
            if (switchNum >= 50) break;
            bool isOn = (statusByte >> bitIdx) & 1;
            updateSwitchStatus(switchNum + 1, isOn);
        }
    }

    updateCurves();
}

void TemperatureInfoWindow::updateTemperature(int channel, double temperature)
{
    if (channel < 1 || channel > TEMP_CHANNEL_COUNT) return;

    m_temperatureData[channel - 1] = temperature;

    QTableWidget *table = ui->temperatureTable;
    int row = channel - 1;
    QTableWidgetItem *item = table->item(row, 1);
    if (item) {
        item->setText(QString::number(temperature, 'f', 2));
    }
}

void TemperatureInfoWindow::updateSwitchStatus(int switchId, bool onOff)
{
    if (switchId < 1 || switchId > SWITCH_COUNT) return;

    QTableWidget *table = ui->switchTable;
    int row = switchId - 1;
    QTableWidgetItem *item = table->item(row, 1);
    if (item) {
        item->setText(onOff
            ? QString::fromUtf8("开")
            : QString::fromUtf8("关"));
        if (onOff) {
            item->setBackground(QColor("#e74c3c"));
            item->setForeground(QColor("#ffffff"));
            item->setFont(QFont("Microsoft YaHei UI", 9, QFont::Bold));
        } else {
            item->setBackground(table->alternatingRowColors() && (row % 2 == 1)
                               ? QColor("#F2F2F2") : QColor("#ffffff"));
            item->setForeground(QColor("#333333"));
            item->setFont(QFont("Microsoft YaHei UI", 9, QFont::Normal));
        }
    }
}

// ============================================================================
//                          Thermal Control Slot Functions
// ============================================================================

void TemperatureInfoWindow::on_button_heatingManualSend_clicked()
{
    int circuitId = ui->spinBox_heatingCircuit->value();
    if (circuitId < 1 || circuitId > 50) {
        QMessageBox::warning(this,
            QString::fromUtf8("错误"),
            QString::fromUtf8("加热回路编号必须在 1~50 之间。"));
        return;
    }

    int action = ui->comboBox_heatingAction->currentIndex();
    if (action < 0 || action > 1) {
        QMessageBox::warning(this,
            QString::fromUtf8("错误"),
            QString::fromUtf8("请选择有效的操作类型。"));
        return;
    }

    QByteArray heatingControl(7, 0x00);
    int byteIndex = (circuitId - 1) / 8;
    int bitIndex = (circuitId - 1) % 8;
    if (action == 0) {
        heatingControl[byteIndex] = static_cast<char>(heatingControl[byteIndex] | (1 << bitIndex));
    }

    QByteArray rawPacket;
    rawPacket.append(static_cast<char>(0x55));
    rawPacket.append(static_cast<char>(0x40));
    rawPacket.append(static_cast<char>(0xAB));
    rawPacket.append(heatingControl);
    rawPacket.append(static_cast<char>(0x00));

    char checksum = 0;
    for (int i = 1; i < rawPacket.size(); i++) {
        checksum ^= rawPacket[i];
    }
    rawPacket.append(checksum);
    rawPacket.append(static_cast<char>(0xAA));

    QByteArray command;
    command.append(static_cast<char>(0xEB));
    command.append(static_cast<char>(0x90));
    command.append(static_cast<char>(0x05));
    command.append(static_cast<char>(0xAB));
    command.append(rawPacket);
    command.append(QByteArray(3, static_cast<char>(0x00)));
    command.append(static_cast<char>(0x16));

    emit sendRawCommandToServer(command);

    QString actionText = (action == 0)
        ? QString::fromUtf8("开启")
        : QString::fromUtf8("关闭");
    QMessageBox::information(this,
        QString::fromUtf8("完成"),
        QString::fromUtf8("已发送加热回路 %1 %2命令。").arg(circuitId).arg(actionText));
}

void TemperatureInfoWindow::on_button_thermalReset_clicked()
{
    QByteArray rawPacket;
    rawPacket.append(static_cast<char>(0x55));
    rawPacket.append(static_cast<char>(0x40));
    rawPacket.append(static_cast<char>(0x30));
    rawPacket.append(static_cast<char>(0x00));

    char checksum = 0;
    for (int i = 1; i < rawPacket.size(); i++) {
        checksum ^= rawPacket[i];
    }
    rawPacket.append(checksum);
    rawPacket.append(static_cast<char>(0xAA));

    QByteArray command;
    command.append(static_cast<char>(0xEB));
    command.append(static_cast<char>(0x90));
    command.append(static_cast<char>(0x05));
    command.append(static_cast<char>(0x30));
    command.append(rawPacket);
    command.append(QByteArray(3, static_cast<char>(0x00)));
    command.append(static_cast<char>(0x10));

    emit sendRawCommandToServer(command);

    QMessageBox::information(this,
        QString::fromUtf8("完成"),
        QString::fromUtf8("热控模块复位命令已发送。"));
}

// ============================================================================
//                          Mapping Configuration Slots
// ============================================================================

void TemperatureInfoWindow::on_button_loadMapping_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this,
        QString::fromUtf8("加载映射文件"), "",
        QString::fromUtf8("CSV 文件 (*.csv)"));
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this,
            QString::fromUtf8("错误"),
            QString::fromUtf8("无法打开文件"));
        return;
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");

    QTableWidget *table = ui->mappingTable;
    int rowIndex = 0;
    bool isHeader = true;

    while (!in.atEnd() && rowIndex < SWITCH_COUNT) {
        QString line = in.readLine();
        if (line.trimmed().isEmpty()) continue;

        if (isHeader) {
            isHeader = false;
            continue;
        }

        QStringList fields = line.split(',');
        if (fields.size() < 5) continue;

        QString switchName = fields[0].trimmed();
        QString channel = fields[1].trimmed();
        QString lowThreshold = fields[2].trimmed();
        QString highThreshold = fields[3].trimmed();
        QString judge = fields[4].trimmed();

        QTableWidgetItem *itemSwitch = table->item(rowIndex, 0);
        if (itemSwitch) itemSwitch->setText(switchName);

        QTableWidgetItem *itemChannel = table->item(rowIndex, 1);
        if (itemChannel) {
            itemChannel->setText(channel);
            m_primaryChannels[rowIndex] = channel.toInt();
        }

        QTableWidgetItem *itemLow = table->item(rowIndex, 2);
        if (itemLow) itemLow->setText(lowThreshold);

        QTableWidgetItem *itemHigh = table->item(rowIndex, 3);
        if (itemHigh) itemHigh->setText(highThreshold);

        QTableWidgetItem *itemJudge = table->item(rowIndex, 4);
        if (itemJudge) itemJudge->setText(judge);

        rowIndex++;
    }

    file.close();
    QMessageBox::information(this,
        QString::fromUtf8("成功"),
        QString::fromUtf8("映射文件加载成功，共加载 %1 行数据。").arg(rowIndex));
}

void TemperatureInfoWindow::on_button_saveMapping_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(this,
        QString::fromUtf8("保存映射文件"), "",
        QString::fromUtf8("CSV 文件 (*.csv)"));
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this,
            QString::fromUtf8("错误"),
            QString::fromUtf8("无法保存文件"));
        return;
    }

    file.write("\xEF\xBB\xBF");

    QTextStream out(&file);
    out.setCodec("UTF-8");

    out << QString::fromUtf8("开关回路编号,测温通道编号,低温阈值,高温阈值,判断\r\n");

    QTableWidget *table = ui->mappingTable;
    for (int i = 0; i < SWITCH_COUNT; ++i) {
        QString switchName = table->item(i, 0)->text();
        QString channel = table->item(i, 1)->text();
        QString lowThreshold = table->item(i, 2)->text();
        QString highThreshold = table->item(i, 3)->text();
        QString judge = table->item(i, 4)->text();

        out << switchName << "," << channel << ","
            << lowThreshold << "," << highThreshold << ","
            << judge << "\r\n";
    }

    file.close();
    QMessageBox::information(this,
        QString::fromUtf8("成功"),
        QString::fromUtf8("映射文件保存成功"));
}

void TemperatureInfoWindow::on_button_applyMapping_clicked()
{
    QTableWidget *table = ui->mappingTable;
    for (int i = 0; i < SWITCH_COUNT; ++i) {
        QTableWidgetItem *itemPrimary = table->item(i, 2);
        QTableWidgetItem *itemBackup = table->item(i, 3);

        if (itemPrimary) {
            m_primaryChannels[i] = itemPrimary->text().toInt();
        }
        if (itemBackup) {
            m_backupChannels[i] = itemBackup->text().toInt();
        }
    }

    emit mappingConfigChanged(m_primaryChannels, m_backupChannels);
    QMessageBox::information(this,
        QString::fromUtf8("成功"),
        QString::fromUtf8("映射配置已应用"));
}

void TemperatureInfoWindow::on_button_batchSet_clicked()
{
    int startSwitch = ui->spinBox_batchStart->value();
    int endSwitch = ui->spinBox_batchEnd->value();

    if (startSwitch < 1 || startSwitch > SWITCH_COUNT) {
        QMessageBox::warning(this,
            QString::fromUtf8("错误"),
            QString::fromUtf8("起始开关编号必须在 1~%1 之间。").arg(SWITCH_COUNT));
        return;
    }
    if (endSwitch < 1 || endSwitch > SWITCH_COUNT) {
        QMessageBox::warning(this,
            QString::fromUtf8("错误"),
            QString::fromUtf8("结束开关编号必须在 1~%1 之间。").arg(SWITCH_COUNT));
        return;
    }
    if (startSwitch > endSwitch) {
        QMessageBox::warning(this,
            QString::fromUtf8("错误"),
            QString::fromUtf8("起始开关编号不能大于结束开关编号。"));
        return;
    }

    int action = ui->comboBox_heatingAction_2->currentIndex();
    if (action < 0 || action > 1) {
        QMessageBox::warning(this,
            QString::fromUtf8("错误"),
            QString::fromUtf8("请选择有效的开关状态。"));
        return;
    }

    QByteArray heatingControl(7, 0x00);
    for (int i = startSwitch - 1; i < endSwitch; ++i) {
        int byteIndex = i / 8;
        int bitIndex = i % 8;
        if (action == 0) {
            heatingControl[byteIndex] = static_cast<char>(heatingControl[byteIndex] | (1 << bitIndex));
        }
    }

    QByteArray rawPacket;
    rawPacket.append(static_cast<char>(0x55));
    rawPacket.append(static_cast<char>(0x40));
    rawPacket.append(static_cast<char>(0xAB));
    rawPacket.append(heatingControl);
    rawPacket.append(static_cast<char>(0x00));

    char checksum = 0;
    for (int i = 1; i < rawPacket.size(); i++) {
        checksum ^= rawPacket[i];
    }
    rawPacket.append(checksum);
    rawPacket.append(static_cast<char>(0xAA));

    QByteArray command;
    command.append(static_cast<char>(0xEB));
    command.append(static_cast<char>(0x90));
    command.append(static_cast<char>(0x05));
    command.append(static_cast<char>(0xAB));
    command.append(rawPacket);
    command.append(QByteArray(3, static_cast<char>(0x00)));
    command.append(static_cast<char>(0x16));

    emit sendRawCommandToServer(command);

    QString actionText = (action == 0)
        ? QString::fromUtf8("开启")
        : QString::fromUtf8("关闭");
    QMessageBox::information(this,
        QString::fromUtf8("完成"),
        QString::fromUtf8("已批量%1 %2~%3，并发送控制命令。")
            .arg(actionText).arg(startSwitch).arg(endSwitch));
}

void TemperatureInfoWindow::onCurveMouseMoved(QMouseEvent *event)
{
    if (!m_customPlot) return;

    double x = m_customPlot->xAxis->pixelToCoord(event->pos().x());
    double y = m_customPlot->yAxis->pixelToCoord(event->pos().y());

    QCPGraph *nearestGraph = nullptr;
    double minDistSq = 1e18;

    for (int i = 0; i < TEMP_CHANNEL_COUNT; ++i) {
        QCPGraph *graph = m_customPlot->graph(i);
        if (!graph || graph->data()->isEmpty()) continue;

        const auto &data = graph->data();
        int closestIdx = -1;

        double minDSq = 1e18;
        for (int k = 0; k < data->size(); ++k) {
            double dx = data->at(k)->key - x;
            double dy = data->at(k)->value - y;
            double d2 = dx*dx + dy*dy;
            if (d2 < minDSq) {
                minDSq = d2;
                closestIdx = k;
            }
        }

        if (closestIdx >= 0 && minDSq < minDistSq) {
            minDistSq = minDSq;
            nearestGraph = graph;
            m_tracer->setGraph(graph);
            m_tracer->setGraphKey(data->at(closestIdx)->key);
        }
    }

    if (nearestGraph && minDistSq < 400) {
        m_tracer->setVisible(true);

        double yValue = m_tracer->position->value();
        double xKey = m_tracer->position->key();
        QString curveName = nearestGraph->name();

        QString timeStr = QDateTime::fromSecsSinceEpoch(qint64(xKey))
                              .toString("MM-dd HH:mm:ss");
        m_tooltipText->setText(
            QString::fromUtf8("%1\n时间: %2\n温度: %3℃")
                .arg(curveName).arg(timeStr).arg(yValue, 0, 'f', 2));
        m_tooltipText->position->setCoords(xKey, yValue);
        m_tooltipText->setVisible(true);
    } else {
        m_tracer->setVisible(false);
        m_tooltipText->setVisible(false);
    }

    m_customPlot->replot();
}

void TemperatureInfoWindow::onTimeScrollChanged(int value)
{
    if (!m_customPlot || m_timeStamps.isEmpty()) return;

    double dataStart = m_timeStamps.first();
    double dataEnd   = m_timeStamps.last();
    static const double WINDOW_SIZE = 60.0;

    double offset = value / 10.0;
    double viewStart = dataStart + offset;
    double viewEnd = viewStart + WINDOW_SIZE;

    if (viewEnd > dataEnd) {
        viewEnd = dataEnd;
        viewStart = qMax(dataEnd - WINDOW_SIZE, dataStart);
    }
    if (viewStart < dataStart) {
        viewStart = dataStart;
        viewEnd = dataStart + WINDOW_SIZE;
    }

    m_customPlot->xAxis->setRange(viewStart, viewEnd);
    m_customPlot->replot();
}
