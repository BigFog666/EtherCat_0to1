/* 验证故障优先级、锁存和复位沿，不修改 main 的练习变量。 */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "../CiA402/cia402_fault.h"
#include "../CiA402/cia402_state.h"
#include "../EtherCAT/ethercat_pdo.h"

int main(void)
{
    const CIA402_DriveState normal[] = {
        CIA402_SWITCH_ON_DISABLED, CIA402_READY_TO_SWITCH_ON,
        CIA402_SWITCHED_ON, CIA402_OPERATION_ENABLED, CIA402_QUICK_STOP_ACTIVE
    };
    for (size_t i = 0; i < sizeof normal / sizeof normal[0]; ++i) {
        CIA402_DriveState drive = normal[i];
        bool history = false;
        assert(CIA402_ProcessFault(&drive, 0x000F, &history, false, true));
        assert(drive == normal[i]); /* 非故障状态的完成输入不能凭空造故障。 */
        assert(CIA402_ProcessFault(&drive, 0x008F, &history, true, true));
        assert(drive == CIA402_FAULT_REACTION_ACTIVE); /* 新故障优先，先报告反应。 */
        assert(history);
        assert(CIA402_ProcessFault(&drive, 0x0080, &history, false, false));
        assert(drive == CIA402_FAULT_REACTION_ACTIVE); /* 原因消失不替代处理完成。 */
        assert(CIA402_ProcessFault(&drive, 0x0080, &history, false, true));
        assert(drive == CIA402_FAULT); /* 反应期的复位请求不会排队。 */
        assert(CIA402_ProcessFault(&drive, 0x0080, &history, false, false));
        assert(drive == CIA402_FAULT); /* 持续高位不产生新沿。 */
        assert(CIA402_ProcessFault(&drive, 0x0000, &history, true, false));
        assert(CIA402_ProcessFault(&drive, 0x0080, &history, true, false));
        assert(drive == CIA402_FAULT); /* 新沿也不能清掉仍在的原因。 */
        assert(CIA402_ProcessFault(&drive, 0x0080, &history, false, false));
        assert(drive == CIA402_FAULT); /* 原因消失后也不追认失败的请求。 */
        assert(CIA402_ProcessFault(&drive, 0x000F, &history, false, false));
        assert(drive == CIA402_FAULT); /* 普通使能请求不能退出锁存故障。 */
        assert(!history);
        assert(CIA402_ProcessFault(&drive, 0x008F, &history, false, false));
        assert(drive == CIA402_SWITCH_ON_DISABLED); /* 下位为 F 的复位帧也只复位。 */
        assert(CIA402_ProcessFault(&drive, 0x008F, &history, false, false));
        assert(drive == CIA402_SWITCH_ON_DISABLED);
        assert(CIA402_ProcessFault(&drive, 0x0006, &history, false, false));
        assert(CIA402_UpdateState(&drive, 0x0006, true));
        assert(CIA402_UpdateState(&drive, 0x0007, true));
        assert(CIA402_UpdateState(&drive, 0x000F, true));
        assert(drive == CIA402_OPERATION_ENABLED);
        assert(CIA402_ProcessFault(&drive, 0x000F, &history, true, false));
        assert(drive == CIA402_FAULT_REACTION_ACTIVE); /* 重新出现故障仍能进入反应。 */
    }
    const struct {
        CIA402_DriveState state;
        uint16_t statusword;
    } reports[] = {{CIA402_FAULT_REACTION_ACTIVE, 0x000F}, {CIA402_FAULT, 0x0008}};
    for (size_t i = 0; i < sizeof reports / sizeof reports[0]; ++i) {
        uint16_t sw = 0;
        assert(CIA402_EncodeStatusword(reports[i].state, &sw));
        assert(sw == reports[i].statusword);
        assert(CIA402_DecodeStatusword(sw) == reports[i].state);
        assert(!CIA402_IsOperationEnabled(sw));
        EC_TxPDO tx = {.statusword = sw, .actual_position = 300};
        EC_TxPDO rx = {0};
        uint8_t bytes[EC_TXPDO_SIZE];
        EC_PackTxPDO(bytes, &tx);
        assert(bytes[0] == (uint8_t)sw && bytes[1] == 0);
        EC_UnpackTxPDO(&rx, bytes);
        assert(rx.statusword == sw && rx.actual_position == 300);
    }
    CIA402_DriveState drive = CIA402_OPERATION_ENABLED;
    bool history = false;
    assert(CIA402_ProcessFault(&drive, 0x0080, &history, false, false));
    assert(drive == CIA402_OPERATION_ENABLED && history); /* 正常阶段也记录历史。 */
    assert(CIA402_ProcessFault(&drive, 0x0080, &history, true, false));
    assert(CIA402_ProcessFault(&drive, 0x0080, &history, false, true));
    assert(CIA402_ProcessFault(&drive, 0x0080, &history, false, false));
    assert(drive == CIA402_FAULT); /* 故障前已有的高位不是故障后的新复位。 */

    assert(!CIA402_ProcessFault(NULL, 0x0000, &history, true, true));
    assert(history);
    assert(!CIA402_ProcessFault(&drive, 0x0000, NULL, true, true));
    assert(drive == CIA402_FAULT);
    const CIA402_DriveState unsupported[] = {CIA402_NOT_READY_TO_SWITCH_ON, CIA402_UNKNOWN};
    for (size_t i = 0; i < sizeof unsupported / sizeof unsupported[0]; ++i) {
        drive = unsupported[i];
        assert(!CIA402_ProcessFault(&drive, 0x0000, &history, true, true));
        assert(drive == unsupported[i] && history);
    }
    puts("Lesson 11 part 3 checks passed: fault priority, reaction completion, latched cause, reset edges/history, re-fault, invalid inputs, PDO reports");
    return 0;
}
