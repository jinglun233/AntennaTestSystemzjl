#ifndef SERVERPROTOCOL_H
#define SERVERPROTOCOL_H

#include <QByteArray>
#include <QList>
#include <QtGlobal>

// 前向声明：环形缓冲区（避免头文件依赖倒置）
class RingBuffer;

// ============================================================================
//
//                    ★ ServerProtocol ★
//
//  职责：解析服务端接收到的客户端上报数据（遥测子帧协议，0x68 帧头）
//
//  使用场景：
//    MainWindow 作为 TCP Server 运行时（模拟电子设备模式），
//    客户端（天线地检端）通过 TCP 上报原始字节流 → 本类完成帧同步、
//    校验、拆分子包，最终返回结构化的子帧列表。
//
//  对应 clientprotocol.h 的关系：
//    clientprotocol.h   → 客户端协议（EB90 帧头，固定1029字节标准帧）
//    serverprotocol.h   → 服务端协议（0x68 帧头，变长子帧，C# 遥测子帧）
//    两者完全独立，分别用于 MainWindow 的两种工作模式。
//
//  对应 C# 端方法：
//    FindData   → findFrame()     — 从环形缓冲区提取一帧
//    Validate   → validate()      — 累加和校验
//    ResolveData→ resolve()       — 完整流水线（find + 校验 + 拆分）
//
//  子帧类型识别（速变遥测/直接遥测）由 MainWindow 业务层负责，
//  本类仅负责协议层解析，不涉及业务分发逻辑。
//
//  协议帧格式（0x68 双帧头）：
//
//  ┌──────┬───────┬───────┬──────┬─────────────────┬──────────┐
//  │ 0x68 │ 长度H  │ 长度L  │ 0x68 │   数据域(leng)   │ 尾部3B   │
//  │ 1B   │ 1B    │ 1B    │ 1B   │  7~1017 字节     │ 校验等   │
//  └──────┴───────┴───────┴──────┴─────────────────┴──────────┘
//  ←—————— 帧头 4 字节 ————→←———— 数据（变长） ————→←— 固定 —→
//
//  总帧长 = leng + 7 （leng 有效范围：7 ~ 1017）
//
// ============================================================================

class ServerProtocol
{
public:

    // ==================== 帧格式常量 ====================

    /** 帧头标志字节 */
    static constexpr quint8 FRAME_HEADER_BYTE = 0x68;

    /** 最小有效帧长度（低于此值不可能包含完整帧） */
    static constexpr int MIN_FRAME_SIZE = 15;

    /** 数据域最小长度 */
    static constexpr int MIN_DATA_LENGTH = 7;

    /** 数据域最大长度 */
    static constexpr int MAX_DATA_LENGTH = 1017;

    /** 帧头+数据域+尾部 的固定开销 = 7 字节 */
    static constexpr int FRAME_OVERHEAD = 7;

    // ==================== 核心静态方法 ====================

    /**
     * @brief 从环形缓冲区搜索并提取一帧有效的 0x68 协议数据
     *
     * 对应 C# 方法：FindData(RingBuffer)
     *
     * 算法流程：
     *   1. 前置校验（空指针检查）
     *   2. 循环搜索帧头：索引0 == 0x68 且 索引3 == 0x68
     *      - 不匹配则逐字节丢弃（consume(1)），继续搜索
     *   3. 匹配到帧头后读取长度字段：leng = idx[1]*256 + idx[2]
     *   4. 长度合法性校验：7 <= leng <= 1017，否则丢弃首字节继续找
     *   5. 缓冲区剩余 >= leng+7 时，批量提取完整帧并返回
     *
     * @param ringBuffer  环形缓冲区指针（来自客户端连接的接收缓冲区）
     * @return 提取到的完整原始帧；数据不足或出错返回空 QByteArray
     */
    static QByteArray findFrame(RingBuffer *ringBuffer);

    /**
     * @brief 累加和校验 —— 验证数据段的有效性
     *
     * 对应 C# 方法：Validate(data, len)
     *
     * 校验原理：
     *   sum = data[0] + data[1] + ... + data[len-1]
     *   valid = (sum & 0xFF) == data[len]
     *
     * @param data  待校验的数据
     * @param len   参与校验的字节数（校验和位于 data[len] 处）
     * @return true=校验通过；false=校验失败或参数非法
     */
    static bool validate(const QByteArray &data, int len);

    /**
     * @brief 完整解析流水线 —— 从环形缓冲区提取所有遥测子帧
     *
     * 对应 C# 方法：ResolveData(RingBuffer)
     *
     * 执行步骤：
     *   ① findFrame()   — 从缓冲区提取一帧原始数据
     *   ② 全零检测      — 全零帧视为无效，直接丢弃
     *   ③ validate()    — 累加和校验（校验长度 = 数据长度-3）
     *   ④ 子段拆分      — 从索引11开始按 [类型][长度H][长度L] 格式逐个拆出子帧
     *
     * @param ringBuffer  环形缓冲区指针
     * @return 解析出的所有子帧列表；无数据返回空列表
     */
    static QList<QByteArray> resolve(RingBuffer *ringBuffer);
};

#endif // SERVERPROTOCOL_H
