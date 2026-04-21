#include "dataprotocol.h"
#include <QtEndian>

// ============================================================================
//                           累加校验和（8位）
// ============================================================================

quint8 ProtocolCodec::calcSum8(const char *data, size_t len)
{
    quint32 sum = 0;
    for (size_t i = 0; i < len; ++i) {
        sum += static_cast<unsigned char>(data[i]);
    }
    return static_cast<quint8>(sum & 0xFF);
}

// ============================================================================
//                              编码
// ============================================================================

QByteArray ProtocolCodec::encode(const DataFrame &frame)
{
    QByteArray packet;
    packet.resize(Protocol::TOTAL_FRAME_SIZE); // 固定 1029 字节
    memset(packet.data(), 0, static_cast<size_t>(packet.size()));

    // 使用大端序（网络字节序）
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    // --- 帧头 (2 bytes): EB 90 ---
    stream << Protocol::FRAME_HEADER;

    // --- 帧类型 (1 byte): 0x04 ---
    stream << frame.frameType;

    // --- 指令类型 (1 byte), 如 0xC1 ---
    stream << frame.command;

    // --- 数据域 (1024 bytes) ---
    int payloadLen = frame.payload.size();
    if (payloadLen > 0) {
        int copyLen = payloadLen > Protocol::PAYLOAD_SIZE
                      ? Protocol::PAYLOAD_SIZE : payloadLen;
        // 从当前位置写入原始数据
        memcpy(packet.data() + Protocol::HEADER_SIZE + Protocol::FRAME_TYPE_SIZE + Protocol::COMMAND_SIZE,
               frame.payload.constData(), static_cast<size_t>(copyLen));
        // 剩余部分已为零（memset 初始化），无需额外补零
    }

    // --- 校验和 (1 byte): 对数据域累加求和，取低8位 ---
    const char *payloadPtr = packet.constData()
                             + Protocol::HEADER_SIZE
                             + Protocol::FRAME_TYPE_SIZE
                             + Protocol::COMMAND_SIZE;
    quint8 sum = calcSum8(payloadPtr, Protocol::PAYLOAD_SIZE);

    // 写入校验和到最后一字节
    packet[Protocol::TOTAL_FRAME_SIZE - 1] = static_cast<char>(sum);

    return packet;
}

// ============================================================================
//                              解析
// ============================================================================

bool ProtocolCodec::parse(const QByteArray &rawData, DataFrame &outFrame, int &consumed)
{
    consumed = 0;

    if (rawData.size() < Protocol::TOTAL_FRAME_SIZE) {
        return false; // 数据不足一整帧
    }

    // ====== 第一步：搜索帧头 ======
    int headerPos = findHeader(rawData);
    if (headerPos < 0) {
        // 整个数据中找不到帧头，全部丢弃
        consumed = rawData.size();
        return false;
    }

    if (headerPos > 0) {
        // 帧头前面有垃圾数据，跳过它们
        consumed = headerPos;
        return false;
    }

    // ====== 第二步：验证固定头部 ======
    const char *ptr = rawData.constData();

    // 验证帧头
    quint16 hdrValue = qFromBigEndian<quint16>(reinterpret_cast<const uchar*>(ptr));
    if (hdrValue != Protocol::FRAME_HEADER) {
        consumed = 1;
        return false;
    }

    // 验证帧类型 (固定 0x04)
    quint8 frameType = static_cast<quint8>(ptr[2]);
    if (frameType != Protocol::FRAME_TYPE) {
        // 帧类型不匹配，跳过首字节重搜
        consumed = 1;
        return false;
    }

    // 读取指令类型 (1 byte)
    quint8 command = static_cast<quint8>(ptr[3]);

    // 验证指令类型是否为已知合法值
    switch (command) {
    case Protocol::Cmd::TELEMETRY_DATA: // 遥测数据
        break;  // 合法指令类型，继续
    default:
        // 非法指令类型，跳过首字节重搜
        consumed = 1;
        return false;
    }

    // ====== 第三步：提取并校验数据域 ======
    // 数据域起始位置: header(2) + frameType(1) + command(1) = offset 4
    const char *payloadPtr = ptr + Protocol::HEADER_SIZE
                                     + Protocol::FRAME_TYPE_SIZE
                                     + Protocol::COMMAND_SIZE;

    // 计算数据域累加校验和
    quint8 calculatedSum = calcSum8(payloadPtr, Protocol::PAYLOAD_SIZE);

    // 接收到的校验和（最后一字节）
    quint8 receivedSum = static_cast<unsigned char>(
        ptr[Protocol::TOTAL_FRAME_SIZE - 1]);

    if (receivedSum != calculatedSum) {
        // 校验和不匹配，跳过首字节继续搜索
        consumed = 1;
        return false;
    }

    // ====== 第四步：解析成功 ======
    outFrame.frameType = frameType;
    outFrame.command   = command;
    outFrame.checksum  = receivedSum;
    outFrame.payload   = QByteArray(payloadPtr, Protocol::PAYLOAD_SIZE);

    consumed = Protocol::TOTAL_FRAME_SIZE; // 固定消费 1029 字节
    return true;
}

int ProtocolCodec::findHeader(const QByteArray &data)
{
    if (data.size() < 2) return -1;

    // 搜索 0xEB 0x90
    for (int i = 0; i <= data.size() - 2; ++i) {
        if (static_cast<unsigned char>(data[i]) == 0xEB &&
            static_cast<unsigned char>(data[i+1]) == 0x90) {
            return i;
        }
    }
    return -1;
}

bool ProtocolCodec::hasCompleteFrame(int availableBytes)
{
    return availableBytes >= Protocol::TOTAL_FRAME_SIZE;
}
