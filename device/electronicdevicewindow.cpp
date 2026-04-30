#include "electronicdevicewindow.h"
#include "ui_electronicdevicewindow.h"
#include "calc.h"

#include <QDebug>
#include <QMessageBox>
#include <QDateTime>
#include <QScrollBar>
#include <QLineEdit>

ElectronicDeviceWindow::ElectronicDeviceWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ElectronicDeviceWindow),
    m_calcNom(nullptr),
    m_calcDir(nullptr)
{
    ui->setupUi(this);
}

ElectronicDeviceWindow::~ElectronicDeviceWindow()
{
    delete m_calcNom;
    delete m_calcDir;
    delete ui;
}

bool ElectronicDeviceWindow::loadDecodeConfig(const QString &csvPath)
{
    try {
        Calc *calc = new Calc(csvPath);

        // 根据文件名自动分配到对应的解码器
        if (csvPath.contains(QStringLiteral("nom"), Qt::CaseInsensitive)) {
            delete m_calcNom;
            m_calcNom = calc;
            qDebug() << "[ElectronicDeviceWindow] 加载 nom 配置成功:" << csvPath;
        } else if (csvPath.contains(QStringLiteral("dir"), Qt::CaseInsensitive)) {
            delete m_calcDir;
            m_calcDir = calc;
            qDebug() << "[ElectronicDeviceWindow] 加载 dir 配置成功:" << csvPath;
        } else {
            // 兜底：nom 未占用则放 nom，否则放 dir
            if (!m_calcNom) {
                m_calcNom = calc;
                qDebug() << "[ElectronicDeviceWindow] 加载配置到 nom:" << csvPath;
            } else {
                m_calcDir = calc;
                qDebug() << "[ElectronicDeviceWindow] 加载配置到 dir:" << csvPath;
            }
        }
        return true;

    } catch (const std::exception &e) {
        qCritical() << "[ElectronicDeviceWindow] 配置加载失败:" << e.what();
        QMessageBox::warning(this,
            QString::fromUtf8("配置错误"),
            QString::fromUtf8("遥测配置文件加载失败！\n%1\n\n文件：%2")
                .arg(e.what()).arg(csvPath));
        return false;
    }
}

void ElectronicDeviceWindow::autoLoadDefaultConfigs()
{
    const QString basePath = QStringLiteral("D:/");
    QStringList configs = {
        basePath + QStringLiteral("calc_nom.csv"),
        basePath + QStringLiteral("calc_dir.csv")
    };

    int successCount = 0;
    for (const QString &path : configs) {
        if (loadDecodeConfig(path)) successCount++;
    }

    qDebug() << "[ElectronicDeviceWindow] 自动加载配置完成:"
             << successCount << "/" << configs.size() << "成功";
}

/**
 * @brief 通过控件名查找 QLineEdit 并设置文本
 * @return 是否找到并设置成功
 */
static bool setLineEditByName(QWidget *parentWidget, const QString &name, const QString &value)
{
    QLineEdit *edit = parentWidget->findChild<QLineEdit *>(name);
    if (edit) {
        edit->setText(value);
        return true;
    }
    return false;
}

void ElectronicDeviceWindow::decodeTelemetry(const QByteArray &payload)
{
    if (!m_calcNom && !m_calcDir) {
        qWarning() << "[ElectronicDeviceWindow] 未加载任何解码配置，请先调用 loadDecodeConfig()";
        return;
    }

    // ========== 遍历所有已加载的解码器进行解码 ==========
    int totalFields = 0;

    for (int pass = 0; pass < 2; ++pass) {
        Calc *calc = (pass == 0) ? m_calcNom : m_calcDir;
        if (!calc) continue;

        QString logLine;
        logLine.reserve(512);
        bool first = true;
        int fieldsThisPass = 0;

        for (int idx = 0; idx < 50; ++idx) {
            QString value = calc->Bit2Display(payload, idx);
            if (value.isEmpty()) break;

            totalFields++;
            fieldsThisPass++;

            // ========== 根据解码器类型映射到不同的 UI 控件组 ==========
            if (pass == 0) {
                // ---- NOM 解码器 → cgYCline_* 控件（后缀 = 字段索引 + 1）----
                QString ctrlName = QString("cgYCline_%1").arg(idx + 1);
                setLineEditByName(this, ctrlName, value);
            } else {
                // ---- DIR 解码器 → dirYCline_* 控件（后缀按实际UI编号）----
                // DIR 控件的编号不连续: 1,3,4,5,6（无2）
                // 使用查找表映射
                static const int dirSuffixMap[] = {1, 3, 4, 5, 6};  // dirIdx→UI后缀
                if (idx < 5) {
                    QString ctrlName = QString("dirYCline_%1").arg(dirSuffixMap[idx]);
                    setLineEditByName(this, ctrlName, value);
                }
                // idx >= 5 的额外字段也尝试用 dirYCline_{idx+1}
                else {
                    QString ctrlName = QString("dirYCline_%1").arg(idx + 1);
                    setLineEditByName(this, ctrlName, value);
                }
            }

            // 构建日志行
            if (!first) logLine.append(QStringLiteral(" | "));
            logLine.append(QString("[%1]%2").arg(idx).arg(value));
            first = false;
        }

        // 追加日志（标注来源）
        if (ui->logText_2 && !first) {
            QString tag = (pass == 0) ? QStringLiteral("NOM") : QStringLiteral("DIR");
            QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
            ui->logText_2->append(
                QString("[%1] [%2] %3").arg(timestamp).arg(tag).arg(logLine));

            QScrollBar *sb = ui->logText_2->verticalScrollBar();
            if (sb) sb->setValue(sb->maximum());
        }

        qDebug() << "[ElectronicDeviceWindow]" << ((pass == 0) ? "NOM" : "DIR")
                 << "解码完成," << fieldsThisPass << "个字段";
    }

    qDebug() << "[ElectronicDeviceWindow] 总计" << totalFields << "个字段";
}
