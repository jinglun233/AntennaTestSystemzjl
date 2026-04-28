#include "powervoltagewindow.h"
#include "ui_powervoltagewindow.h"

#include <QAxWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
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

    // 强制设置 IE 仿真模式为 IE11（解决默认 IE7 模式不支持 ES6+ 语法的问题）
    setIEEmulationMode();

    initBrowser();
}

PowerVoltageWindow::~PowerVoltageWindow()
{
    delete ui;
}

/**
 * @brief 设置当前程序的 IE 浏览器仿真模式为 IE11
 *
 * 通过注册表 HKEY_CURRENT_USER\Software\Microsoft\Internet Explorer\Main\FeatureControl\FEATURE_BROWSER_EMULATION
 * 设置 exe 名对应的值为 11001（IE11 Edge 模式）。
 * 这样嵌入的 QAxWidget(IE ActiveX) 就会用 IE11 内核渲染，支持现代 JavaScript。
 */
void PowerVoltageWindow::setIEEmulationMode()
{
    // 获取程序文件名（不含路径）
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

    // 11001 = IE11 Edge 模式，支持 HTML5/ES6+
    int emulationValue = settings.value(appName, -1).toInt();
    if (emulationValue != 11001) {
        settings.setValue(appName, 11001);
        settings.sync();
    }
}

/**
 * @brief 初始化嵌入浏览器（IE/Edge ActiveX 控件）
 *
 * 使用 Qt ActiveX 容器 (axcontainer) 模块嵌入 Windows 的
 * Internet Explorer 浏览器引擎，无需 webenginewidgets 模块。
 */
void PowerVoltageWindow::initBrowser()
{
    // 1) 创建地址栏（默认百度，兼容性更好）
    m_urlEdit = new QLineEdit("https://www.baidu.com");
    m_urlEdit->setPlaceholderText("输入网址，回车或点击按钮导航");
    m_urlEdit->setMinimumHeight(28);

    // 2) 创建导航按钮
    QPushButton *navigateBtn = new QPushButton("导航");
    navigateBtn->setMinimumHeight(28);
    connect(navigateBtn, &QPushButton::clicked,
            this, &PowerVoltageWindow::on_navigateButton_clicked);
    connect(m_urlEdit, &QLineEdit::returnPressed,
            this, &PowerVoltageWindow::on_navigateButton_clicked);

    // 3) 工具栏布局（地址栏 + 按钮）
    QHBoxLayout *toolbarLayout = new QHBoxLayout();
    toolbarLayout->setSpacing(6);
    toolbarLayout->setContentsMargins(4, 4, 4, 4);
    toolbarLayout->addWidget(m_urlEdit);
    toolbarLayout->addWidget(navigateBtn);

    // 4) 创建嵌入式浏览器控件（IE ActiveX）
    m_browser = new QAxWidget(this);
    if (!m_browser->setControl(WEB_BROWSER_CLSID)) {
        QMessageBox::warning(this, "警告",
            "无法创建浏览器控件，请确保系统已安装 Internet Explorer。\n"
            "将使用系统浏览器作为备选方案。");
        delete m_browser;
        m_browser = nullptr;
    } else {
        // 禁止弹出脚本错误对话框
        m_browser->dynamicCall("Silent = true");
    }

    // 5) 组装布局：工具栏在上面，浏览器在下面
    ui->browserLayout->addLayout(toolbarLayout);
    if (m_browser) {
        // 浏览器占满剩余空间
        ui->browserLayout->addWidget(m_browser, 1);   // stretch=1

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
        // 备选方案：调用系统浏览器
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

    // 通过 IE 的 Navigate 方法加载网页
    m_browser->dynamicCall("Navigate(const QString&)", url);
}
