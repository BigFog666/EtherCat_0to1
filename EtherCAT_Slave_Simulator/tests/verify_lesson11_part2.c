/* 检查停止请求、完成事件和重新使能的区别，保留 main 的学习者练习。 */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "../CiA402/cia402_state.h"
#include "../EtherCAT/ethercat_pdo.h"

int main(void)
{
    const uint16_t requests[] = {0x0002, 0x0003, 0x000A, 0x000B, 0x0012};
    for (size_t i = 0; i < sizeof requests / sizeof requests[0]; ++i) {
        CIA402_DriveState drive = CIA402_OPERATION_ENABLED;
        assert(CIA402_UpdateState(&drive, requests[i], false));
        assert(drive == CIA402_QUICK_STOP_ACTIVE); /* 不受进入使能条件干扰。 */
        assert(CIA402_CompleteQuickStop(&drive, false));
        assert(drive == CIA402_QUICK_STOP_ACTIVE);
        assert(CIA402_UpdateState(&drive, 0x000F, true));
        assert(drive == CIA402_QUICK_STOP_ACTIVE); /* 本选项不允许直接恢复。 */
        assert(CIA402_UpdateState(&drive, 0x0006, true));
        assert(CIA402_UpdateState(&drive, 0x0007, true));
        assert(drive == CIA402_QUICK_STOP_ACTIVE);
        assert(CIA402_UpdateState(&drive, requests[i], true));
        assert(drive == CIA402_QUICK_STOP_ACTIVE); /* 重复请求不表示停止完成。 */

        uint16_t sw = 0;
        assert(CIA402_EncodeStatusword(drive, &sw));
        assert(sw == 0x0007);
        assert(CIA402_DecodeStatusword(sw) == CIA402_QUICK_STOP_ACTIVE);
        assert(!CIA402_IsOperationEnabled(sw)); /* bit2=1 也不是已使能状态。 */
        EC_TxPDO tx = {.statusword = sw, .actual_position = 300};
        EC_TxPDO master = {0};
        uint8_t bytes[EC_TXPDO_SIZE];
        EC_PackTxPDO(bytes, &tx);
        assert(bytes[0] == 0x07 && bytes[1] == 0x00);
        EC_UnpackTxPDO(&master, bytes);
        assert(master.statusword == sw && master.actual_position == 300);

        assert(CIA402_CompleteQuickStop(&drive, true));
        assert(drive == CIA402_SWITCH_ON_DISABLED);
        assert(CIA402_UpdateState(&drive, 0x000F, true));
        assert(drive == CIA402_SWITCH_ON_DISABLED); /* 完成事件也不自动重新使能。 */
        assert(CIA402_UpdateState(&drive, 0x0006, true));
        assert(CIA402_UpdateState(&drive, 0x0007, true));
        assert(CIA402_UpdateState(&drive, 0x000F, true));
        assert(drive == CIA402_OPERATION_ENABLED);
    }
    const CIA402_DriveState not_running[] = {
        CIA402_SWITCH_ON_DISABLED, CIA402_READY_TO_SWITCH_ON, CIA402_SWITCHED_ON
    };
    for (size_t i = 0; i < sizeof not_running / sizeof not_running[0]; ++i) {
        CIA402_DriveState drive = not_running[i];
        assert(CIA402_UpdateState(&drive, 0x0002, true));
        assert(drive == CIA402_SWITCH_ON_DISABLED);
    }
    CIA402_DriveState drive = CIA402_QUICK_STOP_ACTIVE;
    assert(CIA402_UpdateState(&drive, 0x0000, true));
    assert(drive == CIA402_SWITCH_ON_DISABLED); /* Disable Voltage 转换仍可用。 */
    const CIA402_DriveState other_states[] = {
        CIA402_NOT_READY_TO_SWITCH_ON, CIA402_SWITCH_ON_DISABLED,
        CIA402_READY_TO_SWITCH_ON, CIA402_SWITCHED_ON, CIA402_OPERATION_ENABLED,
        CIA402_FAULT_REACTION_ACTIVE, CIA402_FAULT, CIA402_UNKNOWN
    };
    for (size_t i = 0; i < sizeof other_states / sizeof other_states[0]; ++i) {
        drive = other_states[i];
        assert(!CIA402_CompleteQuickStop(&drive, true));
        assert(drive == other_states[i]);
    }
    drive = CIA402_QUICK_STOP_ACTIVE;
    assert(!CIA402_UpdateState(&drive, 0x0082, true)); /* bit7=1 是故障复位类输入。 */
    assert(drive == CIA402_QUICK_STOP_ACTIVE);
    assert(!CIA402_CompleteQuickStop(NULL, true));
    puts("Lesson 11 part 2 checks passed: Quick Stop masks, pending/completed, option 2 restart, non-running states, rejected events, PDO feedback");
    return 0;
}
