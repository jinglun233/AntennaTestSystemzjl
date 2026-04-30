/**
 * @file  electronicdevicewindow.cpp
 * @brief 电子设备窗口实现 —— 遥测数据解码与显示
 *
 * 本文件实现电子设备窗口的完整功能：
 *   1. 启动时通过 findAndSortLineEdits 查找并排序所有遥测 QLineEdit 控件
 *   2. 加载 CSV 解码配置（calc_nom / calc_dir 两套）
 *   3. updataYC() / updataDir() 分别处理速变遥测和直接遥测的解码与 UI 更新
 *
 * 数据流向：
 *   TCP 数据 → ServerProtocol::resolve() → MainWindow::onServerDataReceived()
 *     → ElectronicDeviceWindow::updataYC() / updataDir()
 *       → Calc::Bit2Display() 解码 → signalUpdateYCLineEdit → UI 更新
 */

#include "electronicdevicewindow.h"
#include "ui_electronicdevicewindow.h"
#include "calc.h"

#include <QDebug>
#include <QMessageBox>
#include <QDateTime>
#include <QScrollBar>
#include <QLineEdit>
#include <QGroupBox>
#include <QMetaObject>   // QMetaObject::invokeMethod（线程安全 UI 更新）
#include <algorithm>     // std::sort

// ============================================================================
//                              构造 / 析构
// ============================================================================

ElectronicDeviceWindow::ElectronicDeviceWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ElectronicDeviceWindow),
    m_calcNom(nullptr),
    m_calcDir(nullptr),
    calc_nom(nullptr),    // 别名，构造后由 autoLoadDefaultConfigs 或 loadDecodeConfig 赋值
    calc_dir(nullptr)
{
    ui->setupUi(this);

    // 启动时立即查找并填充遥测控件列表
    initLineEditLists();
}

ElectronicDeviceWindow::~ElectronicDeviceWindow()
{
    delete m_calcNom;
    delete m_calcDir;
    delete ui;
}

// ============================================================================
//
//                    ★ initLineEditLists ★ — 初始化控件列表
//
//  在构造函数中调用，查找所有 cgYCline_* 和 dirYCline_* 控件并按序号排序。
//
// ============================================================================

void ElectronicDeviceWindow::initLineEditLists()
{
    // ---- 速变遥测控件：前缀 "cgYCline_" ----
    YCdataQline = findAndSortLineEdits(this, QStringLiteral("cgYCline_"));
    qDebug() << "[ElectronicDeviceWindow] 找到" << YCdataQline.count() << "个速变遥测控件 (cgYCline_*)";

    // ---- 直接遥测控件：前缀 "dirYCline_" ----
    dirYCdataQline = findAndSortLineEdits(this, QStringLiteral("dirYCline_"));
    qDebug() << "[ElectronicDeviceWindow] 找到" << dirYCdataQline.count() << "个直接遥测控件 (dirYCline_*)";
}

// ============================================================================
//
//                 ★ findLineEditRecursively ★ — 递归查找所有 QLineEdit
//
// ============================================================================

void ElectronicDeviceWindow::findLineEditRecursively(QWidget *parent, QList<QLineEdit*> &outList)
{
    if (!parent) return;

    // 查找直接子对象中的 QLineEdit
    QList<QLineEdit*> children = parent->findChildren<QLineEdit*>();
    for (QLineEdit *le : children) {
        if (le) {
            outList.append(le);
        }
    }
}

// ============================================================================
//
//                  ★ findAndSortLineEdits ★ — 查找+过滤+排序
//
// ============================================================================

QList<QLineEdit*> ElectronicDeviceWindow::findAndSortLineEdits(QWidget *targetWidget, const QString &prefix)
{
    QList<QLineEdit*> allLineEdits;      // 所有查找到的 QLineEdit
    QList<QLineEdit*> targetLineEdits;   // 仅存放符合 prefix 的目标元素

    // 步骤0：递归查找容器内全部 QLineEdit
    findLineEditRecursively(targetWidget, allLineEdits);

    QString effectivePrefix = prefix;

    // 步骤1：过滤 — 仅保留符合 prefix 前缀且能提取有效数字的目标元素
    for (QLineEdit *le : allLineEdits)
    {
        if (!le) continue;   // 跳过空指针

        QString objName = le->objectName();

        // 条件1：必须以指定前缀开头
        if (!objName.startsWith(effectivePrefix)) {
            continue;   // 非目标前缀，直接丢弃
        }

        // 条件2：前缀后必须是有效整数
        QString numStr = objName.mid(effectivePrefix.length());
        bool isNumber = false;
        numStr.toInt(&isNumber);

        if (isNumber) {
            targetLineEdits.append(le);   // 符合两个条件才加入最终列表
        }
    }

    // 步骤2：对目标列表按数字后缀升序排序
    std::sort(targetLineEdits.begin(), targetLineEdits.end(),
              [effectivePrefix](QLineEdit *a, QLineEdit *b) -> bool
    {
        if (a == nullptr || b == nullptr) {
            return a == nullptr;   // 空指针排前面
        }

        QString nameA = a->objectName();
        QString nameB = b->objectName();

        // 提取数字部分比较
        QString numStrA = nameA.mid(effectivePrefix.length());
        QString numStrB = nameB.mid(effectivePrefix.length());

        int numA = numStrA.toInt();
        int numB = numStrB.toInt();

        return numA < numB;
    });

    // 步骤3：返回仅包含目标前缀的排序列表
    return targetLineEdits;
}

// ============================================================================
//
//                      ★ 配置加载 ★
//
// ============================================================================

bool ElectronicDeviceWindow::loadDecodeConfig(const QString &csvPath)
{
    try {
        Calc *calc = new Calc(csvPath);

        // 根据文件名自动分配到对应的解码器
        if (csvPath.contains(QStringLiteral("nom"), Qt::CaseInsensitive)) {
            delete m_calcNom;
            m_calcNom = calc;
            calc_nom = m_calcNom;   // 同步别名
            qDebug() << "[ElectronicDeviceWindow] 加载 nom 配置成功:" << csvPath;
        } else if (csvPath.contains(QStringLiteral("dir"), Qt::CaseInsensitive)) {
            delete m_calcDir;
            m_calcDir = calc;
            calc_dir = m_calcDir;   // 同步别名
            qDebug() << "[ElectronicDeviceWindow] 加载 dir 配置成功:" << csvPath;
        } else {
            // 兜底：nom 未占用则放 nom，否则放 dir
            if (!m_calcNom) {
                m_calcNom = calc;
                calc_nom = m_calcNom;
                qDebug() << "[ElectronicDeviceWindow] 加载配置到 nom:" << csvPath;
            } else {
                m_calcDir = calc;
                calc_dir = m_calcDir;
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

// ============================================================================
//
//                    ★ updataYC ★ — 更新遥测
//
//
// ============================================================================

void ElectronicDeviceWindow::updataYC(const QByteArray &data)
{
    if (!calc_nom) {
        qWarning() << "[ElectronicDeviceWindow::updataYC] calc_nom 未初始化，跳过";
        return;
    }

    // ======================================
    // 步骤1：截取子字节数组（从索引5开始，去掉末尾1字节）
    // ======================================
    const int startIndex = 5;
    int length = data.length() - startIndex - 1;
    QByteArray datanew = data.mid(startIndex, length);

    QString logText = QString("%1 %2")
                        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
                        .arg(tr("电子设备遥测接收成功"));

    // 写入电子设备界面自身的日志区
    if (ui->logText_2) {
        ui->logText_2->append(logText);
    }

    // ======================================
    // 步骤2：遍历 YCdataQline，调用 Bit2Display 填充数据
    // ======================================
    for (int i = 0; i < YCdataQline.count(); ++i)
    {
        QLineEdit* currentLineEdit = YCdataQline[i];
        if (!currentLineEdit) continue;

        // 获取当前 QLineEdit 的对象名称
        QString objName = currentLineEdit->objectName();

        // 提取对象名称末尾的数字序号
        int lineEditIndex = -1;
        QString numStr;

        for (int j = objName.length() - 1; j >= 0; --j)
        {
            QChar c = objName.at(j);
            if (c.isDigit()) {
                numStr.prepend(c);    // 前置添加保证顺序正确（如"10"不变成"01"）
            } else {
                break;               // 遇到非数字停止
            }
        }

        bool convertOk = false;
        if (!numStr.isEmpty()) {
            lineEditIndex = numStr.toInt(&convertOk);
        }

        // 调用 calc_nom 的 Bit2Display 方法获取解析后的值
        QString displayValue = calc_nom->Bit2Display(datanew, lineEditIndex - 1);

        // 特殊处理：cgYCline_5 — 首次成像时间秒值
        if (objName == "cgYCline_5")
        {
            bool timeConvertOk = false;
            uint64_t seconds = displayValue.toULongLong(&timeConvertOk);
            if (timeConvertOk)
            {
                QDateTime baseTime(QDate(2020, 1, 1), QTime(0, 0, 0));
                QDateTime targetTime = baseTime.addSecs(static_cast<qint64>(seconds));
                displayValue = targetTime.toString("yyyy-MM-dd HH:mm:ss");
            }
        }

        // 特殊处理：cgYCline_8 — 当前任务首次成像时间秒值
        if (objName == "cgYCline_8")
        {
            bool timeConvertOk = false;
            uint64_t seconds = displayValue.toULongLong(&timeConvertOk);
            if (timeConvertOk)
            {
                QDateTime baseTime(QDate(2020, 1, 1), QTime(0, 0, 0));
                QDateTime targetTime = baseTime.addSecs(static_cast<qint64>(seconds));
                displayValue = targetTime.toString("yyyy-MM-dd HH:mm:ss");
            }
        }

        // 直接更新 QLineEdit（QMetaObject::invokeMethod 确保线程安全）
        QMetaObject::invokeMethod(currentLineEdit, "setText",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, displayValue));
    }
}

// ============================================================================
//
//                   ★ updataDir ★ — 更新直接遥测
//
//  对应 C# 端直接遥测解码逻辑：
//    从子帧索引2开始截取数据 → 遍历 dirYCdataQline → calc_dir->Bit2Display → 填充UI
//    含热敏电阻/电压特殊计算逻辑
//
// ============================================================================

void ElectronicDeviceWindow::updataDir(const QByteArray &data)
{
    if (!calc_dir) {
        qWarning() << "[ElectronicDeviceWindow::updataDir] calc_dir 未初始化，跳过";
        return;
    }

    // ======================================
    // 步骤1：截取子字节数组（从索引2开始）
    // ======================================
    const int startIndex = 2;
    int length = data.length() - startIndex;
    QByteArray datanew = data.mid(startIndex, length);

    QString logText = QString("%1 %2")
                        .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"))
                        .arg(tr("电子设备遥测接收成功"));

    // 写入电子设备界面自身的日志区
    if (ui->logText_2) {
        ui->logText_2->append(logText);
    }

    // ======================================
    // 步骤2：遍历 dirYCdataQline，调用 Bit2Display 填充数据
    // ======================================
    for (int i = 0; i < dirYCdataQline.count(); ++i)
    {
        QLineEdit* currentLineEdit = dirYCdataQline[i];
        if (!currentLineEdit) continue;

        // 获取当前 QLineEdit 的对象名称
        QString objName = currentLineEdit->objectName();

        // 提取对象名称末尾的数字序号
        int lineEditIndex = -1;
        QString numStr;

        for (int j = objName.length() - 1; j >= 0; --j)
        {
            QChar c = objName.at(j);
            if (c.isDigit()) {
                numStr.prepend(c);
            } else {
                break;
            }
        }

        bool convertOk = false;
        if (!numStr.isEmpty()) {
            lineEditIndex = numStr.toInt(&convertOk);
        }

        // 调用 calc_dir 的 Bit2Display 方法获取解析后的值
        QString displayValue = calc_dir->Bit2Display(datanew, lineEditIndex - 1);

        double value = 0.0;
        bool convertSuccess = false;

        // 特殊处理：lineEditIndex 为 6 或 18 时执行热敏电阻逻辑
        if (lineEditIndex == 6 || lineEditIndex == 18)
        {
            value = displayValue.toDouble(&convertSuccess);
            if (convertSuccess)
            {
                value = value * 5 / 4096;           // 转电压
                value = value * 4.7 / (5 - value);  // 转电阻 (kΩ)
                displayValue = QString::number(value, 'f', 3);
            }
        }
        else
        {
            // 默认电压值计算逻辑
            value = displayValue.toDouble(&convertSuccess);
            if (convertSuccess)
            {
                value = value * 5 / 4096;
                displayValue = QString::number(value, 'f', 3);   // 保留3位小数
            }
        }

        // 直接更新 QLineEdit（QMetaObject::invokeMethod 确保线程安全）
        QMetaObject::invokeMethod(currentLineEdit, "setText",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, displayValue));
    }
}

// ============================================================================
//
//                   ★ decodeTelemetry ★ — 兼容入口（自动分发）
//
//  根据子帧特征码判断类型，分发到 updataYC 或 updataDir。
//  供 MainWindow 调用（兼容旧的 decodeTelemetry 接口）。
//
// ============================================================================

void ElectronicDeviceWindow::decodeTelemetry(const QByteArray &payload)
{
    if (payload.length() < 5) return;

    // 速变遥测特征码：sub[3]==0x3A && sub[4]==0xFF
    if (payload.length() > 4
        && payload.at(3) == char(0x3A)
        && static_cast<uint8_t>(payload.at(4)) == 0xFF)
    {
        updataYC(payload);
    }
    // 直接遥测特征码：sub[0]==0x51 && sub[1]==0xAA
    else if (payload.at(0) == char(0x51)
             && static_cast<uint8_t>(payload.at(1)) == 0xAA)
    {
        updataDir(payload);
    }
}
