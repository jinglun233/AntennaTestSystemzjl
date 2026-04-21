"""
生成 1029B 回复数据帧 (EB90 04 C1 + 1024Bytes Payload + Sum8)
用于网络调试助手 / TCP Client 工具测试 AntennaTestSystem 服务端解析

协议结构:
  Offset   Size    Field
  0x00     2B      帧头: EB 90
  0x02     1B      帧类型: 04
  0x03     1B      指令类型: C1 (遥测数据)
  0x04     1024B   数据域
  0x404    1B      校验和 (数据域累加和低8位)
  ─────────────────────────────
  Total: 1029 Bytes

数据域布局 (参考 temperatureinfowindow 解析):
  0x04~0x20B (508B): 保留/前段数据 (填零)
  0x20C (512):       热控状态标志字节
  0x20D~0x20E (2B):  保留
  0x20F (515):       112路测温数据起始 (每路1Byte, 10~245)
  0x273 (627):       50路开关状态 (bit-packed, 7Bytes)
  0x27A~0x403:       填充 (0xAA)
"""

import struct
import math
import sys

# ======================== 构建数据域 (1024 Bytes) ========================
payload = bytearray(1024)

# --- 0x000~0x1FB: 前段保留区 (508 Bytes) ---
# 保持全零

# --- 0x200 (offset 512): 热控状态标志 ---
payload[0x200] = 0x55

# --- 0x201~0x202: 保留 2 字节 ---
# 保持全零

# --- 0x203 (offset 515): 112 路测温通道数据 ---
# 模拟正弦波形 + 温度梯度, 范围 10~245°C (无符号8位)
for i in range(112):
    base_temp = 80 + 60 * math.sin(i * 0.12) + 30 * (i / 112.0)
    temp = int(base_temp + (-1)**i * 5)  # 加点抖动
    temp = max(10, min(245, temp))
    payload[0x203 + i] = temp & 0xFF

# --- 0x273 (offset 627): 50 路开关状态 (bit-packed, 7 Bytes) ---
# 前20路闭合, 35~48偶数路闭合, 其余断开
for sw in range(50):
    if sw < 20 or (sw >= 35 and sw % 2 == 0):
        byte_idx = sw // 8
        bit_idx = sw % 8
        payload[0x273 + byte_idx] |= (1 << bit_idx)

# --- 0x27A~0x403: 剩余填充 (0xAA pattern) ---
for i in range(0x273 + 7, 1024):
    payload[i] = 0xAA

# ======================== 计算校验和 (仅数据域) ========================
checksum = sum(payload) & 0xFF

# ======================== 组装完整帧 ========================
frame = bytearray()
frame += b'\xEB\x90'          # 帧头 (2B)
frame += b'\x04'              # 帧类型 (1B)
frame += b'\xC1'              # 指令类型=遥测数据 (1B)
frame += bytes(payload)        # 数据域 (1024B)
frame.append(checksum)         # 校验和 (1B)

assert len(frame) == 1029, f"帧长度错误: {len(frame)}, 期望 1029"

# ======================== 输出 ========================
print(f"=" * 60)
print(f"  1029B 测试数据帧 (回复遥测)")
print(f"=" * 60)
print(f"总长度: {len(frame)} Bytes")
print(f"校验和 (Sum8 of Payload): 0x{checksum:02X} ({checksum})")
print()
print("--- 关键字段验证 ---")
print(f"  [0-1]   帧头:     0x{frame[0]:02X} 0x{frame[1]:02X}")
print(f"  [2]     帧类型:   0x{frame[2]:02X}")
print(f"  [3]     指令类型: 0x{frame[3]:02X} ({'遥测数据' if frame[3]==0xC1 else '未知'})")
print(f"  [512]   热控标志: 0x{frame[0x204]:02X}")  # 注意: frame中偏移+4
print(f"  [517]   温度[0]:  {frame[0x207]}°C")
print(f"  [629]   温度[111]:{frame[0x277]}°C")
print(f"  [631]   开关字节: 0x{frame[0x277]:02X}")
print(f"  [1028]  校验和:   0x{frame[-1]:02X}")
print()

# 格式1: 连续 Hex 字符串 (无空格, 适用于支持纯hex输入的调试工具)
hex_continuous = ''.join(f'{b:02X}' for b in frame)
print("--- 格式1: 纯Hex字符串 (2058字符) ---")
print(hex_continuous)
print()

# 格式2: 带空格的 Hex (易读, 部分调试助手支持)
hex_spaced = ' '.join(f'{b:02X}' for b in frame)
print("--- 格式2: 带空格Hex (可读) ---")
print(hex_spaced)
print()

# 格式3: C语言字节数组 (Qt/C++ 可直接用)
print("--- 格式3: C/Qt 字节数组 ---")
print("// 1029B test telemetry frame (EB90 04 C1)")
lines = []
for i in range(0, len(frame), 16):
    chunk = frame[i:i+16]
    hex_vals = ', '.join(f'0x{b:02X}' for b in chunk)
    lines.append(f'    {hex_vals}')
c_array = 'const unsigned char TEST_FRAME_1029[1029] = {\n' + ',\n'.join(lines) + '\n};'
print(c_array)
print()

# 写入二进制文件 (可用 TCP 工具发送文件)
bin_path = __file__.replace('.py', '_1029.bin') if __file__.endswith('.py') else 'test_frame_1029.bin'
with open(bin_path, 'wb') as f:
    f.write(frame)
print(f"--- 二进制文件已保存: {bin_path} ---")

# 也写入 hex 文本文件 (方便复制)
txt_path = bin_path.replace('.bin', '.hex.txt')
with open(txt_path, 'w', encoding='utf-8') as f:
    f.write(hex_continuous)
print(f"--- Hex文本已保存: {txt_path} ---")
