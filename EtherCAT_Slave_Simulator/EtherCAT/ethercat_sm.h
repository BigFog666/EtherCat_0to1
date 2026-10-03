#ifndef ETHERCAT_SM_H
#define ETHERCAT_SM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* 第九课只管理固定的 6 字节过程数据，不模拟邮箱型 SM。
 * 每个通道的数组代表自己的那段 DPRAM，而非真的 ESC 地址空间。 */
#define EC_SM_PROCESS_SIZE 6u

/* 用从站视角命名方向：Rx 是主站写、应用读；Tx 是应用写、主站读。 */
typedef enum {
    EC_SM_RXPDO,
    EC_SM_TXPDO
} EC_SM_Direction;

/* 同一段数据的两种访问来源。
 * EtherCAT 侧对应报文访问；PDI 侧对应 STM32 等本地应用访问。 */
typedef enum {
    EC_SM_ETHERCAT_SIDE,
    EC_SM_PDI_SIDE
} EC_SM_Side;

typedef enum {
    EC_SM_OK,
    EC_SM_INVALID_ARGUMENT,
    EC_SM_DISABLED,
    EC_SM_OUT_OF_RANGE,
    EC_SM_WRONG_DIRECTION,
    EC_SM_NO_DATA
} EC_SM_Result;

typedef struct {
    uint16_t physical_start; /* ESC 本地起始地址，不是对象字典索引。 */
    EC_SM_Direction direction;
    bool enabled;
    bool has_data; /* 首次完整写入前，不能把初始零值当作有效反馈。 */
    uint8_t bytes[EC_SM_PROCESS_SIZE]; /* 最近一次完整发布的 6 字节。 */
} EC_SyncManager;

/* 初始化一个教学通道。SM2/SM3 的编号由 main 中的变量名表示。
 * 配置失败不修改原通道；成功后等待第一份完整数据。 */
EC_SM_Result EC_SM_Init(EC_SyncManager *sm, uint16_t physical_start,
                        EC_SM_Direction direction);

/* 本课每次必须从起始地址完整传递 6 字节；长度/方向不符就拒绝。
 * 成功写入后可反复读取，后续完整写入可覆盖旧数据，读取不会清空。
 * 这表达过程数据的“最新完整值”，不同于邮箱的逐笔收发握手。
 * 调用者提供至少 count 字节的有效缓冲区，按顺序调用这些函数。
 * 单线程复制不是硬件三缓冲，也不提供并发线程安全。 */
EC_SM_Result EC_SM_Write(EC_SyncManager *sm, EC_SM_Side side,
                         uint16_t physical_address, const uint8_t *source,
                         size_t count);
EC_SM_Result EC_SM_Read(const EC_SyncManager *sm, EC_SM_Side side,
                        uint16_t physical_address, uint8_t *destination,
                        size_t count);
const char *EC_SM_ResultName(EC_SM_Result result);

#endif
