#ifndef AUTOTESTWINDOW_H
#define AUTOTESTWINDOW_H

#include <QDialog>

namespace Ui {
class AutoTestWindow;
}

class AutoTestWindow : public QDialog
{
    Q_OBJECT

public:
    explicit AutoTestWindow(QWidget *parent = nullptr);
    ~AutoTestWindow();

private slots:
    void on_connectButton_clicked();
    void on_selectWaveDirButton_clicked();
    void on_startAutoTestButton_clicked();

private:
    void appendResult(const QString &text);

    Ui::AutoTestWindow *ui;
    QString m_waveDirPath;
    bool m_connected;
};

#endif // AUTOTESTWINDOW_H
