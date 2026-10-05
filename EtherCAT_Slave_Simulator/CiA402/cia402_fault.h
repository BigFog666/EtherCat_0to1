#ifndef CIA402_FAULT_H
#define CIA402_FAULT_H

#include "cia402.h"

/* 第十一课第三步：每次收到命令后，先处理故障和复位，再考虑普通命令。
 * fault_active：故障原因现在是否仍存在；不是状态字的故障标志。
 * reaction_completed：故障处理动作是否完成；不等于故障原因消失。
 * previous_reset_bit 指向本驱动上一次命令的 bit7，初始值为 false。
 * 必须每步调用，包括正常状态，才能正确记住 bit7 的变化。
 * 已有五个状态以及 FAULT_REACTION_ACTIVE、FAULT 为支持范围。
 * true 表示本次输入已处理，状态可能保持；false 保留状态及历史。
 * 假定 state 和 previous_reset_bit 指向不同的有效变量。
 * 本课模拟本地事件，不执行真实硬件保护或清除实际故障。 */
bool CIA402_ProcessFault(CIA402_DriveState *state, uint16_t controlword,
                         bool *previous_reset_bit, bool fault_active,
                         bool reaction_completed);

#endif
