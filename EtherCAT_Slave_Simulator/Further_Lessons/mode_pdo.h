#ifndef MODE_PDO_H
#define MODE_PDO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../EtherCAT/ethercat_pdo.h"

/* 后续课程使用独立的 13 字节布局，旧课程的 6 字节布局继续保留。
 * 前六字节复用旧编解码器：字 0～1，位置 2～5。
 * 模式 6，速度 7～10，扭矩 11～12。这里不配置真实 PDO Mapping。 */
#define MODE_PDO_SIZE 13u
typedef struct {
    EC_RxPDO position;
    int8_t mode;              /* 0x6060：请求模式。 */
    int32_t target_velocity;  /* 0x60FF：目标速度。 */
    int16_t target_torque;    /* 0x6071：目标扭矩。 */
} ModeCommand;
typedef struct {
    EC_TxPDO position;
    int8_t mode_display;      /* 0x6061：已经接受的模式。 */
    int32_t actual_velocity;  /* 0x606C：实际速度。 */
    int16_t actual_torque;    /* 0x6077：实际扭矩。 */
} ModeFeedback;

/* 检查非空和精确长度；失败时保留输出。数组不是结构体内存复制。 */
bool ModePDO_PackRx(uint8_t *bytes, size_t size, const ModeCommand *command);
bool ModePDO_UnpackRx(ModeCommand *command, const uint8_t *bytes, size_t size);
bool ModePDO_PackTx(uint8_t *bytes, size_t size, const ModeFeedback *feedback);
bool ModePDO_UnpackTx(ModeFeedback *feedback, const uint8_t *bytes, size_t size);

#endif
