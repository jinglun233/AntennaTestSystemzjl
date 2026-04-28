#include "powervoltagewindow.h"
#include "ui_powervoltagewindow.h"

#include <QAxWidget>
#include <QMessageBox>
#include <QUrl>
#include <QDesktopServices>
#include <QSettings>
#include <QApplication>
#include <QFileInfo>

// IE WebBrowser 控件 CLSID
static const QString WEB_BROWSER_CLSID =
    QStringLiteral("{8856F961-340A-11D0-A96B-00C04FD705A2}");

PowerVoltageWindow::PowerVoltageWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PowerVoltageWindow),
    m_browser(nullptr),
    m_urlEdit(nullptr)
{
    ui->setupUi(this);

    // 强制设置 IE 仿真模式为 IE11
    setIEEmulationMode();

    initBrowser();
}

PowerVoltageWindow::~PowerVoltageWindow()
{
    delete ui;
}

/**
 * @brief 设置当前程序的 IE 浏览器仿真模式为 IE11
 */
void PowerVoltageWindow::setIEEmulationMode()
{
    QString appName = QApplication::applicationName();
    if (appName.isEmpty()) {
        appName = QFileInfo(QApplication::applicationFilePath()).fileName();
    }
    if (appName.endsWith(".exe", Qt::CaseInsensitive)) {
        appName.chop(4);
    }

    QSettings settings(
        QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\FEATURE_BROWSER_EMULATION"),
        QSettings::NativeFormat
        );

    int emulationValue = settings.value(appName, -1).toInt();
    if (emulationValue != 11001) {
        settings.setValue(appName, 11001);
        settings.sync();
    }
}

/**
 * @brief 初始化嵌入浏览器（使用 UI 中预置的控件）
 */
void PowerVoltageWindow::initBrowser()
{
    // 使用 UI 文件中已创建的地址栏和按钮
    m_urlEdit = ui->urlEdit;
    QPushButton *navigateBtn = ui->navigateBtn;

    // 连接信号槽
    connect(navigateBtn, &QPushButton::clicked,
            this, &PowerVoltageWindow::on_navigateButton_clicked);
    connect(m_urlEdit, &QLineEdit::returnPressed,
            this, &PowerVoltageWindow::on_navigateButton_clicked);

    // 创建嵌入式浏览器控件（IE ActiveX）
    m_browser = new QAxWidget(this);
    if (!m_browser->setControl(WEB_BROWSER_CLSID)) {
        QMessageBox::warning(this, "警告",
            "无法创建浏览器控件，请确保系统已安装 Internet Explorer。\n"
            "将使用系统浏览器作为备选方案。");
        delete m_browser;
        m_browser = nullptr;
    } else {
        // 静默脚本错误
        m_browser->setProperty("Silent", true);
        m_browser->dynamicCall("setSilent(bool)", true);

        // 将浏览器放入 UI 中的 browserPlaceholder 布局
        // 先隐藏占位标签
        ui->placeholderLabel->hide();
        ui->browserPlaceholderLayout->addWidget(m_browser);

        // 加载默认页面
        on_navigateButton_clicked();
    }
}

/**
 * @brief 导航按钮/回车 → 在嵌入式浏览器中打开网址
 */
void PowerVoltageWindow::on_navigateButton_clicked()
{
    if (!m_browser) {
        QString url = m_urlEdit ? m_urlEdit->text().trimmed() : "";
        if (!url.isEmpty()) {
            QDesktopServices::openUrl(QUrl(url));
        }
        return;
    }

    QString url = m_urlEdit ? m_urlEdit->text().trimmed() : "";
    if (url.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入网址！");
        return;
    }

    // 自动补全协议前缀
    if (!url.startsWith("http://", Qt::CaseInsensitive) &&
        !url.startsWith("https://", Qt::CaseInsensitive)) {
        url.prepend("https://");
        if (m_urlEdit) m_urlEdit->setText(url);
    }

    // 每次导航前重新设 Silent
    m_browser->setProperty("Silent", true);

    // Navigate 加载网页
    m_browser->dynamicCall("Navigate(const QString&)", url);
}
