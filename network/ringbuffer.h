#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <QByteArray>
#include <QMutex>
#include <vector>

/**
 * @brief 线程安全的环形字节缓冲区
 *
 * 用途：TCP 接收到的原始数据先写入环形缓冲，
 *       上层按需取出完整帧进行处理。
 * 特点：
 *   - 线程安全（读写各一把锁）
 *   - 自动扩容（写入时空间不足则翻倍）
 *   - 支持连续读取指定长度（内部自动处理回绕）
 */
class RingBuffer
{
public:
    /**
     * @brief 构造函数
     * @param capacity 初始容量（字节），默认 64KB
     */
    explicit RingBuffer(size_t capacity = 65536);

    ~RingBuffer() = default;

    // ========== 禁止拷贝/移动 ==========
    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    // ==================== 写入操作 ====================

    /**
     * @brief 写入原始字节数据（线程安全）
     * @param data 要写入的数据指针
     * @param len  数据长度
     * @return 实际写入的字节数（始终等于 len，除非内存不足抛异常）
     */
    size_t write(const char *data, size_t len);

    /** 便捷重载 */
    size_t write(const QByteArray &data);

    // ==================== 读取操作（不消费） ====================

    /**
     * @brief 查看缓冲区中当前可读数据的总字节数
     */
    size_t readableBytes() const;

    /**
     * @brief 缓冲区是否为空
     */
    bool isEmpty() const;

    /**
     * @brief 连续查看前 N 字节数据（不移动读指针，不消费数据）
     *
     * 内部会处理回绕情况，保证返回的是连续的内存块。
     * 用于上层做帧头检测、长度校验等"预览"操作。
     *
     * @param len  要查看的字节数
     * @return 连续的数据副本；若可读不足 len 则返回空
     */
    QByteArray peek(size_t len) const;

    /**
     * @brief 从当前位置连续查看直到找到目标字节（不消费）
     * @param target 目标字节
     * @param maxSearch 最大搜索长度（防止无限搜）
     * @return 找到返回从当前位置到目标的(含target)数据；未找到返回空
     */
    QByteArray peekUntil(char target, size_t maxSearch = 0) const;

    // ==================== 读取操作（消费） ====================

    /**
     * @brief 消费（丢弃）前 N 字节数据（移动读指针）
     * @param len 要丢弃的字节数（不能超过可读数量）
     */
    void consume(size_t len);

    /**
     * @brief 读出并消费前 N 字节（peek + consume 的组合）
     * @param len 要读出的字节数
     * @return 读出的数据
     */
    QByteArray read(size_t len);

    // ==================== 管理 ====================

    /**
     * @brief 清空缓冲区，重置读写指针
     */
    void clear();

    /**
     * @brief 获取当前已分配的缓冲区容量
     */
    size_t capacity() const;

private:
    void ensureSpace(size_t required); // 确保有足够写入空间

    std::vector<char> m_buffer;        // 底层存储
    size_t m_readPos = 0;              // 读指针
    size_t m_writePos = 0;             // 写指针
    mutable QMutex m_readMutex;        // 读取锁
    QMutex m_writeMutex;               // 写入锁
};

#endif // RINGBUFFER_H
