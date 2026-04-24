#ifndef INSTRUMENTCONTROLWINDOW_H
#define INSTRUMENTCONTROLWINDOW_H

#include <QWidget>

namespace Ui {
class InstrumentControlWindow;
}

class InstrumentControlWindow : public QWidget
{
    Q_OBJECT

public:
    explicit InstrumentControlWindow(QWidget *parent = nullptr);
    ~InstrumentControlWindow();

private slots:
    void on_connectInstrumentButton_clicked();
    void on_disconnectInstrumentButton_clicked();
    void on_initInstrumentButton_clicked();
    void on_readParamButton_clicked();
    void on_setParamButton_clicked();

private:
    void appendLog(const QString &text);

    Ui::InstrumentControlWindow *ui;
    bool m_instrumentConnected;
};

#endif // INSTRUMENTCONTROLWINDOW_H
