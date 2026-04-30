#ifndef TEMPERATUREINFOWINDOW_H
#define TEMPERATUREINFOWINDOW_H

#include <QWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QVBoxLayout>
#include <QElapsedTimer>
#include <QScrollBar>

// 前向声明（cpp 中才实际 include）
class QCustomPlot;
class QCPItemTracer;
class QCPItemText;

namespace Ui {
class temperatureinfowindow;
}

/**
 * @brief 温度信息窗口
 *
 * 功能:
 * 1. 112路测温通道温度长表 + 50路开关状态长表
 * 2. 全通道温度实时曲线绘制
 * 3. 灵活配置测温通道与开关的对应映射关系
 * 4. 热控指令：加热回路手动控制、阈值上注、热控模块复位
 */
class TemperatureInfoWindow : public QWidget
{
    Q_OBJECT

public:
    explicit TemperatureInfoWindow(QWidget *parent = nullptr);
    ~TemperatureInfoWindow();

    // ========== 数据更新接口 ==========
    /// 解析并更新112路测温数据（来自遥测帧 payload）
    void parseThermalTelemetry(const QByteArray &payload);
    /// 更新单路测温温度值
    void updateTemperature(int channel, double temperature);
    /// 更新单路开关状态
    void updateSwitchStatus(int switchId, bool onOff);

public slots: // 热控指令操作
    void on_button_heatingManualSend_clicked();       // 加热回路手动控制发送
    void on_button_thermalReset_clicked();             // 热控模块复位
    // 映射配置
    void on_button_loadMapping_clicked();              // 加载映射文件
    void on_button_saveMapping_clicked();              // 保存映射到文件
    void on_button_applyMapping_clicked();             // 应用当前映射配置
    void on_button_batchSet_clicked();                 // 批量设置映射

    // ========== 曲线交互 ==========
    void onCurveMouseMoved(QMouseEvent *event);  // 鼠标移动显示数值
    void onTimeScrollChanged(int value);          // 时间滚动条变化
    void onSelectChannelTextChanged();            // 通道选择文本变化（回车/失焦触发）

signals:
    /// 向服务器发送原始热控控制指令
    void sendRawCommandToServer(const QByteArray &command);
    /// 加热手动控制信号（回路编号, 动作:0=关/1=开）
    void heatingManualControl(int circuitId, int action);
    /// 阈值上注信号（回路编号, 阈值温度）
    void thresholdUpload(int circuitId, double threshold);
    /// 热控模块复位信号
    void thermalReset();
    /// 映射配置变更通知
    void mappingConfigChanged(const QVector<int> &primaryChannels,
                              const QVector<int> &backupChannels);

private:
    // ========== 初始化方法 ==========
    void initTemperatureTable();    // 初始化 112 路测温长表
    void initSwitchTable();         // 初始化 50 路开关状态长表
    void initMappingTable();        // 初始化映射配置表格
    void initCurveContainer();      // 初始化 QCustomPlot 曲线

    // ========== 曲线更新 ==========
    void updateCurves();            // 刷新温度曲线显示

    // ========== 通道选择解析 ==========
    /// 解析通道选择文本（如 "1-20,25,40"），返回排序去重的通道号列表（1-based）
    QVector<int> parseChannelSelection(const QString &text) const;

private:
    Ui::temperatureinfowindow *ui;

    // ========== 常量 ==========
    static const int TEMP_CHANNEL_COUNT = 112;  // 测温通道总数
    static const int SWITCH_COUNT       = 50;   // 开关总数
    static const int CURVE_MAX_POINTS  = 300;  // 每条曲线最大数据点数（时间窗口）

    // ========== 映射配置缓存 ==========
    QVector<int> m_primaryChannels;     // 每个开关的主测温通道 (1~112)
    QVector<int> m_backupChannels;      // 每个开关的备份测温通道 (0=无)

    // ========== 温度数据缓存（供曲线使用） ==========
    QVector<double> m_temperatureData;  // 112 路温度数据缓存（最新值）

    // ========== 通道选择（曲线过滤显示） ==========
    QSet<int> m_selectedChannels;       // 当前选中的通道集合（1-based），空=全选

    // ========== 时间序列曲线数据 ==========
    QVector<QVector<double>> m_timeSeries;   // 每通道的时间序列数据 [channel][point]
    QVector<double> m_timeStamps;            // 全局时间戳队列
    double m_timeBase;                       // 时间基准（秒）
    QElapsedTimer m_elapsedTimer;            // 启动后计时器
    bool m_curvePaused;                      // 曲线暂停标志

    // ========== 曲线组件 ==========
    QCustomPlot *m_customPlot;         // 嵌入 curveContainer 的曲线实例
    QScrollBar *m_timeScrollBar;       // 时间滚动条
    QCPItemTracer *m_tracer;           // 十字准星追踪器
    QCPItemText *m_tooltipText;        // 悬浮提示文字
    bool m_scrollBarDragging;          // 滚动条拖拽标志

    // 曲线重绘节流（防止高频遥测导致主线程卡顿）
    QTimer *m_replotThrottleTimer;
    bool m_pendingReplot;
};

#endif // TEMPERATUREINFOWINDOW_H
