#ifndef ETHERCAT_STATE_H
#define ETHERCAT_STATE_H

#include <stdbool.h>

/* 第五课中的四个通信状态，数值对应 EtherCAT AL 状态编码。
 * 状态错误标志和错误码暂时不在本课实现。 */
typedef enum {
    EC_STATE_INIT = 0x01,
    EC_STATE_PREOP = 0x02,
    EC_STATE_SAFEOP = 0x04,
    EC_STATE_OP = 0x08
} EtherCAT_State;

/* 将状态值转换为便于打印的名称。 */
const char *EC_StateName(EtherCAT_State state);
/* 请求切换状态。current 是指针，函数通过它修改调用者的状态变量。
 * 当前只演示：保持原状态、逐级向上切换、回到 INIT。
 * 真实从站还需要检查邮箱、PDO、时钟同步配置及 AL 错误等条件。
 * 返回 true 表示接受请求，false 表示拒绝，拒绝时保持原状态。 */
bool EC_RequestState(EtherCAT_State *current, EtherCAT_State requested);

#endif
