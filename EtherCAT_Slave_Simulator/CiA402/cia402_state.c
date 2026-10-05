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
        break;
    default:
        return false; /* 故障等状态尚未实现，不能假装处理成功。 */
    }

    /* 只检查命令有意义的位，忽略无关的模式位。
     * Shutdown 的 bit3 无关，0x0006 和 0x000E 都能识别。
     * Disable Voltage 只要求 bit7 = 0、bit1 = 0。
     * 普通命令要求 bit7 = 0，避免混入故障复位请求。 */
    bool shutdown = (controlword & 0x0087u) == CIA402_CW_SHUTDOWN;
    bool switch_on = (controlword & CIA402_CW_COMMAND_MASK) == CIA402_CW_SWITCH_ON;
    bool enable_operation = CIA402_ControlRequestsEnableOperation(controlword);
    bool disable_voltage = (controlword & 0x0082u) == 0u;
    if (!shutdown && !switch_on && !enable_operation && !disable_voltage) {
        return false; /* 例如 Quick Stop、Fault Reset，留到后续小节。 */
    }
    if (disable_voltage) {
        /* 正常四个状态都能回到禁止接通；这里只改软件状态。 */
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
        if (switch_on) {
            *state = CIA402_SWITCHED_ON;
        } else if (enable_operation) {
            /* 标准也允许“接通并使能”的组合命令（转换 3 + 4）。
             * 条件满足则完成两步；否则只接通，保持未使能。
             * 主演示仍分别发 0x0007、0x000F，便于观察。 */
            *state = enable_condition_met ? CIA402_OPERATION_ENABLED : CIA402_SWITCHED_ON;
        }
        break;
    case CIA402_SWITCHED_ON:
        if (shutdown) {
            *state = CIA402_READY_TO_SWITCH_ON;
        } else if (enable_operation && enable_condition_met) {
            *state = CIA402_OPERATION_ENABLED;
        }
        /* 请求使能但条件未满足：不赋值，仍是 SWITCHED_ON。 */
        break;
    case CIA402_OPERATION_ENABLED:
        if (shutdown) {
            *state = CIA402_READY_TO_SWITCH_ON;
        } else if (switch_on) {
            /* 已使能时再发 0x0007，含义是 Disable Operation。
             * 同一个控制字的作用要结合当前状态理解。 */
            *state = CIA402_SWITCHED_ON;
        }
        break;
    default:
        return false; /* 上面已排除，这里保留完整的分支检查。 */
    }
    return true;
}

bool CIA402_EncodeStatusword(CIA402_DriveState state, uint16_t *statusword)
{
    if (statusword == NULL) {
        return false;
    }
    /* 编码与第十课的解码对应，只覆盖本小节四个状态。
     * 状态字不是控制字的复制：命令 0x000F 成功后反馈 0x0027。 */
    switch (state) {
    case CIA402_SWITCH_ON_DISABLED: *statusword = 0x0040u; break;
    case CIA402_READY_TO_SWITCH_ON: *statusword = 0x0021u; break;
    case CIA402_SWITCHED_ON: *statusword = 0x0023u; break;
    case CIA402_OPERATION_ENABLED: *statusword = 0x0027u; break;
    default: return false;
    }
    return true;
}
