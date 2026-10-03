#include "cia402.h"

bool CIA402_ControlRequestsEnableOperation(uint16_t controlword)
{
    /* & 是按位与：掩码为 1 的位留下，为 0 的位忽略。
     * 比如 0x001F 带有额外的模式位，但基本请求仍是使能。
     * 返回 true 只是“请求了”，不替代状态反馈。 */
    return (controlword & CIA402_CW_COMMAND_MASK) == CIA402_CW_ENABLE_OPERATION;
}

CIA402_DriveState CIA402_DecodeStatusword(uint16_t statusword)
{
    /* 先识别允许忽略 bit5 的四种状态。
     * 0x004F = 0100 1111，保留 bit6 和 bit3～0。
     * 忽略电压标志 bit4、警告 bit7 和其他不决定状态的位。 */
    switch (statusword & 0x004Fu) {
    case 0x0000u: return CIA402_NOT_READY_TO_SWITCH_ON;
    case 0x0040u: return CIA402_SWITCH_ON_DISABLED;
    case 0x000Fu: return CIA402_FAULT_REACTION_ACTIVE;
    case 0x0008u: return CIA402_FAULT;
    default: break;
    }

    /* 以下四种状态需要用 bit5 区分正常运行和 Quick Stop Active。
     * 0x006F = 0110 1111，比前一个掩码多保留 bit5。
     * 例如 0x0027 / 0x0037 / 0x00A7 都能解码为 Operation Enabled。 */
    switch (statusword & 0x006Fu) {
    case 0x0021u: return CIA402_READY_TO_SWITCH_ON;
    case 0x0023u: return CIA402_SWITCHED_ON;
    case 0x0027u: return CIA402_OPERATION_ENABLED;
    case 0x0007u: return CIA402_QUICK_STOP_ACTIVE;
    default: return CIA402_UNKNOWN;
    }
}

const char *CIA402_StateName(CIA402_DriveState state)
{
    switch (state) {
    case CIA402_NOT_READY_TO_SWITCH_ON: return "NOT_READY_TO_SWITCH_ON";
    case CIA402_SWITCH_ON_DISABLED: return "SWITCH_ON_DISABLED";
    case CIA402_READY_TO_SWITCH_ON: return "READY_TO_SWITCH_ON";
    case CIA402_SWITCHED_ON: return "SWITCHED_ON";
    case CIA402_OPERATION_ENABLED: return "OPERATION_ENABLED";
    case CIA402_QUICK_STOP_ACTIVE: return "QUICK_STOP_ACTIVE";
    case CIA402_FAULT_REACTION_ACTIVE: return "FAULT_REACTION_ACTIVE";
    case CIA402_FAULT: return "FAULT";
    default: return "UNKNOWN";
    }
}

bool CIA402_IsOperationEnabled(uint16_t statusword)
{
    /* 不能只看 bit2：Quick Stop Active 等状态中也可能有这个位。
     * 必须用完整的状态编码，避免把不同状态误认为已使能。 */
    return CIA402_DecodeStatusword(statusword) == CIA402_OPERATION_ENABLED;
}
