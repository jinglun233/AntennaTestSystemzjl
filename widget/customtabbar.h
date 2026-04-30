#ifndef CUSTOMTABBAR_H
#define CUSTOMTABBAR_H

#include <QTabBar>
#include <QMouseEvent>
#include <QPoint>

class CustomTabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit CustomTabBar(QWidget *parent = nullptr);
    ~CustomTabBar();

signals:
    void tabDetached(int index, const QPoint &pos);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool m_dragging;
    int m_dragIndex;
    QPoint m_dragStartPos;
    QPoint m_dragOffset;
    static const int DRAG_THRESHOLD = 30; // 拖动阈值（像素）
};

#endif // CUSTOMTABBAR_H