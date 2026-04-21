#ifndef ANTENNADEVICEWINDOW_H
#define ANTENNADEVICEWINDOW_H

#include <QWidget>
#include <QTableWidgetItem>
#include <QVector>

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

signals:
    void antennaControlCommand(double targetAngle, double rotationSpeed, int controlMode, double accuracy);
    void sendRawCommandToServer(const QByteArray &command);  // 向服务端发送原始控制指令

private slots:
    void on_connectButton_clicked();
    void on_antennaPowerOnButton_clicked();
    void on_antennaPowerOffButton_clicked();
    void on_thermalPowerOnButton_clicked();
    void on_thermalPowerOffButton_clicked();
    void on_monitorPowerOnButton_clicked();
    void on_monitorPowerOffButton_clicked();
    void on_waveControlFileButton_clicked();
    void on_waveControlUploadButton_clicked();
    void on_waveControlReadButton_clicked();

    // 二维波控操作
    void on_wave2DFileButton_clicked();
    void on_wave2DUploadButton_clicked();
    void on_wave2DReadButton_clicked();

    // 全计算基本参数操作
    void on_basicParamFileButton_clicked();
    void on_basicParamUploadButton_clicked();
    void on_basicParamReadButton_clicked();

    // 全计算布控操作
    void on_layoutCtrlFileButton_clicked();
    void on_layoutCtrlUploadButton_clicked();
    void on_layoutCtrlReadButton_clicked();
    void on_prfDownloadButton_clicked();
    void on_startPRFButton_clicked();
    void on_stopPRFButton_clicked();
    void on_waveControlSendButton_clicked();
    void on_testParamDownloadButton_clicked();
    void initTelemetryTables();

    void on_startButton_clicked();

    void on_stopButton_clicked();

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
    Ui::antennadevicewindow *ui;
    QString m_wave2DFilePath;
    QString m_basicParamFilePath;
    QString m_layoutCtrlFilePath;
};

#endif // ANTENNADEVICEWINDOW_H
