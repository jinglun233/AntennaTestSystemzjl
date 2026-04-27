#include "autotestwindow.h"
#include "ui_autotestwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QHeaderView>

AutoTestWindow::AutoTestWindow(QWidget *parent)
    : QWidget(parent, Qt::Window)
    , ui(new Ui::AutoTestWindow)
{
    ui->setupUi(this);
    ui->statusIconLabel->setPixmap(QPixmap(":/pic/redLED.png"));
    ui->statusTextLabel->setText("空闲");
    ui->startAutoTestButton->setEnabled(false);
}

AutoTestWindow::~AutoTestWindow()
{
    delete ui;
}

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
    ui->startAutoTestButton->setEnabled(canStartTest());
}

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

    updateTestStatus(true);
    appendResult(QString("[%1] 自动测试开始...")
                 .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));

    // TODO: 实现实际的自动测试逻辑

    updateTestStatus(false);
    appendResult(QString("[%1] 自动测试完成")
                 .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
}

void AutoTestWindow::appendResult(const QString &text)
{
    ui->resultPlainTextEdit->appendPlainText(text);
}
