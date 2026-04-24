#include "autotestwindow.h"
#include "ui_autotestwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>

AutoTestWindow::AutoTestWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AutoTestWindow)
    , m_connected(false)
{
    ui->setupUi(this);
}

AutoTestWindow::~AutoTestWindow()
{
    delete ui;
}

void AutoTestWindow::on_connectButton_clicked()
{
    QString ip = ui->ipLineEdit->text().trimmed();
    QString port = ui->portLineEdit->text().trimmed();

    if (ip.isEmpty() || port.isEmpty()) {
        QMessageBox::warning(this, "提示", "请填写网络地址和端口号！");
        return;
    }

    // TODO: 实现实际的网络连接逻辑
    m_connected = true;
    ui->connectButton->setText("断开");
    appendResult(QString("[%1] 已连接到 %2:%3")
                 .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                 .arg(ip).arg(port));
}

void AutoTestWindow::on_selectWaveDirButton_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择波位文件夹", m_waveDirPath);
    if (!dir.isEmpty()) {
        m_waveDirPath = dir;
        appendResult(QString("[%1] 已选择波位文件夹：%2")
                     .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                     .arg(dir));
    }
}

void AutoTestWindow::on_startAutoTestButton_clicked()
{
    if (!m_connected) {
        QMessageBox::warning(this, "提示", "请先连接测试设备！");
        return;
    }

    if (m_waveDirPath.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先选择波位文件夹！");
        return;
    }

    appendResult(QString("[%1] 自动测试开始...")
                 .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));

    // TODO: 实现实际的自动测试逻辑

    appendResult(QString("[%1] 自动测试完成")
                 .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
}

void AutoTestWindow::appendResult(const QString &text)
{
    ui->resultPlainTextEdit->appendPlainText(text);
}
