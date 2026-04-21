#include "ringbuffer.h"
#include <cstring>
#include <algorithm>

// ============================================================================
//                           构造 / 析构
// ============================================================================

RingBuffer::RingBuffer(size_t capacity)
    : m_buffer(capacity, '\0')
    , m_readPos(0)
    , m_writePos(0)
{
}

// ============================================================================
//                              写入
// ============================================================================

size_t RingBuffer::write(const char *data, size_t len)
{
    if (len == 0) return 0;

    QMutexLocker locker(&m_writeMutex);
    ensureSpace(len);

    size_t bufSize = m_buffer.size();

    // 计算当前可写空间
    size_t readable = (m_writePos >= m_readPos)
                          ? (m_writePos - m_readPos)
                          : (bufSize - m_readPos + m_writePos);

    if (readable + len > bufSize) {
        // 确保扩容后足够（ensureSpace 已处理）
        bufSize = m_buffer.size();
    }

    // 分两段写入（处理回绕）
    size_t first = std::min(len, bufSize - m_writePos);
    std::memcpy(m_buffer.data() + m_writePos, data, first);

    size_t second = len - first;
    if (second > 0) {
        std::memcpy(m_buffer.data(), data + first, second);
    }

    m_writePos = (m_writePos + len) % bufSize;
    return len;
}

size_t RingBuffer::write(const QByteArray &data)
{
    return write(data.constData(), static_cast<size_t>(data.size()));
}

// ============================================================================
//                            查询
// ============================================================================

size_t RingBuffer::readableBytes() const
{
    QMutexLocker locker(&m_readMutex);
    QMutexLocker wlocker(const_cast<QMutex*>(&m_writeMutex)); // 写锁也要加，防止写入中读
    size_t sz = m_buffer.size();
    return (m_writePos >= m_readPos) ? (m_writePos - m_readPos) : (sz - m_readPos + m_writePos);
}

bool RingBuffer::isEmpty() const
{
    return readableBytes() == 0;
}

// ============================================================================
//                        Peek（不消费）
// ============================================================================

QByteArray RingBuffer::peek(size_t len) const
{
    QMutexLocker locker(&m_readMutex);
    QMutexLocker wlocker(const_cast<QMutex*>(&m_writeMutex));

    size_t bufSize = m_buffer.size();
    size_t available = (m_writePos >= m_readPos)
                           ? (m_writePos - m_readPos)
                           : (bufSize - m_readPos + m_writePos);
    if (len > available || len == 0) {
        return QByteArray(); // 数据不足或无效请求
    }

    QByteArray result(static_cast<int>(len), '\0');
    char *dst = result.data();

    // 可能需要分两段读取（回绕）
    size_t first = std::min(len, bufSize - m_readPos);
    std::memcpy(dst, m_buffer.data() + m_readPos, first);

    size_t second = len - first;
    if (second > 0) {
        std::memcpy(dst + first, m_buffer.data(), second);
    }

    return result;
}

QByteArray RingBuffer::peekUntil(char target, size_t maxSearch) const
{
    QMutexLocker locker(&m_readMutex);
    QMutexLocker wlocker(const_cast<QMutex*>(&m_writeMutex));

    size_t bufSize = m_buffer.size();
    size_t available = (m_writePos >= m_readPos)
                           ? (m_writePos - m_readPos)
                           : (bufSize - m_readPos + m_writePos);
    if (available == 0) return QByteArray();

    size_t searchLen = (maxSearch > 0 && maxSearch < available) ? maxSearch : available;

    size_t foundIdx = searchLen; // 默认未找到
    for (size_t i = 0; i < searchLen; ++i) {
        size_t pos = (m_readPos + i) % bufSize;
        if (m_buffer[pos] == target) {
            foundIdx = i;
            break;
        }
    }
    if (foundIdx >= searchLen) return QByteArray();

    size_t resultLen = foundIdx + 1;
    QByteArray result(static_cast<int>(resultLen), '\0');
    char *dst = result.data();
    size_t first = std::min(resultLen, bufSize - m_readPos);
    std::memcpy(dst, m_buffer.data() + m_readPos, first);
    size_t second = resultLen - first;
    if (second > 0) {
        std::memcpy(dst + first, m_buffer.data(), second);
    }
    return result;
}

// ==================== Consume（消费） ====================

void RingBuffer::consume(size_t len)
{
    QMutexLocker locker(&m_readMutex);
    QMutexLocker wlocker(&m_writeMutex);

    size_t bufSize = m_buffer.size();
    size_t available = (m_writePos >= m_readPos)
                           ? (m_writePos - m_readPos)
                           : (bufSize - m_readPos + m_writePos);
    if (len > available) len = available;

    m_readPos = (m_readPos + len) % bufSize;

    if (m_readPos == m_writePos) {
        m_readPos = 0;
        m_writePos = 0;
    }
}

QByteArray RingBuffer::read(size_t len)
{
    QByteArray data = peek(len);
    consume(data.size());
    return data;
}

// ==================== 管理 ====================

void RingBuffer::clear()
{
    QMutexLocker rlock(&m_readMutex);
    QMutexLocker wlock(&m_writeMutex);
    m_readPos = 0;
    m_writePos = 0;
}

size_t RingBuffer::capacity() const
{
    return m_buffer.size();
}

// ============================================================================
//                         私有辅助方法
// ============================================================================

void RingBuffer::ensureSpace(size_t required)
{
    size_t bufSize = m_buffer.size();
    size_t currentUsed = (m_writePos >= m_readPos)
                             ? (m_writePos - m_readPos)
                             : (bufSize - m_readPos + m_writePos);
    size_t freeSpace = bufSize - currentUsed;

    if (freeSpace >= required) return; // 空间够用

    // 需要扩容：新容量至少是当前容量的2倍，且能容纳所需空间
    size_t newSize = std::max(bufSize * 2, bufSize + required);

    // 将数据整理为连续的（消除回绕）
    std::vector<char> newBuf(newSize, '\0');

    if (currentUsed > 0) {
        if (m_writePos >= m_readPos) {
            // 无回绕：直接拷贝
            std::memcpy(newBuf.data(), m_buffer.data() + m_readPos, currentUsed);
        } else {
            // 有回绕：分两段拷贝
            size_t tailLen = bufSize - m_readPos;
            std::memcpy(newBuf.data(), m_buffer.data() + m_readPos, tailLen);
            std::memcpy(newBuf.data() + tailLen, m_buffer.data(), m_writePos);
        }
    }

    m_buffer.swap(newBuf);
    m_readPos = 0;
    m_writePos = currentUsed;
}
