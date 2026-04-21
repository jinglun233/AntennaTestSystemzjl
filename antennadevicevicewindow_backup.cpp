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

/**
 * @brief AntennaDeviceWindow鏋勯€犲嚱锟?
 *
 * 鍒濆鍖栧唴锟?
 * 1. 鍔犺浇UI鏂囦欢锛堝寘鍚帶鍒堕潰鏉裤€佹尝鎺ф搷浣溿€丳RF鍙傛暟銆佽皟璇曟寚浠ゃ€佹祴璇曞弬鏁般€侀仴娴嬭〃鏍硷級
 * 2. 鍒濆锟?2涓仴娴嬭〃鏍硷紙姣忚〃15锟?鍒楋細搴忓彿/鍚嶇О/鏁板€硷級
 */
AntennaDeviceWindow::AntennaDeviceWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::antennadevicewindow)
    , m_wave2DFilePath()
    , m_basicParamFilePath()
    , m_layoutCtrlFilePath()
{
    ui->setupUi(this);

    // 鍒濆鍖栨墍鏈夐仴娴嬭〃鏍硷紙12涓〃鏍硷紝姣忎釜15琛屾暟鎹級
    initTelemetryTables();
}

AntennaDeviceWindow::~AntennaDeviceWindow()
{
    delete ui;
}

// ==================== 閬ユ祴琛ㄦ牸鍒濆锟?====================

/**
 * @brief 鍒濆锟?2涓仴娴嬭〃鏍肩殑鏁版嵁鍚嶇О鍜屽唴锟?
 *
 * 琛ㄦ牸鍒嗗竷:
 * - 閬ユ祴1 Tab: 琛ㄦ牸1~6 锟?娉㈡帶1 ~ 娉㈡帶6
 * - 閬ユ祴2 Tab: 琛ㄦ牸7~12 锟?娉㈡帶7 ~ 娉㈡帶12
 *
 * 姣忎釜娉㈡帶锟?5椤规暟锟?
 * 0: +5V1   1: -5V1   2: +5V2   3: -5V2
 * 4: 28V1   5: 28V2
 * 6: TR缁勪欢1 7: TR缁勪欢2 ... 13: TR缁勪欢8
 * 14: 鏁板瓧閬ユ祴
 */
void AntennaDeviceWindow::initTelemetryTables()
{
    // ========== 閬ユ祴1锟?涓〃鏍硷紙娉㈡帶1~6锟?==========
    QStringList names1 = {
        "娉㈡帶1 +5V1", "娉㈡帶1 -5V1", "娉㈡帶1 +5V2", "娉㈡帶1 -5V2",
        "娉㈡帶1 28V1", "娉㈡帶1 28V2", "娉㈡帶1 TR缁勪欢1", "娉㈡帶1 TR缁勪欢2",
        "娉㈡帶1 TR缁勪欢3", "娉㈡帶1 TR缁勪欢4", "娉㈡帶1 TR缁勪欢5", "娉㈡帶1 TR缁勪欢6",
        "娉㈡帶1 TR缁勪欢7", "娉㈡帶1 TR缁勪欢8", "娉㈡帶1 鏁板瓧閬ユ祴"
    };
    QStringList names2 = { /* 娉㈡帶2 */ "娉㈡帶2 +5V1", "娉㈡帶2 -5V1", "娉㈡帶2 +5V2", "娉㈡帶2 -5V2", "娉㈡帶2 28V1", "娉㈡帶2 28V2", "娉㈡帶2 TR缁勪欢1", "娉㈡帶2 TR缁勪欢2", "娉㈡帶2 TR缁勪欢3", "娉㈡帶2 TR缁勪欢4", "娉㈡帶2 TR缁勪欢5", "娉㈡帶2 TR缁勪欢6", "娉㈡帶2 TR缁勪欢7", "娉㈡帶2 TR缁勪欢8", "娉㈡帶2 鏁板瓧閬ユ祴" };
    QStringList names3 = { /* 娉㈡帶3 */ "娉㈡帶3 +5V1", "娉㈡帶3 -5V1", "娉㈡帶3 +5V2", "娉㈡帶3 -5V2", "娉㈡帶3 28V1", "娉㈡帶3 28V2", "娉㈡帶3 TR缁勪欢1", "娉㈡帶3 TR缁勪欢2", "娉㈡帶3 TR缁勪欢3", "娉㈡帶3 TR缁勪欢4", "娉㈡帶3 TR缁勪欢5", "娉㈡帶3 TR缁勪欢6", "娉㈡帶3 TR缁勪欢7", "娉㈡帶3 TR缁勪欢8", "娉㈡帶3 鏁板瓧閬ユ祴" };
    QStringList names4 = { /* 娉㈡帶4 */ "娉㈡帶4 +5V1", "娉㈡帶4 -5V1", "娉㈡帶4 +5V2", "娉㈡帶4 -5V2", "娉㈡帶4 28V1", "娉㈡帶4 28V2", "娉㈡帶4 TR缁勪欢1", "娉㈡帶4 TR缁勪欢2", "娉㈡帶4 TR缁勪欢3", "娉㈡帶4 TR缁勪欢4", "娉㈡帶4 TR缁勪欢5", "娉㈡帶4 TR缁勪欢6", "娉㈡帶4 TR缁勪欢7", "娉㈡帶4 TR缁勪欢8", "娉㈡帶4 鏁板瓧閬ユ祴" };
    QStringList names5 = { /* 娉㈡帶5 */ "娉㈡帶5 +5V1", "娉㈡帶5 -5V1", "娉㈡帶5 +5V2", "娉㈡帶5 -5V2", "娉㈡帶5 28V1", "娉㈡帶5 28V2", "娉㈡帶5 TR缁勪欢1", "娉㈡帶5 TR缁勪欢2", "娉㈡帶5 TR缁勪欢3", "娉㈡帶5 TR缁勪欢4", "娉㈡帶5 TR缁勪欢5", "娉㈡帶5 TR缁勪欢6", "娉㈡帶5 TR缁勪欢7", "娉㈡帶5 TR缁勪欢8", "娉㈡帶5 鏁板瓧閬ユ祴" };
    QStringList names6 = { /* 娉㈡帶6 */ "娉㈡帶6 +5V1", "娉㈡帶6 -5V1", "娉㈡帶6 +5V2", "娉㈡帶6 -5V2", "娉㈡帶6 28V1", "娉㈡帶6 28V2", "娉㈡帶6 TR缁勪欢1", "娉㈡帶6 TR缁勪欢2", "娉㈡帶6 TR缁勪欢3", "娉㈡帶6 TR缁勪欢4", "娉㈡帶6 TR缁勪欢5", "娉㈡帶6 TR缁勪欢6", "娉㈡帶6 TR缁勪欢7", "娉㈡帶6 TR缁勪欢8", "娉㈡帶6 鏁板瓧閬ユ祴" };

    // ========== 閬ユ祴2锟?涓〃鏍硷紙娉㈡帶7~12锟?==========
    QStringList names7 = { /* 娉㈡帶7 */ "娉㈡帶7 +5V1", "娉㈡帶7 -5V1", "娉㈡帶7 +5V2", "娉㈡帶7 -5V2", "娉㈡帶7 28V1", "娉㈡帶7 28V2", "娉㈡帶7 TR缁勪欢1", "娉㈡帶7 TR缁勪欢2", "娉㈡帶7 TR缁勪欢3", "娉㈡帶7 TR缁勪欢4", "娉㈡帶7 TR缁勪欢5", "娉㈡帶7 TR缁勪欢6", "娉㈡帶7 TR缁勪欢7", "娉㈡帶7 TR缁勪欢8", "娉㈡帶7 鏁板瓧閬ユ祴" };
    QStringList names8 = { /* 娉㈡帶8 */ "娉㈡帶8 +5V1", "娉㈡帶8 -5V1", "娉㈡帶8 +5V2", "娉㈡帶8 -5V2", "娉㈡帶8 28V1", "娉㈡帶8 28V2", "娉㈡帶8 TR缁勪欢1", "娉㈡帶8 TR缁勪欢2", "娉㈡帶8 TR缁勪欢3", "娉㈡帶8 TR缁勪欢4", "娉㈡帶8 TR缁勪欢5", "娉㈡帶8 TR缁勪欢6", "娉㈡帶8 TR缁勪欢7", "娉㈡帶8 TR缁勪欢8", "娉㈡帶8 鏁板瓧閬ユ祴" };
    QStringList names9 = { /* 娉㈡帶9 */ "娉㈡帶9 +5V1", "娉㈡帶9 -5V1", "娉㈡帶9 +5V2", "娉㈡帶9 -5V2", "娉㈡帶9 28V1", "娉㈡帶9 28V2", "娉㈡帶9 TR缁勪欢1", "娉㈡帶9 TR缁勪欢2", "娉㈡帶9 TR缁勪欢3", "娉㈡帶9 TR缁勪欢4", "娉㈡帶9 TR缁勪欢5", "娉㈡帶9 TR缁勪欢6", "娉㈡帶9 TR缁勪欢7", "娉㈡帶9 TR缁勪欢8", "娉㈡帶9 鏁板瓧閬ユ祴" };
    QStringList names10 = { /* 娉㈡帶10 */ "娉㈡帶10 +5V1", "娉㈡帶10 -5V1", "娉㈡帶10 +5V2", "娉㈡帶10 -5V2", "娉㈡帶10 28V1", "娉㈡帶10 28V2", "娉㈡帶10 TR缁勪欢1", "娉㈡帶10 TR缁勪欢2", "娉㈡帶10 TR缁勪欢3", "娉㈡帶10 TR缁勪欢4", "娉㈡帶10 TR缁勪欢5", "娉㈡帶10 TR缁勪欢6", "娉㈡帶10 TR缁勪欢7", "娉㈡帶10 TR缁勪欢8", "娉㈡帶10 鏁板瓧閬ユ祴" };
    QStringList names11 = { /* 娉㈡帶11 */ "娉㈡帶11 +5V1", "娉㈡帶11 -5V1", "娉㈡帶11 +5V2", "娉㈡帶11 -5V2", "娉㈡帶11 28V1", "娉㈡帶11 28V2", "娉㈡帶11 TR缁勪欢1", "娉㈡帶11 TR缁勪欢2", "娉㈡帶11 TR缁勪欢3", "娉㈡帶11 TR缁勪欢4", "娉㈡帶11 TR缁勪欢5", "娉㈡帶11 TR缁勪欢6", "娉㈡帶11 TR缁勪欢7", "娉㈡帶11 TR缁勪欢8", "娉㈡帶11 鏁板瓧閬ユ祴" };
    QStringList names12 = { /* 娉㈡帶12 */ "娉㈡帶12 +5V1", "娉㈡帶12 -5V1", "娉㈡帶12 +5V2", "娉㈡帶12 -5V2", "娉㈡帶12 28V1", "娉㈡帶12 28V2", "娉㈡帶12 TR缁勪欢1", "娉㈡帶12 TR缁勪欢2", "娉㈡帶12 TR缁勪欢3", "娉㈡帶12 TR缁勪欢4", "娉㈡帶12 TR缁勪欢5", "娉㈡帶12 TR缁勪欢6", "娉㈡帶12 TR缁勪欢7", "娉㈡帶12 TR缁勪欢8", "娉㈡帶12 鏁板瓧閬ユ祴" };

    // 鍒濆鍖栭仴锟? Tab 锟?6 涓〃锟?
    setupTelemetryTable(ui->telemetryTable1_1, names1);
    setupTelemetryTable(ui->telemetryTable1_2, names2);
    setupTelemetryTable(ui->telemetryTable1_3, names3);
    setupTelemetryTable(ui->telemetryTable1_4, names4);
    setupTelemetryTable(ui->telemetryTable1_5, names5);
    setupTelemetryTable(ui->telemetryTable1_6, names6);


    // 鍒濆鍖栭仴锟? Tab 锟?6 涓〃锟?
    setupTelemetryTable(ui->telemetryTable2_1, names7);
    setupTelemetryTable(ui->telemetryTable2_2, names8);
    setupTelemetryTable(ui->telemetryTable2_3, names9);
    setupTelemetryTable(ui->telemetryTable2_4, names10);
    setupTelemetryTable(ui->telemetryTable2_5, names11);
    setupTelemetryTable(ui->telemetryTable2_6, names12);
}

/**
 * @brief 閰嶇疆鍗曚釜閬ユ祴琛ㄦ牸鐨勫瑙傚拰鏁版嵁
 *
 * 姣忎釜琛ㄦ牸閰嶇疆锟?
 * - 2锟? 搴忓彿(Fixed 35px) | 鍚嶇О(Fixed 100px) | 鏁帮拷?Fixed 55px) 锟?鎵€锟?2涓〃鏍煎畬鍏ㄤ竴锟?
 * - 15锟? 鍥哄畾琛岄珮18px锛岀揣鍑戞樉锟?
 * - 鍙妯″紡锛屼笉鍙紪锟?
 * - 鏃犳í鍚戞粴鍔ㄦ潯
 * - 鏂囧瓧涓嶆埅鏂渷鐣ワ紙ElideNone锟?
 */
void AntennaDeviceWindow::setupTelemetryTable(QTableWidget* table, const QStringList& names)
{
    // 鍩烘湰灞炴€ц锟?
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);
    table->setStyleSheet("QTableWidget { alternate-background-color: #F2F2F2; }");

    // 闅愯棌鍨傜洿琛ㄥご
//    table->verticalHeader()->setVisible(false);
//    table->verticalHeader()->setDefaultSectionSize(0);
//    table->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    // 璁剧疆琛屽垪锟?
    table->setRowCount(15);

    // 绂佺敤妯悜婊氬姩锟?
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    table->setTextElideMode(Qt::ElideNone);

    // ===== 鍏抽敭锟?2涓〃鏍肩粺涓€浣跨敤鐩稿悓鐨勫浐瀹氬垪锟?=====
    // 锟?锟?搴忓彿): Fixed 35px | 锟?锟?鍚嶇О): Fixed 100px | 锟?锟?鏁帮拷?: Fixed 55px
//    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
//    table->setColumnWidth(0, 35);
    table->setColumnWidth(0, 100);
    table->setColumnWidth(1, 67);

    // 鍥哄畾琛岄珮18px + 鍐欏叆15琛屾暟锟?
    int rowHeight = 30;
    for (int i = 0; i < 15; i++) {
        table->setRowHeight(i, rowHeight);

        // 锟?锟? 鍚嶇О锛堜粠names鍒楄〃鑾峰彇锟?
        QTableWidgetItem *nameItem = new QTableWidgetItem(names.at(i));
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        table->setItem(i, 0, nameItem);

        // 锟?锟? 鏁板€硷紙鍒濆涓虹┖锟?
        QTableWidgetItem *valueItem = new QTableWidgetItem("");
        valueItem->setFlags(valueItem->flags() & ~Qt::ItemIsEditable);
        valueItem->setTextAlignment(Qt::AlignCenter);
        valueItem->setForeground(QColor("#333333"));
        table->setItem(i, 1, valueItem);
    }
}

// ==================== 閬ユ祴鏁版嵁鏇存柊妲藉嚱锟?====================

/**
 * @brief 鏇存柊澶╃嚎閬ユ祴鏁版嵁锛堟棫鎺ュ彛锛屼繚鐣欏吋瀹癸級
 *
 * 褰撳墠姝ゅ嚱鏁颁笉鍋氬疄闄呭鐞嗭紝瀹為檯鏁版嵁閫氳繃 updateWaveControlTelemetry() 鏇存柊锟?
 */
void AntennaDeviceWindow::updateAntennaTelemetry(double angle, double speed, int status, double accuracy)
{
    Q_UNUSED(angle);
    Q_UNUSED(speed);
    Q_UNUSED(status);
    Q_UNUSED(accuracy);
}

/**
 * @brief 鏇存柊鎸囧畾娉㈡帶鐨勯仴娴嬫暟鎹埌瀵瑰簲琛ㄦ牸
 *
 * @param waveId 娉㈡帶缂栧彿锟?~12锟?
 * @param values 璇ユ尝鎺х殑15涓猀String鍊兼暟锟?
 *
 * 鏄犲皠鍏崇郴:
 * - waveId 1~6  锟?閬ユ祴1 Tab 锟?telemetryTable1_1 ~ telemetryTable1_6
 * - waveId 7~12 锟?閬ユ祴2 Tab 锟?telemetryTable2_1 ~ telemetryTable2_6
 */
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

/**
 * @brief 瑙ｆ瀽娉㈡帶鏁板瓧閬ユ祴鏁版嵁骞舵洿鏂板搴旇〃锟?
 *
 * 鏁版嵁鍩熸牸锟?1024瀛楄妭)锟?
 * 绗竴锟?(0-23瀛楄妭): 12涓尝鎺х殑+5V1鍊硷紝姣忔尝锟?瀛楄妭(浣庡瓧鑺傚湪鍓嶏紝楂樺瓧鑺傚湪锟?
 *   娉㈡帶1 +5V1: bytes 0-1
 *   娉㈡帶2 +5V1: bytes 2-3
 *   ...
 *   娉㈡帶12 +5V1: bytes 22-23
 *
 * 绗簩锟?(96-287瀛楄妭): 12涓尝鎺х殑鍏朵綑14椤规暟鎹紝姣忔尝锟?6瀛楄妭
 *   娉㈡帶1: bytes 96-111
 *   娉㈡帶2: bytes 112-127
 *   ...
 *   娉㈡帶12: bytes 272-287
 *
 * 姣忔尝锟?4椤规暟鎹帓鍒楋細
 *   [0]-5V1 [1]+5V2 [2]-5V2 [3]28V1 [4]28V2 [5]TR1 [6]TR2 [7]TR3
 *   [8]TR4 [9]TR5 [10]TR6 [11]TR7 [12]TR8 [13-15]鏁板瓧閬ユ祴(3瀛楄妭)
 *
 * @param payload 閬ユ祴鏁版嵁锟?1024瀛楄妭)
 */
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

/**
 * @brief 瑙ｆ瀽鐑帶閬ユ祴搴旂瓟鏁版嵁
 *
 * 鏁版嵁浣嶇疆: payload 锟?12-635瀛楄妭 (124瀛楄妭)
 * 鏍煎紡:
 *   瀛楄妭1 (offset 0):  鍖呭ご = 0x55
 *   瀛楄妭2 (offset 1):  璁惧璇嗗埆锟?= 0x40
 *   瀛楄妭3 (offset 2):  鎸囦护绫诲瀷 = 0xAA
 *   瀛楄妭4-115 (3-114): 娴嬫俯1-112閬ユ祴锟?(112瀛楄妭锛屾瘡Byte瀵瑰簲涓€璺祴娓╅€氶亾)
 *   瀛楄妭116-122 (115-121): 鍔犵儹鍥炶矾寮€鍏崇姸锟?(7瀛楄妭锟?0璺紑锟?
 *   瀛楄妭123 (offset 122): 甯ф牎锟?
 *   瀛楄妭124 (offset 123): 鍖呭熬 = 0xAA
 */
void AntennaDeviceWindow::parseThermalTelemetry(const QByteArray &payload)
{
    if (payload.size() < 636) return;

    const char *data = payload.constData() + 512;

    quint8 header = static_cast<quint8>(data[0]);
    quint8 deviceId = static_cast<quint8>(data[1]);
    quint8 cmdType = static_cast<quint8>(data[2]);

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
                .arg((statusByte >> bitIdx) & 1 ? "寮€" : "鍏?);
        }
    }

    Q_UNUSED(tempValues);
    Q_UNUSED(deviceId);
    Q_UNUSED(cmdType);
    Q_UNUSED(heaterStatus);
}

// ==================== 鎺у埗闈㈡澘鎸夐挳妲藉嚱锟?====================

void AntennaDeviceWindow::on_connectButton_clicked()
{
}

void AntennaDeviceWindow::on_antennaPowerOnButton_clicked()
{
    // 澶╃嚎寮€鏈烘寚锟? {0xEB, 0x90, 0x01, 0xA1, 0x05}
    QByteArray command = QByteArray::fromRawData("\xEB\x90\x01\xA1\x05", 5);
    emit sendRawCommandToServer(command);
}

void AntennaDeviceWindow::on_antennaPowerOffButton_clicked()
{
    // 澶╃嚎鍏虫満鎸囦护: {0xEB, 0x90, 0x01, 0xA0, 0x05}
    QByteArray command = QByteArray::fromRawData("\xEB\x90\x01\xA0\x05", 5);
    emit sendRawCommandToServer(command);
}

void AntennaDeviceWindow::on_thermalPowerOnButton_clicked()
{
    // 鐑帶鍔犵數鎸囦护: {0xEB, 0x90, 0x01, 0x31, 0x05}
    QByteArray command = QByteArray::fromRawData("\xEB\x90\x01\x31\x05", 5);
    emit sendRawCommandToServer(command);
}

void AntennaDeviceWindow::on_thermalPowerOffButton_clicked()
{
    // 鐑帶鏂數鎸囦护: {0xEB, 0x90, 0x01, 0x30, 0x05}
    QByteArray command = QByteArray::fromRawData("\xEB\x90\x01\x30\x05", 5);
    emit sendRawCommandToServer(command);
}

void AntennaDeviceWindow::on_monitorPowerOnButton_clicked()
{
}

void AntennaDeviceWindow::on_monitorPowerOffButton_clicked()
{
}

void AntennaDeviceWindow::on_startButton_clicked()
{
    // 娴嬭瘯寮€濮嬫寚锟? {0xEB, 0x90, 0x01, 0xF1, 0x05}
    QByteArray command = QByteArray::fromRawData("\xEB\x90\x01\xF1\x05", 5);
    emit sendRawCommandToServer(command);
}

void AntennaDeviceWindow::on_stopButton_clicked()
{
    // 娴嬭瘯缁撴潫鎸囦护: {0xEB, 0x90, 0x01, 0xF0, 0x05}
    QByteArray command = QByteArray::fromRawData("\xEB\x90\x01\xF0\x05", 5);
    emit sendRawCommandToServer(command);
}


// ==================== 娉㈡帶鎿嶄綔鎸夐挳妲藉嚱锟?====================

void AntennaDeviceWindow::on_waveControlFileButton_clicked()
{
}

void AntennaDeviceWindow::on_waveControlUploadButton_clicked()
{
}

void AntennaDeviceWindow::on_waveControlReadButton_clicked()
{
}

// ==================== 浜岀淮娉㈡帶鎿嶄綔妲藉嚱锟?====================

void AntennaDeviceWindow::on_wave2DFileButton_clicked()
{
    QString dirPath = QFileDialog::getExistingDirectory(
        this,
        "閫夋嫨浜岀淮娉㈡帶鏂囦欢澶?,
        m_wave2DFilePath.isEmpty() ? "." : m_wave2DFilePath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dirPath.isEmpty()) {
        return;
    }

    m_wave2DFilePath = dirPath;
    ui->wave2DFilePathEdit->setText(dirPath);
}

/**
 * @brief 浜岀淮娉㈡帶鐮佷笂娉ㄦ寜閽偣鍑诲锟?
 *
 * 鍔熻兘璇存槑锟?
 * 1. 浠庣敤鎴烽€夋嫨鐨勬枃浠跺す涓鍙栨墍鏈夋枃浠讹紙鎸夋枃浠跺悕鎺掑簭锟?
 * 2. 灏嗘瘡涓枃浠朵腑鐨勪簩杩涘埗瀛楃涓插唴瀹癸紙"10101010" 鏍煎紡锛夎浆鎹负瀛楄妭鏁版嵁
 * 3. 鍚堝苟鎵€鏈夋枃浠剁殑瀛楄妭鏁版嵁
 * 4. 鍙戦€佹尝鎺х爜涓婃敞鎸囦护锛堝憡璇夎澶囧嵆灏嗗彂閫佸灏戦〉鏁版嵁锟?
 * 5. 鍒嗛〉鍙戦€佹暟鎹紙姣忛〉1024瀛楄妭锛夛紝姣忛〉涔嬮棿闂撮殧50ms
 * 6. 姣忛〉鏁版嵁浣跨敤 bokongMaSend() 灏佽鎴愬抚鏍煎紡鍙戯拷?
 *
 * 甯ф牸寮忚鏄庯細
 * - sendBokongCode() 锟? 0xEB 0x90 0x01 0xB0 [闀垮害浣庡瓧鑺俔 [闀垮害楂樺瓧鑺俔
 * - bokongMaSend() 锟?  0xEB 0x90 0x03 0xB2 [椤靛彿浣庡瓧鑺俔 [椤靛彿楂樺瓧鑺俔 [鏁版嵁闀垮害浣嶿 [鏁版嵁闀垮害楂榏 [1024瀛楄妭鏁版嵁] [鏍￠獙鍜宂
 */
void AntennaDeviceWindow::on_wave2DUploadButton_clicked()
{
    // 1. 妫€鏌ユ枃浠跺す璺緞鏄惁鏈夋晥
    if (m_wave2DFilePath.isEmpty()) {
        QMessageBox::warning(this, "提示", "璇峰厛閫夋嫨浜岀淮娉㈡帶鏂囦欢澶广€?);
        return;
    }

    QDir dir(m_wave2DFilePath);
    if (!dir.exists()) {
        QMessageBox::warning(this, "错误", "文件夹不存在。"*.*";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name);

    if (fileList.isEmpty()) {
        QMessageBox::warning(this, "提示", "鏂囦欢澶逛腑娌℃湁鏂囦欢锟?);
        return;
    }

    // 3. 璇诲彇骞跺悎骞舵墍鏈夋枃浠剁殑浜岃繘鍒舵暟锟?
    QByteArray combinedData;

    for (const QFileInfo &fileInfo : fileList) {
        // 璇诲彇鏂囦欢鍐呭
        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        QByteArray fileContent = file.readAll();
        file.close();

        // 灏嗘枃浠跺唴瀹硅浆鎹负浜岃繘鍒跺瓧绗︿覆骞跺幓闄ょ┖锟?
        // 渚嬪: "10101010 11001100" -> "1010101011001100"
        QString strContent = QString::fromUtf8(fileContent);
        strContent = strContent.replace(" ", "");

        // 4. 灏嗕簩杩涘埗瀛楃涓茶浆鎹负瀛楄妭鏁扮粍
        // 锟?浣嶄簩杩涘埗鏁拌浆鎹负涓€涓瓧锟?(0-255)
        // 濡傛灉闀垮害涓嶆槸8鐨勫€嶆暟锛屽湪鏈熬锟?
        int remainder = strContent.length() % 8;
        if (remainder != 0) {
            strContent = strContent.leftJustified(strContent.length() + (8 - remainder), '0');
        }

        QByteArray currentFileBytes;
        for (int i = 0; i < strContent.length() / 8; i++) {
            // 姣忔锟?涓瓧绗︼紝杞崲涓轰簩杩涘埗锟?
            QString tempBinary = strContent.mid(i * 8, 8);
            bool ok;
            unsigned short value = tempBinary.toUShort(&ok, 2);
            if (ok) {
                currentFileBytes.append(static_cast<char>(value));
            }
        }

        // 鍚堝苟鍒版€绘暟鎹腑
        combinedData.append(currentFileBytes);
    }

    // 妫€鏌ユ槸鍚︽湁鏈夋晥鏁版嵁
    if (combinedData.isEmpty()) {
        QMessageBox::warning(this, "错误", "娌℃湁鏈夋晥鐨勬尝鎺ф暟鎹拷?);
        return;
    }

    // 5. 璁＄畻鎬婚〉鏁帮紙姣忛〉1024瀛楄妭锛屼笉瓒充竴椤典篃绠椾竴椤碉級
    short page = 1;  // 褰撳墠椤靛彿锛屼粠1寮€锟?
    int totalPages = qCeil(static_cast<double>(combinedData.size()) / 1024.0);

    // 6. 鍙戦€佹尝鎺х爜涓婃敞鎸囦护锛堝憡璇夎澶囨€婚〉鏁帮級
    sendBokongCode(totalPages, 0xB0);

    // 7. 绛夊緟50ms鍚庡紑濮嬪彂閫佹暟锟?
    QThread::msleep(50);

    // 8. 鍒嗛〉鍙戦€佹暟锟?
    int offset = 0;
    while (offset < combinedData.size()) {
        int remaining = combinedData.size() - offset;
        int copySize = qMin(remaining, 1024);  // 姣忛〉鏈€锟?024瀛楄妭
        QByteArray dataPage = combinedData.mid(offset, copySize);

        // 鍙戦€佷竴椤垫暟锟?
        bokongMaSend(dataPage, page);
        page++;
        offset += copySize;
        QThread::msleep(50);  // 姣忛〉涔嬮棿闂撮殧50ms
    }

    QMessageBox::information(this, "瀹屾垚", "娉㈡帶鐮佸凡涓婃敞銆?);
}

/**
 * @brief 鍙戦€佹尝鎺х爜鏁版嵁甯?
 *
 * 甯ф牸寮忥紙鍏?3瀛楄妭 + 鏁版嵁闀垮害锛夛細
 * 鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?
 * 鈹?0xEB   鈹?0x90   鈹?0x03   鈹?0xB2   鈹傞〉鍙蜂綆  鈹傞〉鍙烽珮  鈹傞暱搴︿綆    鈹?闀垮害楂?   鈹?
 * 鈹溾攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹尖攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹尖攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹尖攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹尖攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹尖攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹尖攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹尖攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?
 * 鈹?甯уご   鈹?甯уご   鈹?鍛戒护   鈹?鎸囦护ID 鈹?褰撳墠椤靛彿鈹?       鈹?鏁版嵁闀垮害  鈹?          鈹?
 * 鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹粹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹粹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹粹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹粹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹粹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹粹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹粹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?
 * 鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?
 * 鈹? 鏁版嵁     鈹?
 * 鈹溾攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?
 * 鈹?1024瀛楄妭  鈹?
 * 鈹溾攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?
 * 鈹? 鏍￠獙鍜?  鈹?
 * 鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?
 *
 * @param data  鏁版嵁鍐呭锛堟渶澶?024瀛楄妭锛?
 * @param page 椤靛彿锛堜粠1寮€濮嬶級
 */
void AntennaDeviceWindow::bokongMaSend(const QByteArray &data, int page)
{
    QByteArray frameHead(4, '\0');
    frameHead[0] = 0xEB;
    frameHead[1] = 0x90;
    frameHead[2] = 0x03;
    frameHead[3] = 0xB2;

    QByteArray num(2, '\0');
    num[0] = static_cast<char>(page & 0xFF);
    num[1] = static_cast<char>((page >> 8) & 0xFF);

    QByteArray len(2, '\0');
    len[0] = static_cast<char>(data.size() & 0xFF);
    len[1] = static_cast<char>((data.size() >> 8) & 0xFF);

    QByteArray command;
    command.append(frameHead);
    command.append(num[0]);
    command.append(num[1]);
    command.append(len[0]);
    command.append(len[1]);
    command.append(data);
    command.append(static_cast<char>(calcSumCheck(data)));

    emit sendRawCommandToServer(command);
}

/**
 * @brief 璁＄畻鏁版嵁鏍￠獙鍜?
 *
 * 鏍￠獙鍜?= 鎵€鏈夋暟鎹瓧鑺傜殑绱姞鍜岋紙鍙彇浣庡瓧鑺傦級
 * 鐢ㄤ簬鏁版嵁甯х殑鏈€鍚庝竴瀛楄妭锛岀敤浜庢帴鏀剁楠岃瘉鏁版嵁瀹屾暣鎬?
 *
 * @param data 鏁版嵁鍐呭
 * @return 鏍￠獙鍜岋紙浣庡瓧鑺傦級
 */
char AntennaDeviceWindow::calcSumCheck(const QByteArray &data)
{
    char sum = 0;
    for (char byte : data) {
        sum += static_cast<unsigned char>(byte);
    }
    return sum;
}

/**
 * @brief 鍙戦€佹尝鎺х爜/鍩烘湰鍙傛暟/甯冩帶鏁版嵁涓婃敞閫氱煡甯?
 *
 * 甯ф牸寮忥紙鍏?瀛楄妭锛夛細
 * 鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?
 * 鈹?0xEB   鈹?0x90   鈹?0x01   鈹?paramId鈹?闀垮害浣?鈹?闀垮害楂?鈹?
 * 鈹溾攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹尖攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹尖攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹尖攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹尖攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹尖攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?
 * 鈹?甯уご   鈹?甯уご   鈹?鍛戒护   鈹?鍙傛暟ID 鈹?鎬婚暱搴?鈹?       鈹?
 * 鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹粹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹粹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹粹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹粹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹粹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?
 *
 * 鍙傛暟ID瀵瑰簲鍏崇郴锛?
 * - 0xB0: 娉㈡帶鐮佷笂娉?
 * - 0xF3: 鍩烘湰鍙傛暟涓婃敞
 * - 0xD0: 甯冩帶鏁版嵁涓婃敞
 *
 * @param length 鎬绘暟鎹暱搴︼紙瀛楄妭鏁帮級
 * @param paramId 鍙傛暟ID锛堝尯鍒嗕笉鍚岀被鍨嬬殑鏁版嵁涓婃敞锛?
 */
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

/**
 * @brief 鍙戦€佸叏璁＄畻鍩烘湰鍙傛暟鏁版嵁甯?
 *
 * 甯ф牸寮忎笌 bokongMaSend 鐩稿悓锛屽尯鍒湪浜庢寚浠D涓?0xF4
 *
 * @param data 鏁版嵁鍐呭锛堟渶澶?024瀛楄妭锛?
 * @param page 椤靛彿锛堜粠1寮€濮嬶級
 */
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

/**
 * @brief 鍙戦€佸叏璁＄畻甯冩帶鏁版嵁甯?
 *
 * 甯ф牸寮忎笌 bokongMaSend 鐩稿悓锛屽尯鍒湪浜庢寚浠D涓?0xD2
 *
 * @param data 鏁版嵁鍐呭锛堟渶澶?024瀛楄妭锛?
 * @param page 椤靛彿锛堜粠1寮€濮嬶級
 */
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

void AntennaDeviceWindow::on_wave2DReadButton_clicked()
{
}

// ==================== 鍏ㄨ绠楀熀鏈弬鏁版搷浣滄Ы鍑芥暟 ====================

void AntennaDeviceWindow::on_basicParamFileButton_clicked()
{
    QString dirPath = QFileDialog::getExistingDirectory(
        this,
        "閫夋嫨鍏ㄨ绠楀熀鏈弬鏁版枃浠跺す",
        m_basicParamFilePath.isEmpty() ? "." : m_basicParamFilePath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dirPath.isEmpty()) {
        return;
    }

    m_basicParamFilePath = dirPath;
    ui->basicParamFilePathEdit->setText(dirPath);
}

/**
 * @brief 鍏ㄨ绠楀熀鏈弬鏁颁笂娉ㄦ寜閽偣鍑诲锟?
 *
 * 鍔熻兘璇存槑锟?
 * 涓庢尝鎺х爜涓婃敞娴佺▼绫讳技锛屽尯鍒湪浜庯細
 * 1. 浣跨敤 m_basicParamFilePath 鎸囧畾鐨勬枃浠跺す
 * 2. 鍙戦€佸抚鐨勬寚浠D涓嶅悓锟?xF3 鐢ㄤ簬閫氱煡锟?xF4 鐢ㄤ簬鏁版嵁锟?
 *
 * 甯ф牸寮忚鏄庯細
 * - sendBokongCode() 锟? 0xEB 0x90 0x01 0xF3 [闀垮害浣庡瓧鑺俔 [闀垮害楂樺瓧鑺俔
 * - basicParamSend() 锟?  0xEB 0x90 0x03 0xF4 [椤靛彿浣庡瓧鑺俔 [椤靛彿楂樺瓧鑺俔 [鏁版嵁闀垮害浣嶿 [鏁版嵁闀垮害楂榏 [鏁版嵁] [鏍￠獙鍜宂
 */
void AntennaDeviceWindow::on_basicParamUploadButton_clicked()
{
    // 1. 妫€鏌ユ枃浠跺す璺緞鏄惁鏈夋晥
    if (m_basicParamFilePath.isEmpty()) {
        QMessageBox::warning(this, "提示", "璇峰厛閫夋嫨鍏ㄨ绠楀熀鏈弬鏁版枃浠跺す锟?);
        return;
    }

    QDir dir(m_basicParamFilePath);
    if (!dir.exists()) {
        QMessageBox::warning(this, "错误", "文件夹不存在。"*.*";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name);

    if (fileList.isEmpty()) {
        QMessageBox::warning(this, "提示", "鏂囦欢澶逛腑娌℃湁鏂囦欢锟?);
        return;
    }

    // 3. 璇诲彇骞跺悎骞舵墍鏈夋枃浠剁殑浜岃繘鍒舵暟锟?
    QByteArray combinedData;

    for (const QFileInfo &fileInfo : fileList) {
        // 璇诲彇鏂囦欢鍐呭
        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        QByteArray fileContent = file.readAll();
        file.close();

        // 灏嗘枃浠跺唴瀹硅浆鎹负浜岃繘鍒跺瓧绗︿覆骞跺幓闄ょ┖锟?
        QString strContent = QString::fromUtf8(fileContent);
        strContent = strContent.replace(" ", "");

        // 灏嗕簩杩涘埗瀛楃涓茶浆鎹负瀛楄妭鏁扮粍
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

        // 鍚堝苟鍒版€绘暟鎹腑
        combinedData.append(currentFileBytes);
    }

    // 妫€鏌ユ槸鍚︽湁鏈夋晥鏁版嵁
    if (combinedData.isEmpty()) {
        QMessageBox::warning(this, "错误", "娌℃湁鏈夋晥鐨勫熀鏈弬鏁版暟鎹拷?);
        return;
    }

    // 4. 璁＄畻鎬婚〉鏁板苟鍙戦€佷笂娉ㄩ€氱煡
    short page = 1;
    int totalPages = qCeil(static_cast<double>(combinedData.size()) / 1024.0);
    sendBokongCode(totalPages, 0xF3);

    // 5. 绛夊緟50ms鍚庡紑濮嬪彂閫佹暟锟?
    QThread::msleep(50);

    // 6. 鍒嗛〉鍙戦€佹暟锟?
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

    QMessageBox::information(this, "瀹屾垚", "鍏ㄨ绠楀熀鏈弬鏁板凡涓婃敞锟?);
}

void AntennaDeviceWindow::on_basicParamReadButton_clicked()
{
}

// ==================== 鍏ㄨ绠楀竷鎺ф搷浣滄Ы鍑芥暟 ====================

void AntennaDeviceWindow::on_layoutCtrlFileButton_clicked()
{
    QString dirPath = QFileDialog::getExistingDirectory(
        this,
        "閫夋嫨鍏ㄨ绠楀竷鎺ф枃浠跺す",
        m_layoutCtrlFilePath.isEmpty() ? "." : m_layoutCtrlFilePath,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dirPath.isEmpty()) {
        return;
    }

    m_layoutCtrlFilePath = dirPath;
    ui->layoutCtrlFilePathEdit->setText(dirPath);
}

/**
 * @brief 鍏ㄨ绠楀竷鎺ф暟鎹笂娉ㄦ寜閽偣鍑诲锟?
 *
 * 鍔熻兘璇存槑锟?
 * 涓庢尝鎺х爜涓婃敞娴佺▼绫讳技锛屽尯鍒湪浜庯細
 * 1. 浣跨敤 m_layoutCtrlFilePath 鎸囧畾鐨勬枃浠跺す
 * 2. 鍙戦€佸抚鐨勬寚浠D涓嶅悓锟?xD0 鐢ㄤ簬閫氱煡锟?xD2 鐢ㄤ簬鏁版嵁锟?
 *
 * 甯ф牸寮忚鏄庯細
 * - sendBokongCode() 锟?  0xEB 0x90 0x01 0xD0 [闀垮害浣庡瓧鑺俔 [闀垮害楂樺瓧鑺俔
 * - layoutCtrlSend() 锟?  0xEB 0x90 0x03 0xD2 [椤靛彿浣庡瓧鑺俔 [椤靛彿楂樺瓧鑺俔 [鏁版嵁闀垮害浣嶿 [鏁版嵁闀垮害楂榏 [鏁版嵁] [鏍￠獙鍜宂
 */
void AntennaDeviceWindow::on_layoutCtrlUploadButton_clicked()
{
    // 1. 妫€鏌ユ枃浠跺す璺緞鏄惁鏈夋晥
    if (m_layoutCtrlFilePath.isEmpty()) {
        QMessageBox::warning(this, "提示", "璇峰厛閫夋嫨鍏ㄨ绠楀竷鎺ф枃浠跺す锟?);
        return;
    }

    QDir dir(m_layoutCtrlFilePath);
    if (!dir.exists()) {
        QMessageBox::warning(this, "错误", "文件夹不存在。"*.*";
    QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name);

    if (fileList.isEmpty()) {
        QMessageBox::warning(this, "提示", "鏂囦欢澶逛腑娌℃湁鏂囦欢锟?);
        return;
    }

    // 3. 璇诲彇骞跺悎骞舵墍鏈夋枃浠剁殑浜岃繘鍒舵暟锟?
    QByteArray combinedData;

    for (const QFileInfo &fileInfo : fileList) {
        // 璇诲彇鏂囦欢鍐呭
        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            continue;
        }

        QByteArray fileContent = file.readAll();
        file.close();

        // 灏嗘枃浠跺唴瀹硅浆鎹负浜岃繘鍒跺瓧绗︿覆骞跺幓闄ょ┖锟?
        QString strContent = QString::fromUtf8(fileContent);
        strContent = strContent.replace(" ", "");

        // 灏嗕簩杩涘埗瀛楃涓茶浆鎹负瀛楄妭鏁扮粍
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

        // 鍚堝苟鍒版€绘暟鎹腑
        combinedData.append(currentFileBytes);
    }

    // 妫€鏌ユ槸鍚︽湁鏈夋晥鏁版嵁
    if (combinedData.isEmpty()) {
        QMessageBox::warning(this, "错误", "娌℃湁鏈夋晥鐨勫竷鎺ф暟鎹拷?);
        return;
    }

    // 4. 璁＄畻鎬婚〉鏁板苟鍙戦€佷笂娉ㄩ€氱煡
    short page = 1;
    int totalPages = qCeil(static_cast<double>(combinedData.size()) / 1024.0);
    sendBokongCode(totalPages, 0xD0);

    // 5. 绛夊緟50ms鍚庡紑濮嬪彂閫佹暟锟?
    QThread::msleep(50);

    // 6. 鍒嗛〉鍙戦€佹暟锟?
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

    QMessageBox::information(this, "瀹屾垚", "鍏ㄨ绠楀竷鎺ф暟鎹凡涓婃敞銆?);
}

void AntennaDeviceWindow::on_layoutCtrlReadButton_clicked()
{
}

// ==================== PRF鍙傛暟鎸夐挳妲藉嚱锟?====================

void AntennaDeviceWindow::on_prfDownloadButton_clicked()
{
}

void AntennaDeviceWindow::on_startPRFButton_clicked()
{
}

void AntennaDeviceWindow::on_stopPRFButton_clicked()
{
}

// ==================== 璋冭瘯鎸囦护/娴嬭瘯鍙傛暟鎸夐挳妲藉嚱锟?====================

void AntennaDeviceWindow::on_waveControlSendButton_clicked()
{
}

void AntennaDeviceWindow::on_testParamDownloadButton_clicked()
{
}

void AntennaDeviceWindow::on_autoTestButton_clicked()
{
}

// ==================== 绐楀彛浜嬩欢閲嶅啓 ====================

void AntennaDeviceWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
}




