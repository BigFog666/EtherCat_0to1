#include "cia402_fault.h"

#include <stddef.h>

bool CIA402_ProcessFault(CIA402_DriveState *state, uint16_t controlword,
                         bool *previous_reset_bit, bool fault_active,
                         bool reaction_completed)
{
    if (state == NULL || previous_reset_bit == NULL) {
        return false;
    }
    switch (*state) {
    case CIA402_SWITCH_ON_DISABLED:
    case CIA402_READY_TO_SWITCH_ON:
    case CIA402_SWITCHED_ON:
    case CIA402_OPERATION_ENABLED:
    case CIA402_QUICK_STOP_ACTIVE:
    case CIA402_FAULT_REACTION_ACTIVE:
    case CIA402_FAULT:
        break;
    default:
        return false; /* 初始化阶段和未知状态还不在本课处理范围。 */
    }

    /* 当前为 1、上一次为 0，才算一次新的复位请求。
     * ! 表示逻辑取反；一直保持 1 不会反复产生上升沿。
     * 无论本轮是否能复位，都保存历史；失败的请求不排队等以后执行。 */
    bool reset_bit = (controlword & CIA402_CW_FAULT_RESET) != 0u;
    bool reset_rising = reset_bit && !*previous_reset_bit;
    *previous_reset_bit = reset_bit;

    if (*state == CIA402_FAULT) {
        /* 故障已经锁存：原因消失也不会自动退出。
         * 必须同时满足“新的复位上升沿”和“原因不再存在”。 */
        if (reset_rising && !fault_active) {
            *state = CIA402_SWITCH_ON_DISABLED;
        }
    } else if (*state == CIA402_FAULT_REACTION_ACTIVE) {
        /* 先完成故障处理，才能进入 Fault；这个阶段不接受提前复位。
         * 完成处理和消除原因是两个条件：原因仍在也可以完成处理。 */
        if (reaction_completed) {
            *state = CIA402_FAULT;
        }
    } else if (fault_active) {
        /* 本模型新发现故障时，本轮先进入故障反应状态。
         * 下一轮才判断处理完成，保留一份可观察的故障反应报告。 */
        *state = CIA402_FAULT_REACTION_ACTIVE;
    }
    return true;
}
