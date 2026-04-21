#include "customtabwidget.h"
#include <QVBoxLayout>
#include <QMap>
#include <QApplication>
#include <QScreen>

CustomTabWidget::CustomTabWidget(QWidget *parent) : QTabWidget(parent)
{
    // 创建自定义 tab bar，但延迟设置（因为 setTabBar 是受保护函数）
    m_customTabBar = new CustomTabBar(this);
    
    // 对于在代码中创建的 CustomTabWidget，可以在构造函数中直接调用 replaceTabBar
    // 对于从 UI 文件加载的，需要在 MainWindow 构造函数中调用 replaceTabBar
}

CustomTabWidget::~CustomTabWidget()
{}

void CustomTabWidget::replaceTabBar()
{
    // 保存当前的 tab 信息
    QList<QWidget*> widgets;
    QList<QString> tabTexts;
    QList<QIcon> tabIcons;
    int count = this->count();
    for (int i = 0; i < count; ++i) {
        widgets.append(this->widget(i));
        tabTexts.append(this->tabText(i));
        tabIcons.append(this->tabIcon(i));
    }
    
    // 清除所有 tabs（因为 setTabBar 只能在无 tab 时调用）
    for (int i = count - 1; i >= 0; --i) {
        this->removeTab(i);
    }
    
    // 设置自定义 tab bar
    setTabBar(m_customTabBar);
    
    // 重新添加所有 tabs（同时保存每个 widget 的设计器原始几何尺寸）
    for (int i = 0; i < count; ++i) {
        int idx = this->addTab(widgets[i], tabIcons[i], tabTexts[i]);
        // 记录设计器中的原始几何尺寸（在 addTab 后 widget 尺寸已被布局覆盖，
        // 但如果之前未记录过，用当前 sizeHint 作为 fallback）
        if (!m_designSizes.contains(widgets[i])) {
            m_designSizes[widgets[i]] = widgets[i]->sizeHint();
        }
    }
    
    // 连接信号，处理拖动分离
    connect(m_customTabBar, &CustomTabBar::tabDetached, this, &CustomTabWidget::handleTabDetached);
}

CustomTabBar *CustomTabWidget::customTabBar() const
{
    return m_customTabBar;
}

void CustomTabWidget::recordDesignSize(QWidget *widget, const QSize &designSize)
{
    if (widget && designSize.isValid()) {
        m_designSizes[widget] = designSize;
    }
}

void CustomTabWidget::detachCurrentTab()
{
    int currentIndex = this->currentIndex();
    if (currentIndex < 0 || currentIndex >= this->count()) {
        return;
    }
    
    // 获取当前窗口位置，使用屏幕中心作为弹出窗口的初始位置
    QPoint pos = QCursor::pos();
    
    // 调用 handleTabDetached 处理分离
    handleTabDetached(currentIndex, pos);
}

void CustomTabWidget::keyPressEvent(QKeyEvent *event)
{
    // 检测是否按下了 Tab 键
    if (event->key() == Qt::Key_Tab && this->count() > 0) {
        detachCurrentTab();
        event->accept();
        return;
    }
    
    QTabWidget::keyPressEvent(event);
}

void CustomTabWidget::handleTabDetached(int index, const QPoint &pos)
{
    if (index < 0 || index >= this->count()) {
        return;
    }
    
    // 获取标签中的部件（在移除前先获取）
    QWidget *widget = this->widget(index);
    QString tabText = this->tabText(index);
    QIcon tabIcon = this->tabIcon(index);
    
    // 从当前部件中移除标签
    this->removeTab(index);
    
    // 保存原始信息，用于窗口关闭时恢复
    m_detachedWindows[widget] = index;
    m_detachedTitles[widget] = tabText;
    m_detachedIcons[widget] = tabIcon;
    
    // 直接设置 widget 为窗口模式，保留其原有的布局和 UI
    widget->setParent(nullptr);  // 移除父对象，使其成为独立窗口
    widget->setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint |
                          Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
    widget->setWindowTitle(tabText);
    if (!tabIcon.isNull()) {
        widget->setWindowIcon(tabIcon);
    }

    // 设置窗口大小为 UI 设计器中的原始设计尺寸（优先使用记录的设计尺寸）
    // 并在屏幕上居中显示
    QSize designSize;
    if (m_designSizes.contains(widget) && m_designSizes[widget].isValid()) {
        designSize = m_designSizes[widget];
    } else {
        // fallback：使用 widget 当前尺寸或 sizeHint
        designSize = widget->size();
        if (!designSize.isValid() || designSize.isEmpty()) {
            designSize = this->size();
        }
    }

    // 计算屏幕居中位置
    QScreen *screen = QApplication::screenAt(pos);
    if (!screen) screen = QApplication::primaryScreen();
    QRect screenGeom = screen->availableGeometry();
    int x = screenGeom.x() + (screenGeom.width() - designSize.width()) / 2;
    int y = screenGeom.y() + (screenGeom.height() - designSize.height()) / 2;

    widget->setGeometry(x, y, designSize.width(), designSize.height());
    
    // 安装事件过滤器，捕获关闭事件
    widget->installEventFilter(this);
    
    // 显示 widget（现在作为窗口）
    widget->show();
    
    // 发射 tabDetached 信号，通知外部有 tab 被分离
    emit tabDetached(index, pos);
}

bool CustomTabWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Close && m_detachedWindows.contains(qobject_cast<QWidget*>(obj))) {
        // 窗口正在关闭，将 widget 重新添加回 tab
        QWidget *widget = qobject_cast<QWidget*>(obj);
        if (widget) {
            // 移除事件过滤器
            widget->removeEventFilter(this);
            
            // 获取原始信息
            int index = m_detachedWindows[widget];
            QString title = m_detachedTitles[widget];
            QIcon icon = m_detachedIcons[widget];
            
            // 清除记录
            m_detachedWindows.remove(widget);
            m_detachedTitles.remove(widget);
            m_detachedIcons.remove(widget);
            
            // 恢复窗口标志
            widget->setWindowFlags(Qt::Widget);
            widget->setParent(this);
            
            // 重新添加 tab
            insertTab(index, widget, icon, title);
        }
    }
    
    return QTabWidget::eventFilter(obj, event);
}
