#ifndef CIA402_H
#define CIA402_H

#include <stdbool.h>
#include <stdint.h>

/* 第十课只解释控制字和状态字，不根据命令自动推进驱动状态。
 * 0x6040:00 为 16 位控制字，0x6041:00 为 16 位状态字。
 * 这两个字段早已在固定 PDO 中，本课开始解释其含义。
 * 原对象字典目前只支持 INTEGER32 位置，不因此新增 16 位 SDO 接口。 */
#define CIA402_CW_SHUTDOWN         0x0006u
#define CIA402_CW_SWITCH_ON        0x0007u
#define CIA402_CW_ENABLE_OPERATION 0x000Fu

/* 控制字 bit7、bit3～0 是本课关注的命令位。
 * 其他位可能用于模式命令，例如 bit4，不应要求整个字等于 0x000F。
 * bit7 故障复位置位时，不按普通请求使能命令解释。 */
#define CIA402_CW_COMMAND_MASK 0x008Fu
#define CIA402_SW_WARNING      0x0080u /* 状态字 bit7：有警告，不等同于 Fault。 */

typedef enum {
    CIA402_NOT_READY_TO_SWITCH_ON,
    CIA402_SWITCH_ON_DISABLED,
    CIA402_READY_TO_SWITCH_ON,
    CIA402_SWITCHED_ON,
    CIA402_OPERATION_ENABLED,
    CIA402_QUICK_STOP_ACTIVE,
    CIA402_FAULT_REACTION_ACTIVE,
    CIA402_FAULT,
    CIA402_UNKNOWN /* 位组合不符合已知状态，不能猜作已使能。 */
} CIA402_DriveState;

/* 只判断收到的控制字是否包含请求使能命令，不证明请求已经执行。 */
bool CIA402_ControlRequestsEnableOperation(uint16_t controlword);

/* 解码已经收到的状态字，不改变状态字，不产生状态转换。
 * 按标准状态编码分别使用 0x004F / 0x006F 掩码。
 * 同一驱动状态可以同时带有电压、警告和其他标志。 */
CIA402_DriveState CIA402_DecodeStatusword(uint16_t statusword);
const char *CIA402_StateName(CIA402_DriveState state);
bool CIA402_IsOperationEnabled(uint16_t statusword);

#endif
