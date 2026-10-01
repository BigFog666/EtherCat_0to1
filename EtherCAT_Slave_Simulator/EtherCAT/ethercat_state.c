#include "ethercat_state.h"

const char *EC_StateName(EtherCAT_State state)
{
    /* 仅用于日志输出，对状态本身不做修改。 */
    switch (state) {
    case EC_STATE_INIT: return "INIT";
    case EC_STATE_PREOP: return "PRE-OP";
    case EC_STATE_SAFEOP: return "SAFE-OP";
    case EC_STATE_OP: return "OP";
    default: return "UNKNOWN";
    }
}

bool EC_RequestState(EtherCAT_State *current, EtherCAT_State requested)
{
    /* 防止解引用空指针。这里的 0 表示空指针，不是 INIT 状态。 */
    if (current == 0) {
        return false;
    }
    /* 首先确认当前状态值有效；无效的状态值不接受切换。 */
    switch (*current) {
    case EC_STATE_INIT:
    case EC_STATE_PREOP:
    case EC_STATE_SAFEOP:
    case EC_STATE_OP:
        break;
    default:
        return false;
    }
    /* 本课支持的切换路径，|| 表示满足任意一个条件即可。
     * INIT -> OP 不满足这些条件，所以会被拒绝。
     * 完整协议中的其他转换路径以后再补充。 */
    if (requested == *current || requested == EC_STATE_INIT ||
        (*current == EC_STATE_INIT && requested == EC_STATE_PREOP) ||
        (*current == EC_STATE_PREOP && requested == EC_STATE_SAFEOP) ||
        (*current == EC_STATE_SAFEOP && requested == EC_STATE_OP)) {
        *current = requested; /* 检查通过，才真正更新调用者的状态。 */
        return true;
    }
    return false;
}
