#ifndef CIA402_STATE_H
#define CIA402_STATE_H

#include "cia402.h"

/* 第十一课第一步：只实现正常使能路径上的四个状态。
 * 假定内部初始化已完成，初始状态为 SWITCH_ON_DISABLED。
 * 故障、快速停止、电机控制及通信失联保护留到后续。 */

/* 根据当前状态、控制字、进入运行条件，更新 *state。
 * enable_condition_met 只模拟“允许进入 OPERATION_ENABLED”的条件，
 * 不是运行中的安全监督信号；已经使能后的保护逻辑留到后续。
 * true：本小节支持这次输入，不代表状态一定发生改变。
 * false：空指针、未实现的状态或命令，原状态保持不变。 */
bool CIA402_UpdateState(CIA402_DriveState *state, uint16_t controlword,
                        bool enable_condition_met);

/* 从内部状态生成最小状态编码，写入 *statusword。
 * 与第十课 DecodeStatusword 的方向相反：状态 -> 数字。
 * 不生成电压、警告、远程等附加标志，不代表真实硬件已上电。
 * 未实现的状态或空指针返回 false，原输出保持不变。 */
bool CIA402_EncodeStatusword(CIA402_DriveState state, uint16_t *statusword);

#endif
