#include "ethercat_pdo.h"

/* 阅读顺序：先看文件下半部分的四个 Pack/Unpack 函数，再看这些辅助函数。
 * 协议布局要明确指定每个字节，不能直接把整个结构体复制出去：
 * 编译器可能插入填充字节；不同平台的字节序也可能不同。
 * 本课统一采用小端顺序，也就是低字节放在前面。 */

/* 将一个 16 位无符号数拆成两个字节。
 * 例如 0x1234 -> bytes[0] = 0x34，bytes[1] = 0x12。 */
static void WriteU16(uint8_t *bytes, uint16_t value)
{
    bytes[0] = (uint8_t)value;        /* 转成 uint8_t，只保留最低 8 位。 */
    bytes[1] = (uint8_t)(value >> 8); /* 右移 8 位，再取原来的高字节。 */
}

/* 将两个字节拼回 16 位数：高字节左移 8 位，再与低字节合并。 */
static uint16_t ReadU16(const uint8_t *bytes)
{
    return (uint16_t)((uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8));
}

/* 将位置拆成四个字节。
 * 例如 2000 = 0x000007D0 -> D0 07 00 00。
 * 先转换到 uint32_t，再移位，让负位置的编码也有明确定义。 */
static void WriteI32(uint8_t *bytes, int32_t value)
{
    uint32_t raw = (uint32_t)value;
    bytes[0] = (uint8_t)raw;
    bytes[1] = (uint8_t)(raw >> 8);
    bytes[2] = (uint8_t)(raw >> 16);
    bytes[3] = (uint8_t)(raw >> 24);
}

/* 将四个字节拼回位置。
 * 先提升为 uint32_t 再移位，避免用过窄的整数类型计算。 */
static int32_t ReadI32(const uint8_t *bytes)
{
    uint32_t raw = (uint32_t)bytes[0] |
                   ((uint32_t)bytes[1] << 8) |
                   ((uint32_t)bytes[2] << 16) |
                   ((uint32_t)bytes[3] << 24);
    /* 这一段是兼容负位置的处理，本课可以先跳过推导。
     * 最高位为 0 时是非负数，可以直接转成 int32_t。
     * 最高位为 1 时按 32 位补码还原负数，避免越界的强制转换。
     * 例如 FF FF FF FF -> -1，00 00 00 80 -> INT32_MIN。 */
    if (raw <= (uint32_t)INT32_MAX) {
        return (int32_t)raw;
    }
    return INT32_MIN + (int32_t)(raw - UINT32_C(0x80000000));
}

void EC_PackRxPDO(uint8_t bytes[EC_RXPDO_SIZE], const EC_RxPDO *pdo)
{
    /* 固定映射：先控制字，再目标位置；主站和从站必须一致。 */
    WriteU16(bytes, pdo->controlword);       /* 从偏移 0 开始，写 2 字节。 */
    WriteI32(bytes + 2, pdo->target_position); /* 从偏移 2 开始，写 4 字节。 */
}

void EC_UnpackRxPDO(EC_RxPDO *pdo, const uint8_t bytes[EC_RXPDO_SIZE])
{
    /* 与打包的偏移完全对应：bytes + 2 指向数组第 3 个字节。 */
    pdo->controlword = ReadU16(bytes);
    pdo->target_position = ReadI32(bytes + 2);
}

void EC_PackTxPDO(uint8_t bytes[EC_TXPDO_SIZE], const EC_TxPDO *pdo)
{
    /* 反馈布局：偏移 0 放状态字，偏移 2 放实际位置。 */
    WriteU16(bytes, pdo->statusword);
    WriteI32(bytes + 2, pdo->actual_position);
}

void EC_UnpackTxPDO(EC_TxPDO *pdo, const uint8_t bytes[EC_TXPDO_SIZE])
{
    /* 主站按相同布局解读反馈，不把实际位置当成目标位置。 */
    pdo->statusword = ReadU16(bytes);
    pdo->actual_position = ReadI32(bytes + 2);
}
