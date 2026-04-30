/**
 * @file  serverprotocol.cpp
 * @brief 服务端协议实现 —— 遥测子帧解析（0x68 帧头，变长帧）
 *
 * 本文件实现了服务端接收客户端上报数据时的完整解析链路。
 *
 * 数据流向：
 *   TCP 客户端（天线地检端）
 *     → 原始字节流写入 RingBuffer
 *       → ServerProtocol::resolve(ringBuffer)    （帧同步+校验+拆分）
 *         → MainWindow 内联判断子帧类型           （速变遥测/直接遥测）
 *           → updataDir()                         （分发到电子设备窗口）
 *
 * 对应 C# 方法：
 *   C# FindData    → findFrame()
 *   C# Validate    → validate()
 *   C# ResolveData → resolve()
 *
 * 注意：本类只负责协议层解析（到 resolve 为止），
 *       子帧类型识别和业务分发由 MainWindow 负责。
 */

#include "serverprotocol.h"
#include "ringbuffer.h"

#include <QDebug>

// ============================================================================
//                       静态常量初始化
// ============================================================================

// （本类无额外静态常量需要初始化）

// ============================================================================
//
//                      ★ findFrame ★ — 帧搜索与提取
//
//  功能：从环形缓冲区中找到一帧完整的 0x68 协议数据并提取出来
//  对应：C# 的 FindData(RingBuffer) 方法
//
//  帧格式：
//  ┌──────┬───────┬───────┬──────┬─────────────────┬──────────┐
//  │ 0x68 │ 长度H  │ 长度L  │ 0x68 │   数据域(leng)   │ 尾部3B   │
//  └──────┴───────┴───────┴──────┴─────────────────┴──────────┘
//
// ============================================================================

QByteArray ServerProtocol::findFrame(RingBuffer *ringBuffer)
{
    // ---------- 前置校验 ----------
    if (ringBuffer == nullptr)
    {
        return QByteArray();
    }

    try
    {
        // ---------- 外层循环：确保缓冲区有足够数据进行帧搜索 ----------
        while (ringBuffer->readableBytes() > MIN_FRAME_SIZE)
        {
            // 预览当前缓冲区内容（不消费数据），用于按索引访问各字节
            size_t available = ringBuffer->readableBytes();
            size_t peekSize = std::min(available, static_cast<size_t>(65536));
            QByteArray preview = ringBuffer->peek(peekSize);
            if (preview.isEmpty()) return QByteArray();

            // ---------- 内层循环：帧头同步 ----------
            // 帧头规则：索引0 == 0x68 且 索引3 == 0x68（双0x68标记）
            while (static_cast<uint8_t>(preview.at(0)) != FRAME_HEADER_BYTE
                   || static_cast<uint8_t>(preview.at(3)) != FRAME_HEADER_BYTE)
            {
                // 丢弃头部1个字节（对应 C# 的 Dequeue(singleByteBuf, 1)）
                ringBuffer->consume(1);

                if (ringBuffer->readableBytes() < MIN_FRAME_SIZE)
                {
                    return QByteArray();
                }

                // 重新预览（consume 后缓冲区内容已变化）
                available = ringBuffer->readableBytes();
                peekSize = std::min(available, static_cast<size_t>(65536));
                preview = ringBuffer->peek(peekSize);
                if (preview.isEmpty()) return QByteArray();
            }

            // ---------- 帧头已匹配，读取数据长度字段 ----------
            // 长度字段位于索引1（高字节）和索引2（低字节），大端序
            int leng = static_cast<uint8_t>(preview.at(1)) * 256
                     + static_cast<uint8_t>(preview.at(2));

            // ---------- 长度合法性校验 ----------
            if (leng > MAX_DATA_LENGTH || leng < MIN_DATA_LENGTH)
            {
                ringBuffer->consume(1);
                continue;
            }

            // ---------- 检查缓冲区是否有足够数据容纳完整帧 ----------
            size_t totalFrameLen = static_cast<size_t>(leng) + FRAME_OVERHEAD;

            if (ringBuffer->readableBytes() >= totalFrameLen)
            {
                // 数据充足：批量读取整帧（read 内部自动移动读指针）
                QByteArray validData = ringBuffer->read(totalFrameLen);

                if (validData.length() != static_cast<int>(totalFrameLen))
                {
                    qWarning() << "[ServerProtocol::findFrame]"
                               << "批量提取失败，期望" << totalFrameLen
                               << "字节，实际" << validData.length() << "字节";
                    return QByteArray();
                }

                return validData;
            }
            else
            {
                // 数据不足一帧：等下一批数据
                return QByteArray();
            }
        }

        return QByteArray();
    }
    catch (...)
    {
        qWarning() << "[ServerProtocol::findFrame] 匹配数据时出现未知异常";
        return QByteArray();
    }
}

// ============================================================================
//
//                      ★ validate ★ — 累加和校验
//
// ============================================================================
bool ServerProtocol::validate(const QByteArray &data, int len)
{
    // ======================================
    // 前置健壮性校验
    // ======================================
    // 1. 数据为空、len无效
    if (data.isEmpty() || len < 1)
    {
        qWarning() << "【Validate】无效参数：数据为空或len小于1";
        return false;
    }
    // 2. 数据长度不足（至少需要 len+2 个字节，才能访问 data[len]、data[len+1]）
    if (data.length() < len + 2)
    {
        qWarning() << "【Validate】数据长度不足，无法完成校验";
        return false;
    }

    // ======================================
    // 对应C#逻辑：初始化变量
    // ======================================
    bool flag = false;
    quint16 ax = 0xFFFF;  // 对应C# ushort ax（无符号16位整数，Qt用quint16更规范）
    quint16 lsb = 0;       // 对应C# ushort lsb（存储最低位）
    quint16 i = 0, j = 0;  // 对应C# ushort i, j（循环变量）

    // ======================================
    // 对应C#：外层循环（i从1到len-1，遍历数据参与校验）
    // ======================================
    for (i = 1; i < static_cast<quint16>(len); i++)
    {
        // 1. 取出当前字节（data[i]），与ax异或（对应C# ax ^= ((byte)data[i])）
        quint8 currentByte = static_cast<quint8>(data.at(i)); // 转换为无符号8位整数，对应C# byte
        ax ^= currentByte;

        // 2. 内层循环（j从0到7，处理当前字节的每一位）
        for (j = 0; j < 8; j++)
        {
            // 取ax的最低位（对应C# lsb = (ushort)(ax & 0x0001)）
            lsb = ax & 0x0001;

            // ax右移1位（对应C# ax = (ushort)(ax >> 1)）
            ax >>= 1;

            // 最低位不为0时，ax异或0xA001（CRC16-MODBUS的核心多项式）
            if (lsb != 0)
            {
                ax ^= 0xA001;
            }
        }
    }
    // ======================================
    // 对应C#：BitConverter.GetBytes(ax) 拆分16位整数为2字节数组
    // ======================================
    // C# BitConverter.GetBytes(ushort)返回2字节数组，小端模式下：b[0]是低字节，b[1]是高字节
    // 直接拆分quint16 ax为两个字节，与C#结果保持一致
    quint8 b0 = static_cast<quint8>(ax & 0x00FF);  // 低字节（对应C# b[0]）
    quint8 b1 = static_cast<quint8>((ax >> 8) & 0x00FF); // 高字节（对应C# b[1]）

    // ======================================
    // 对应C#：对比data[len] == b[1] && data[len+1] == b[0]
    // ======================================
    quint8 dataLenByte = static_cast<quint8>(data.at(len));
    quint8 dataLen1Byte = static_cast<quint8>(data.at(len + 1));

    if (dataLenByte == b1 && dataLen1Byte == b0)
    {
        flag = true;
    }

    // ======================================
    // 返回校验结果
    // ======================================
    return flag;
}

// ============================================================================
//
//                      ★ resolve ★ — 完整解析流水线
//
// ============================================================================

QList<QByteArray> ServerProtocol::resolve(RingBuffer *ringBuffer)
{
    QList<QByteArray> resultList;

    // 步骤①：findFrame 提取一帧原始数据
    QByteArray data = findFrame(ringBuffer);
    if (data.isEmpty()) return resultList;

    // 步骤②：全零检测（全零帧视为无效填充）
    bool isNonZeroData = false;
    for (char byteChar : data)
    {
        if (static_cast<uint8_t>(byteChar) != 0) { isNonZeroData = true; break; }
    }
    if (!isNonZeroData) return resultList;

    // 步骤③：累加和校验（校验长度 = 数据长度 - 3）
    int validateLength = data.length() - 3;
    if (!validate(data, validateLength)) return resultList;

    // 步骤④：子段拆分（从偏移11开始按 [类型][长度H][长度L] 格式逐个解析）
    if (data.length() > 14)
    {
        int p = 11;   // 子段起始偏移量

        while (p < data.length() - 3)
        {
            quint8 lenHigh = static_cast<quint8>(data.at(p + 1));
            quint8 lenLow  = static_cast<quint8>(data.at(p + 2));
            int subLen = lenHigh * 256 + lenLow;

            QByteArray subFrame(subLen + 1, 0);
            subFrame[0] = data.at(p);    // 类型码
            p += 3;

            for (int i = 1; i < subFrame.length(); ++i)
            {
                if (p >= data.length()) break;
                subFrame[i] = data.at(p);
                ++p;
            }

            resultList.append(subFrame);
        }
    }

    return resultList;
}
