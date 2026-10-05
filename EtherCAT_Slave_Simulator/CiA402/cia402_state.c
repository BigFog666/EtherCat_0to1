#include "cia402_state.h"

#include <stddef.h> /* NULL：空指针。 */

bool CIA402_UpdateState(CIA402_DriveState *state, uint16_t controlword,
                        bool enable_condition_met)
{
    /* state 指向调用者保存“当前状态”的变量。
     * 先检查指针和状态范围，再读写 *state。 */
    if (state == NULL) {
        return false;
    }
    switch (*state) {
    case CIA402_SWITCH_ON_DISABLED:
    case CIA402_READY_TO_SWITCH_ON:
    case CIA402_SWITCHED_ON:
    case CIA402_OPERATION_ENABLED:
    case CIA402_QUICK_STOP_ACTIVE:
        break;
    default:
        return false; /* 故障由 ProcessFault 处理，此函数只处理普通状态。 */
    }

    /* 只检查命令有意义的位，忽略无关的模式位。
     * Shutdown 的 bit3 无关，0x0006 和 0x000E 都能识别。
     * Disable Voltage 只要求 bit7 = 0、bit1 = 0。
     * 普通命令要求 bit7 = 0，避免混入故障复位请求。 */
    bool shutdown = (controlword & 0x0087u) == CIA402_CW_SHUTDOWN;
    bool switch_on = (controlword & CIA402_CW_COMMAND_MASK) == CIA402_CW_SWITCH_ON;
    bool enable_operation = CIA402_ControlRequestsEnableOperation(controlword);
    bool disable_voltage = (controlword & 0x0082u) == 0u;
    /* Quick Stop：bit7=0、bit2=0、bit1=1；bit3 和 bit0 无关。
     * 因而 0x0002、0x0003、0x000B 等都能表示这一请求。 */
    bool quick_stop = (controlword & 0x0086u) == CIA402_CW_QUICK_STOP;
    if (!shutdown && !switch_on && !enable_operation && !disable_voltage && !quick_stop) {
        return false; /* Fault Reset 交给第三步的 ProcessFault，不作为普通命令。 */
    }
    if (disable_voltage) {
        /* 正常四个状态及快速停止都能回到禁止接通；这里只改软件状态。 */
        *state = CIA402_SWITCH_ON_DISABLED;
        return true;
    }

    /* 用“当前状态”选择规则。有对应转换才赋新值，否则保持原值。
     * 同一个 0x000F，在不同状态下可能得到不同结果。 */
    switch (*state) {
    case CIA402_SWITCH_ON_DISABLED:
        if (shutdown) {
            *state = CIA402_READY_TO_SWITCH_ON;
        }
        /* 直接发 0x000F 太早，不能跳过准备阶段。 */
        break;
    case CIA402_READY_TO_SWITCH_ON:
        if (quick_stop) {
            *state = CIA402_SWITCH_ON_DISABLED;
        } else if (switch_on) {
            *state = CIA402_SWITCHED_ON;
        } else if (enable_operation) {
            /* 标准也允许“接通并使能”的组合命令（转换 3 + 4）。
             * 条件满足则完成两步；否则只接通，保持未使能。
             * 主演示仍分别发 0x0007、0x000F，便于观察。 */
            *state = enable_condition_met ? CIA402_OPERATION_ENABLED : CIA402_SWITCHED_ON;
        }
        break;
    case CIA402_SWITCHED_ON:
        if (quick_stop) {
            *state = CIA402_SWITCH_ON_DISABLED;
        } else if (shutdown) {
            *state = CIA402_READY_TO_SWITCH_ON;
        } else if (enable_operation && enable_condition_met) {
            *state = CIA402_OPERATION_ENABLED;
        }
        /* 请求使能但条件未满足：不赋值，仍是 SWITCHED_ON。 */
        break;
    case CIA402_OPERATION_ENABLED:
        if (quick_stop) {
            *state = CIA402_QUICK_STOP_ACTIVE; /* 收到请求，开始停止过程。 */
        } else if (shutdown) {
            *state = CIA402_READY_TO_SWITCH_ON;
        } else if (switch_on) {
            /* 已使能时再发 0x0007，含义是 Disable Operation。
             * 同一个控制字的作用要结合当前状态理解。 */
            *state = CIA402_SWITCHED_ON;
        }
        break;
    case CIA402_QUICK_STOP_ACTIVE:
        /* 本模型选择 Option Code = 2：要等停止完成后回到禁止接通。
         * 0x000F、0x0006、0x0007 或重复 Quick Stop 都保持当前状态。
         * 本地完成事件由另一个函数处理，不因收到使能请求而打断停止。 */
        break;
    default:
        return false; /* 上面已排除，这里保留完整的分支检查。 */
    }
    return true;
}

bool CIA402_CompleteQuickStop(CIA402_DriveState *state, bool stop_completed)
{
    if (state == NULL || *state != CIA402_QUICK_STOP_ACTIVE) {
        return false;
    }
    /* 两件事分开：请求已经收到，不意味着停止已经完成。
     * 实际设备应由运动/驱动逻辑给出完成事件；本课手动提供 bool。 */
    if (stop_completed) {
        *state = CIA402_SWITCH_ON_DISABLED;
    }
    return true;
}

bool CIA402_EncodeStatusword(CIA402_DriveState state, uint16_t *statusword)
{
    if (statusword == NULL) {
        return false;
    }
    /* 编码与第十课的解码对应，第二步加快速停止，第三步加两个故障状态。
     * 状态字不是控制字的复制：命令 0x000F 成功后反馈 0x0027。 */
    switch (state) {
    case CIA402_SWITCH_ON_DISABLED: *statusword = 0x0040u; break;
    case CIA402_READY_TO_SWITCH_ON: *statusword = 0x0021u; break;
    case CIA402_SWITCHED_ON: *statusword = 0x0023u; break;
    case CIA402_OPERATION_ENABLED: *statusword = 0x0027u; break;
    case CIA402_QUICK_STOP_ACTIVE: *statusword = 0x0007u; break;
    case CIA402_FAULT_REACTION_ACTIVE: *statusword = 0x000Fu; break;
    case CIA402_FAULT: *statusword = 0x0008u; break;
    default: return false;
    }
    return true;
}
