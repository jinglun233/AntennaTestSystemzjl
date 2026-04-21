#include "customtabbar.h"

CustomTabBar::CustomTabBar(QWidget *parent) : QTabBar(parent)
{
    m_dragging = false;
    m_dragIndex = -1;
}

CustomTabBar::~CustomTabBar()
{}

void CustomTabBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        int index = tabAt(event->pos());
        if (index != -1) {
            m_dragIndex = index;
            m_dragStartPos = event->globalPos();
            m_dragOffset = event->pos();
            m_dragging = true;
            // 改变光标形状提供视觉反馈
            setCursor(Qt::OpenHandCursor);
        }
    }
    QTabBar::mousePressEvent(event);
}

void CustomTabBar::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        // 计算拖动距离
        QPoint currentPos = event->globalPos();
        int dragDistance = QLineF(m_dragStartPos, currentPos).length();
        
        // 只有当拖动距离超过阈值时才触发分离
        if (dragDistance >= DRAG_THRESHOLD) {
            // 拖动时改变光标为闭合手形
            if (cursor().shape() != Qt::ClosedHandCursor) {
                setCursor(Qt::ClosedHandCursor);
            }
            
            emit tabDetached(m_dragIndex, currentPos - m_dragOffset);
            m_dragging = false;
        }
    }
    
    QTabBar::mouseMoveEvent(event);
}

void CustomTabBar::mouseReleaseEvent(QMouseEvent *event)
{
    // 恢复光标形状
    if (cursor().shape() == Qt::OpenHandCursor || cursor().shape() == Qt::ClosedHandCursor) {
        unsetCursor();
    }
    
    m_dragging = false;
    m_dragIndex = -1;
    QTabBar::mouseReleaseEvent(event);
}
