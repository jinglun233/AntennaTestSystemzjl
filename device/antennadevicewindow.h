#ifndef ANTENNADEVICEWINDOW_H
#define ANTENNADEVICEWINDOW_H

#include <QWidget>
#include <QTableWidgetItem>
#include <QVector>
#include <QStringList>
#include <QTimer>
#include <QFile>
#include <QTextStream>

namespace Ui {
class antennadevicewindow;
}

class AntennaDeviceWindow : public QWidget
{
    Q_OBJECT

public:
    explicit AntennaDeviceWindow(QWidget *parent = nullptr);
    ~AntennaDeviceWindow();

public slots:
    void updateAntennaTelemetry(double angle, double speed, int status, double accuracy);
    void updateWaveControlTelemetry(int waveId, const QVector<QString> &values);
    void parseWaveControlTelemetry(const QByteArray &payload);

public:
    void antennaPowerOn();   // 天线加电（供外部调用）
    void antennaPowerOff();  // 天线断电（供外部调用）
    void startPrf();         // 开始PRF（供外部调用）
    void stopPrf();          // 停止PRF（供外部调用）

    /**
     * @brief 从单个文件路径下发波控码（自动测试流程使用，异步驱动）
     * @param filePath 波控码 txt 文件的完整路径
     *
     * 读取指定文件的二进制内容，通过 QTimer 分页异步发送到服务器，每页间隔50ms。
     * 全部发送完毕后发出 waveCodeDownloadCompleted 信号。
     */
    void downloadSingleWaveCode(const QString &filePath);

signals:
    void antennaControlCommand(double targetAngle, double rotationSpeed, int controlMode, double accuracy);
    void sendRawCommandToServer(const QByteArray &command);  // 向服务端发送原始控制指令

    /**
     * @brief 波控码异步下发完成信号
     * @param ok 是否成功
     * @param totalPages 总页数
     */
    void waveCodeDownloadCompleted(bool ok, int totalPages);

private slots:
    void on_antennaPowerOnButton_clicked();
    void on_antennaPowerOffButton_clicked();
    void on_thermalPowerOnButton_clicked();
    void on_thermalPowerOffButton_clicked();

    // 二维波控操作
    void on_wave2DFileButton_clicked();
    void on_wave2DUploadButton_clicked();

    // 全计算基本参数操作
    void on_basicParamFileButton_clicked();
    void on_basicParamUploadButton_clicked();

    // 全计算布控操作
    void on_layoutCtrlFileButton_clicked();
    void on_layoutCtrlUploadButton_clicked();
    void on_prfDownloadButton_clicked();
    void on_startPRFButton_clicked();
    void on_stopPRFButton_clicked();
    void on_waveControlSendButton_clicked();
    void on_testParamDownloadButton_clicked();
    void initTelemetryTables();

    void on_startButton_clicked();
    void on_stopButton_clicked();
    void onBukongSettingChanged(int index);

    // 异步分页定时器回调
    void onWavePageTimer();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupTelemetryTable(QTableWidget* table, const QStringList& names);
    void bokongMaSend(const QByteArray &data, int page);
    char calcSumCheck(const QByteArray &data);
    void sendBokongCode(int length, char paramId);
    void basicParamSend(const QByteArray &data, int page);
    void layoutCtrlSend(const QByteArray &data, int page);
    void updateDutyCycle();

    // 文件上注公共辅助函数
    bool readDirectoryBinaryFiles(const QString &dirPath, QByteArray &outData);
    void uploadPagedData(const QByteArray &data, char paramId,
                         const QString &successMsg,
                         void (AntennaDeviceWindow::*pageSender)(const QByteArray&, int));

    // ========== 遥测数据 CSV 保存 ==========
    /// 确保当日 CSV 文件已打开（日期变化时自动创建新文件，写入 BOM + 表头）
    void ensureTelemetryCsvFileOpen();
    /// 将一轮完整的 12 路波控遥测数据追加写入 CSV（一行）
    void saveTelemetryToCsv(const QVector<QVector<QString>> &allWaveValues);
    /// 获取所有列名（180 列：时间 + 波控1~12 各15项）
    static QStringList buildTelemetryColumnNames();

    Ui::antennadevicewindow *ui;
    QString m_wave2DFilePath;
    QString m_basicParamFilePath;
    QString m_layoutCtrlFilePath;

    // 异步波控码下发状态
    QTimer *m_wavePageTimer;           // 分页定时器（50ms/页）
    QByteArray m_pendingWaveData;      // 待发送的波控码数据
    int m_totalPages;                  // 总页数
    int m_currentPageIndex;            // 当前页索引（0-based）

    // ========== 遥测数据 CSV 保存成员 ==========
    QFile *m_telemetryCsvFile;         // 当前打开的 CSV 文件
    QTextStream *m_telemetryCsvStream;  // CSV 文本流（追加写入用）
    QString m_currentCsvDate;          // 当前文件对应的日期字符串 yyyyMMdd
};

#endif // ANTENNADEVICEWINDOW_H
