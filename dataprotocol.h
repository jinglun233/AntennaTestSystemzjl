#ifndef DATAPROTOCOL_H
#define DATAPROTOCOL_H

#include <QByteArray>
#include <QDataStream>
#include <QtGlobal>
#include <cstring>

// ============================================================================
//                          帧格式定义（回复数据帧）
// ============================================================================
//
//  ┌────────┬───────────┬────────────┬──────────────┬───────────┐
//  │  帧头   │  帧类型    │  指令类型    │   数据域      │  校验和    │
//  │ 16Bits │  8Bits    │  8Bits     │ 8*1024 Bits   │  8Bits    │
//  │ 2 字节  │ 1 字节    │ 1 字节     │ 1024 字节     │ 1 字节    │
//  └────────┴───────────┴────────────┴──────────────┴───────────┘
//
// 总长度 = 固定 1029 字节
//
// 字段说明：
//   - 帧头 (Header):     固定值 0xEB90，用于帧同步（2字节）
//   - 帧类型 (FrameType):固定值 0x04（1字节）
//   - 指令类型 (CmdType): 功能码，如 0xC1=遥测数据（1字节）
//   - 数据域 (Payload):   遥测有效数据，固定 1024 字节
//   - 校验和 (Checksum):  数据域的累加校验和（1字节）
//

// ==================== 常量定义 ====================

namespace Protocol {

// 帧头魔术字 (16 bits / 2 bytes)
constexpr quint16 FRAME_HEADER = 0xEB90;

// 帧类型 (8 bits / 1 byte) - 固定值
constexpr quint8 FRAME_TYPE = 0x04;

// 各字段固定大小（字节）
constexpr int HEADER_SIZE      = 2;  // EB 90
constexpr int FRAME_TYPE_SIZE  = 1;  // 0x04
constexpr int COMMAND_SIZE     = 1;  // 指令类型 (如 0xC1)
constexpr int PAYLOAD_SIZE     = 1024; // 数据域固定 1024 字节
constexpr int CHECKSUM_SIZE    = 1;  // 累加校验和 (1 byte)

// 帧头部总开销（不含数据体）= header + frameType + command + checksum
constexpr int HEADER_AND_TAIL_SIZE = HEADER_SIZE + FRAME_TYPE_SIZE
                                     + COMMAND_SIZE + CHECKSUM_SIZE; // = 5

// 整帧固定总长度 = 1029 字节
constexpr int TOTAL_FRAME_SIZE = HEADER_AND_TAIL_SIZE + PAYLOAD_SIZE; // = 1029

// ==================== 指令类型码定义 ====================
namespace Cmd {
    constexpr quint8 TELEMETRY_DATA  = 0xC1;  // 遥测数据
} // namespace Cmd

} // namespace Protocol

// ============================================================================
//                           数据结构
// ============================================================================

/**
 * @brief 完整的数据帧结构（新协议格式）
 */
struct DataFrame {
    quint8   frameType = Protocol::FRAME_TYPE;  // 帧类型, 默认 0x04
    quint8   command   = 0;                     // 指令类型 (1字节)
    QByteArray payload;                         // 数据域, 固定 1024 字节
    quint8   checksum  = 0;                     // 累加校验和 (1字节)

    /** 是否有效 */
    bool isValid() const { return command != 0 || !payload.isEmpty(); }
};

// ============================================================================
//                            帧编解码器
// ============================================================================

/**
 * @brief 协议帧的打包与解析工具类
 *
 * 用法：
 *   打包:  QByteArray packet = ProtocolCodec::encode(frame);
 *   解析:  bool ok = ProtocolCodec::parse(rawData, &outFrame, &consumed);
 */
class ProtocolCodec
{
public:
    // ==================== 编码（发送端） ====================

    /**
     * @brief 将 DataFrame 序列化为可发送的字节数组
     * @param frame 要编码的数据帧
     * @return 完整的帧字节流（含头部+校验），固定 1029 字节
     *
     * 自动计算并填充 checksum。
     * 若 payload 不足 1024 字节，自动补零；超出则截断。
     */
    static QByteArray encode(const DataFrame &frame);

    // ==================== 解码（接收端） ====================

    /**
     * @brief 尝试从原始字节流中解析出一帧完整数据
     *
     * 此方法设计用于配合环形缓冲区使用：
     *   1. 先从 RingBuffer::peek() 获取足够字节预览
     *   2. 调用本方法尝试解析
     *   3. 若成功，根据 consumed 值调用 RingBuffer::consume()
     *
     * 新协议帧长固定为 1029 字节。
     *
     * @param rawData   原始数据（通常来自环形缓冲区的 peek）
     * @param outFrame  [输出] 解析出的帧
     * @param consumed  [输出] 这帧消费了多少字节
     * @return true=成功解析一帧；false=数据不足或校验失败
     */
    static bool parse(const QByteArray &rawData, DataFrame &outFrame, int &consumed);

    /**
     * @brief 在字节流中搜索帧头位置 (0xEB90)
     * @param data 待搜索数据
     * @return 找到返回索引位置；未找到返回 -1
     */
    static int findHeader(const QByteArray &data);

    /**
     * @brief 检查当前可用数据是否至少包含一个完整帧（1029字节）
     */
    static bool hasCompleteFrame(int availableBytes);

private:
    // ==================== 累加校验和计算 ====================

    /**
     * @brief 计算累加校验和（对数据域所有字节求和，取低8位）
     * @param data 数据指针
     * @param len  数据长度
     * @return 累加和（8位）
     */
    static quint8 calcSum8(const char *data, size_t len);

    static quint8 calcSum8(const QByteArray &data) {
        return calcSum8(data.constData(), static_cast<size_t>(data.size()));
    }
};

#endif // DATAPROTOCOL_H
