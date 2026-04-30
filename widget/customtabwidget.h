#ifndef CUSTOMTABWIDGET_H
#define CUSTOMTABWIDGET_H

#include <QTabWidget>
#include "customtabbar.h"
#include <QShowEvent>
#include <QKeyEvent>
#include <QCloseEvent>

class CustomTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit CustomTabWidget(QWidget *parent = nullptr);
    ~CustomTabWidget();
    
    CustomTabBar *customTabBar() const;
    
    void replaceTabBar();
    
    // 记录 widget 的 UI 设计器原始几何尺寸（必须在 addTab 之前调用）
    void recordDesignSize(QWidget *widget, const QSize &designSize);
    
    void detachCurrentTab();
    
    // 关闭所有已分离的独立窗口（供主窗口关闭时调用）
    void closeAllDetachedWindows();

private slots:
    void handleTabDetached(int index, const QPoint &pos);

signals:
    void tabDetached(int index, const QPoint &pos);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    CustomTabBar *m_customTabBar;
    QMap<QWidget*, int> m_detachedWindows;  // 记录分离的窗口及其原始索引
    QMap<QWidget*, QString> m_detachedTitles;  // 记录分离的窗口标题
    QMap<QWidget*, QIcon> m_detachedIcons;  // 记录分离的窗口图标
    QMap<QWidget*, QSize> m_designSizes;     // 记录每个 widget 的 UI 设计器原始几何尺寸
};

#endif // CUSTOMTABWIDGET_H