#ifndef ETHERCAT_PDO_H
#define ETHERCAT_PDO_H

#include <stdint.h>

/* 第六课使用固定布局：2 字节控制字/状态字 + 4 字节位置 = 6 字节。
 * 这里的长度指打包后的 PDO 数据长度，不是结构体的 sizeof。
 * 末尾 u 表示无符号整数常量。 */
#define EC_RXPDO_SIZE 6u
#define EC_TXPDO_SIZE 6u

/* Rx 表示从站接收：主站发给关节的命令。 */
typedef struct {
    uint16_t controlword;     /* 控制字：0x6040:00，16 位。 */
    int32_t target_position;  /* 目标位置：0x607A:00，32 位，可为负数。 */
} EC_RxPDO;

/* Tx 表示从站发送：关节返回给主站的反馈。 */
typedef struct {
    uint16_t statusword;      /* 状态字：0x6041:00，16 位。 */
    int32_t actual_position;  /* 实际位置：0x6064:00，32 位，可为负数。 */
} EC_TxPDO;

/* 主站的过程映像：在内存中存放周期命令和反馈。
 * 本课用两个数组演示；完整以太网帧和 ESC 内存暂未模拟。 */
typedef struct {
    uint8_t outputs[EC_RXPDO_SIZE]; /* 主站输出 -> 从站 RxPDO。 */
    uint8_t inputs[EC_TXPDO_SIZE];   /* 主站输入 <- 从站 TxPDO。 */
} EC_ProcessImage;

/* Pack：把结构体中的数值拆成字节；Unpack：把字节还原成数值。
 * 结构体便于应用代码使用，字节数组用于表示约定的通信布局。
 * 调用者必须提供非空指针，以及至少 6 字节的缓冲区。
 * 注意：函数形参中的 bytes[6] 不会自动检查实际缓冲区长度。 */
void EC_PackRxPDO(uint8_t bytes[EC_RXPDO_SIZE], const EC_RxPDO *pdo);
void EC_UnpackRxPDO(EC_RxPDO *pdo, const uint8_t bytes[EC_RXPDO_SIZE]);
void EC_PackTxPDO(uint8_t bytes[EC_TXPDO_SIZE], const EC_TxPDO *pdo);
void EC_UnpackTxPDO(EC_TxPDO *pdo, const uint8_t bytes[EC_TXPDO_SIZE]);

#endif
