#ifndef AUTOTESTWINDOW_H
#define AUTOTESTWINDOW_H

#include <QWidget>

namespace Ui {
class AutoTestWindow;
}

class AutoTestWindow : public QWidget
{
    Q_OBJECT

public:
    explicit AutoTestWindow(QWidget *parent = nullptr);
    ~AutoTestWindow();

signals:
    void antennaPowerOnRequested();      // 请求天线加电
    void antennaPowerOffRequested();     // 请求天线断电
    void connectToHostRequested(const QString &ip, quint16 port);   // 请求连接矢网仪器
    void disconnectFromHostRequested();  // 请求断开矢网仪器

public slots:
    void onConnected();     // 连接成功回调
    void onDisconnected();  // 断开连接回调

private slots:
    void on_connectButton_clicked();
    void on_powerOnButton_clicked();
    void on_powerOffButton_clicked();
    void on_selectWaveDirButton_clicked();
    void on_selectConfigFileButton_clicked();
    void on_startAutoTestButton_clicked();

private:
    void appendResult(const QString &text);
    void updateTestStatus(bool running);
    void loadConfigCsv(const QString &filePath);
    bool canStartTest() const;
    void updateStartButtonState();

    Ui::AutoTestWindow *ui;
    QString m_waveDirPath;
    QString m_configFilePath;
    bool m_connected = false;
    bool m_powerOn = false;
};

#endif // AUTOTESTWINDOW_H
