#include "mainwindow.h"

#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    // 1. 启用高DPI自动缩放（适配不同DPI屏幕）
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    // 2. 启用高DPI位图支持（避免图片/图标模糊）
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    // 3. 兼容QT5.12的DPI缩放模式（可选，针对Retina屏）
    qputenv("QT_SCALE_FACTOR_ROUNDING_POLICY", "PassThrough");

    QApplication a(argc, argv);

    // 加载全局样式表
    QFile qssFile(":/modernstyle.qss");
    if (qssFile.open(QFile::ReadOnly | QFile::Text)) {
        a.setStyleSheet(qssFile.readAll());
        qssFile.close();
    }

    MainWindow w;
    w.show();
    return a.exec();
}
