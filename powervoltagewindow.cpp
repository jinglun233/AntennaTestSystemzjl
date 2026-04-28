#include "powervoltagewindow.h"
#include "ui_powervoltagewindow.h"

#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>

PowerVoltageWindow::PowerVoltageWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PowerVoltageWindow)
{
    ui->setupUi(this);

    // 添加地址栏和打开按钮（在浏览器容器布局内）
    QLineEdit *urlEdit = new QLineEdit("https://www.google.com");
    urlEdit->setPlaceholderText("输入网址，回车或点击按钮打开");
    QPushButton *openBtn = new QPushButton("打开浏览器");
    openBtn->setMinimumHeight(30);
    connect(openBtn, &QPushButton::clicked, this, &PowerVoltageWindow::on_openBrowserButton_clicked);
    connect(urlEdit, &QLineEdit::returnPressed, this, &PowerVoltageWindow::on_openBrowserButton_clicked);

    QHBoxLayout *toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(6);
    toolbarLayout->addWidget(urlEdit);
    toolbarLayout->addWidget(openBtn);
    ui->browserLayout->insertLayout(0, toolbarLayout);
}

PowerVoltageWindow::~PowerVoltageWindow()
{
    delete ui;
}

/**
 * @brief 点击按钮/回车 → 调用系统默认浏览器打开网页
 */
void PowerVoltageWindow::on_openBrowserButton_clicked()
{
    QLineEdit *urlEdit = findChild<QLineEdit*>();
    QString url = urlEdit ? urlEdit->text().trimmed() : "https://www.google.com";

    if (url.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入网址！");
        return;
    }

    if (!QUrl(url).isValid()) {
        QMessageBox::warning(this, "提示", "网址格式无效！");
        return;
    }

    bool ok = QDesktopServices::openUrl(QUrl(url));
    if (!ok) {
        QMessageBox::warning(this, "错误", "无法打开浏览器，请检查系统默认浏览器设置。");
    }
}