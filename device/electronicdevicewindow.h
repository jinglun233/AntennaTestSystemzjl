#ifndef ELECTRONICDEVICEWINDOW_H
#define ELECTRONICDEVICEWINDOW_H

#include <QWidget>
#include <QByteArray>
#include <QStringList>
#include <QList>
#include <QLineEdit>

// 前向声明 Calc 类（避免头文件依赖暴露到外部）
class Calc;
class QGroupBox;

namespace Ui {
class ElectronicDeviceWindow;
}

/**
 * @brief 电子设备窗口 —— 遥测数据解码与显示
 *
 * 职责：
 *   1. 启动时自动查找并排序所有遥测 QLineEdit 控件（findAndSortLineEdits）
 *   2. 加载 CSV 解码配置（calc_nom.csv / calc_dir.csv）
 *   3. 接收遥测子帧数据，通过 Calc 解码器解析并更新 UI
 *
 * 遥测类型：
 *   - 速变遥测 (updataYC)  → 使用 calc_nom 解码器，前缀 cgYCline_
 *   - 直接遥测 (updataDir)  → 使用 calc_dir 解码器，前缀 dirYCline_
 */
class ElectronicDeviceWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ElectronicDeviceWindow(QWidget *parent = nullptr);
    ~ElectronicDeviceWindow();

    /**
     * @brief 加载遥测解码配置 CSV 文件（通用单文件接口）
     * @param csvPath CSV 配置文件路径
     * @return 是否加载成功
     */
    bool loadDecodeConfig(const QString &csvPath);

    /**
     * @brief 启动时自动加载默认配置文件 + 初始化控件列表
     *
     * 自动从 D:\calc_nom.csv 和 D:\calc_dir.csv 加载两套解码配置，
     * 并调用 initLineEditLists() 查找排序所有遥测 QLineEdit。
     */
    void autoLoadDefaultConfigs();

public slots:
    /**
     * @brief 更新速变遥测数据（使用 calc_nom 解码）
     * @param data 原始子帧字节数据
     */
    void updataYC(const QByteArray &data);

    /**
     * @brief 更新直接遥测数据（使用 calc_dir 解码）
     * @param data 原始子帧字节数据
     */
    void updataDir(const QByteArray &data);

    /**
     * @brief 兼容旧接口 —— 根据子帧特征码自动分发到 updataYC/updataDir
     * @param payload 原始遥测帧字节数据
     */
    void decodeTelemetry(const QByteArray &payload);

signals:
    /** 日志更新信号（供外部连接到主窗口日志区）*/
    void signalUpdateLogText(const QString &text);

private:
    Ui::ElectronicDeviceWindow *ui;

    // ========== 遥测解码（两套配置） ==========
    Calc *m_calcNom;     // calc_nom.csv 解码器（速变遥测用）
    Calc *m_calcDir;     // calc_dir.csv 解码器（直接遥测用）

    // 公开别名（兼容用户代码中的 calc_nom / calc_dir 引用）
    Calc *calc_nom;      // = m_calcNom
    Calc *calc_dir;      // = m_calcDir

    // ========== 遥测 QLineEdit 列表（启动时通过 findAndSortLineEdits 填充）==========
    QList<QLineEdit*> YCdataQline;    ///< 速变遥测 QLineEdit 列表（cgYCline_* 排序后）
    QList<QLineEdit*> dirYCdataQline; ///< 直接遥测 QLineEdit 列表（dirYCline_* 排序后）

    // ========== 内部初始化 ==========
    void initLineEditLists();         // 启动时调用：查找+填充 YCdataQline / dirYCdataQline

public:
    // ==================== 静态工具方法 ====================

    /**
     * @brief 递归查找目标容器内所有 QLineEdit
     * @param parent 父容器 widget
     * @param outList 输出列表（累加追加）
     */
    static void findLineEditRecursively(QWidget *parent, QList<QLineEdit*> &outList);

    /**
     * @brief 查找并按指定前缀排序 QLineEdit（静态公开方法）
     *
     * @param targetGroupBox 目标 GroupBox 容器（或任意父 widget）
     * @param prefix        对象名前缀过滤条件（如 "cgYCline_" 或 "dirYCline_"）
     * @return 仅包含匹配 prefix 的、按数字后缀升序排列的 QLineEdit 列表
     *
     * 用法示例：
     *   QList<QLineEdit*> nomList = findAndSortLineEdits(groupBox, "cgYCline_");
     *   QList<QLineEdit*> dirList = findAndSortLineEdits(groupBox, "dirYCline_");
     */
    static QList<QLineEdit*> findAndSortLineEdits(QWidget *targetWidget, const QString &prefix);
};

#endif // ELECTRONICDEVICEWINDOW_H
