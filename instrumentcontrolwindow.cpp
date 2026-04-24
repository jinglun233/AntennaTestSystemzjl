#include "instrumentcontrolwindow.h"
#include "ui_instrumentcontrolwindow.h"
#include <QMessageBox>
#include <QDateTime>

InstrumentControlWindow::InstrumentControlWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::InstrumentControlWindow)
    , m_instrumentConnected(false)
{
    ui->setupUi(this);
}

InstrumentControlWindow::~InstrumentControlWindow()
{
    delete ui;
}

void InstrumentControlWindow::on_connectInstrumentButton_clicked()
{
    QString visaAddr = ui->visaAddressLineEdit->text().trimmed();
    if (visaAddr.isEmpty()) {
        QMessageBox::warning(this, "提示", "请填写 VISA 地址！");
        return;
    }

    // TODO: 实现实际的 VISA/SCPI 连接逻辑
    m_instrumentConnected = true;
    appendLog(QString("[%1] 仪器已连接：%2")
              .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
              .arg(visaAddr));
}

void InstrumentControlWindow::on_disconnectInstrumentButton_clicked()
{
    if (!m_instrumentConnected) {
        QMessageBox::information(this, "提示", "仪器未连接");
        return;
    }

    m_instrumentConnected = false;
    appendLog(QString("[%1] 仪器已断开")
              .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
}

void InstrumentControlWindow::on_initInstrumentButton_clicked()
{
    if (!m_instrumentConnected) {
        QMessageBox::warning(this, "提示", "请先连接仪器！");
        return;
    }

    appendLog(QString("[%1] 正在初始化仪器...")
              .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));

    // TODO: 实现实际的仪器初始化 SCPI 指令

    appendLog(QString("[%1] 仪器初始化完成")
              .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
}

void InstrumentControlWindow::on_readParamButton_clicked()
{
    if (!m_instrumentConnected) {
        QMessageBox::warning(this, "提示", "请先连接仪器！");
        return;
    }

    appendLog(QString("[%1] 正在读取仪器参数...")
              .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));

    // TODO: 实现实际的参数读取 SCPI 指令

    appendLog(QString("[%1] 参数读取完成")
              .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
}

void InstrumentControlWindow::on_setParamButton_clicked()
{
    if (!m_instrumentConnected) {
        QMessageBox::warning(this, "提示", "请先连接仪器！");
        return;
    }

    QString freq = ui->centerFreqLineEdit->text();
    QString span = ui->spanLineEdit->text();
    QString power = ui->powerLineEdit->text();

    appendLog(QString("[%1] 正在设置参数：中心频率=%2 GHz, 扫宽=%3 MHz, 参考功率=%4 dBm")
              .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
              .arg(freq).arg(span).arg(power));

    // TODO: 实现实际的参数设置 SCPI 指令

    appendLog(QString("[%1] 参数设置完成")
              .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
}

void InstrumentControlWindow::appendLog(const QString &text)
{
    ui->statusPlainTextEdit->appendPlainText(text);
}
