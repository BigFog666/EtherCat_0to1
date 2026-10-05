/* 可先跳过测试实现：它独立检查转换规则，不改 main 中的练习。
 * 包括太早请求、进入条件、重复命令、退回分支、掩码和失败保留。 */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "../CiA402/cia402_state.h"
#include "../EtherCAT/ethercat_pdo.h"

int main(void)
{
    CIA402_DriveState drive = CIA402_SWITCH_ON_DISABLED;
    assert(CIA402_UpdateState(&drive, 0x000F, true));
    assert(drive == CIA402_SWITCH_ON_DISABLED); /* 输入受支持但请求太早。 */
    assert(CIA402_UpdateState(&drive, 0x0006, true));
    assert(drive == CIA402_READY_TO_SWITCH_ON);
    assert(CIA402_UpdateState(&drive, 0x0006, true));
    assert(drive == CIA402_READY_TO_SWITCH_ON); /* 重复命令不重复推进。 */
    assert(CIA402_UpdateState(&drive, 0x0007, true));
    assert(drive == CIA402_SWITCHED_ON);
    assert(CIA402_UpdateState(&drive, 0x000F, false));
    assert(drive == CIA402_SWITCHED_ON);
    assert(CIA402_UpdateState(&drive, 0x001F, true)); /* 附加模式位不干扰。 */
    assert(drive == CIA402_OPERATION_ENABLED);
    assert(CIA402_UpdateState(&drive, 0x000F, true));
    assert(drive == CIA402_OPERATION_ENABLED);
    assert(CIA402_UpdateState(&drive, 0x000F, false));
    assert(drive == CIA402_OPERATION_ENABLED); /* 只模拟进入条件，不模拟保护停机。 */
    assert(CIA402_UpdateState(&drive, 0x0007, true));
    assert(drive == CIA402_SWITCHED_ON); /* 已使能时的 Disable Operation。 */
    assert(CIA402_UpdateState(&drive, 0x000E, true));
    assert(drive == CIA402_READY_TO_SWITCH_ON); /* Shutdown 忽略 bit3。 */
    assert(CIA402_UpdateState(&drive, 0x000F, false));
    assert(drive == CIA402_SWITCHED_ON); /* 组合命令只完成接通。 */
    drive = CIA402_READY_TO_SWITCH_ON;
    assert(CIA402_UpdateState(&drive, 0x000F, true));
    assert(drive == CIA402_OPERATION_ENABLED); /* 组合转换 3 + 4。 */
    assert(CIA402_UpdateState(&drive, 0x0006, true));
    assert(drive == CIA402_READY_TO_SWITCH_ON);

    const struct {
        CIA402_DriveState state;
        uint16_t statusword;
    } normal[] = {
        {CIA402_SWITCH_ON_DISABLED, 0x0040},
        {CIA402_READY_TO_SWITCH_ON, 0x0021},
        {CIA402_SWITCHED_ON, 0x0023},
        {CIA402_OPERATION_ENABLED, 0x0027}
    };
    for (size_t i = 0; i < sizeof normal / sizeof normal[0]; ++i) {
        uint16_t statusword = 0xAAAA;
        assert(CIA402_EncodeStatusword(normal[i].state, &statusword));
        assert(statusword == normal[i].statusword);
        assert(CIA402_DecodeStatusword(statusword) == normal[i].state);

        /* 生成的状态字通过原 PDO 编解码，位置与状态分别保留。 */
        EC_TxPDO tx = {.statusword = statusword, .actual_position = 300};
        EC_TxPDO received = {0};
        uint8_t bytes[EC_TXPDO_SIZE];
        EC_PackTxPDO(bytes, &tx);
        EC_UnpackTxPDO(&received, bytes);
        assert(received.statusword == normal[i].statusword);
        assert(received.actual_position == 300);

        const uint16_t voltage_off[] = {0x0000, 0x0005, 0x000D, 0x0010};
        for (size_t j = 0; j < sizeof voltage_off / sizeof voltage_off[0]; ++j) {
            drive = normal[i].state;
            assert(CIA402_UpdateState(&drive, voltage_off[j], true));
            assert(drive == CIA402_SWITCH_ON_DISABLED);
        }
        const uint16_t unsupported[] = {0x0080, 0x008F}; /* 第二步已支持 Quick Stop。 */
        for (size_t j = 0; j < sizeof unsupported / sizeof unsupported[0]; ++j) {
            drive = normal[i].state;
            assert(!CIA402_UpdateState(&drive, unsupported[j], true));
            assert(drive == normal[i].state);
        }
    }
    const CIA402_DriveState unsupported_states[] = {
        CIA402_NOT_READY_TO_SWITCH_ON,
        CIA402_FAULT_REACTION_ACTIVE, CIA402_FAULT, CIA402_UNKNOWN
    };
    for (size_t i = 0; i < sizeof unsupported_states / sizeof unsupported_states[0]; ++i) {
        drive = unsupported_states[i];
        uint16_t unchanged = 0xAAAA;
        assert(!CIA402_UpdateState(&drive, 0x0000, true));
        assert(drive == unsupported_states[i]);
        if (drive == CIA402_FAULT_REACTION_ACTIVE || drive == CIA402_FAULT) {
            /* 第三步新增故障编码，但普通 UpdateState 仍不处理故障。 */
            assert(CIA402_EncodeStatusword(drive, &unchanged));
            assert(CIA402_DecodeStatusword(unchanged) == drive);
        } else {
            assert(!CIA402_EncodeStatusword(drive, &unchanged));
            assert(unchanged == 0xAAAA);
        }
    }
    assert(!CIA402_UpdateState(NULL, 0x0006, true));
    assert(!CIA402_EncodeStatusword(CIA402_SWITCHED_ON, NULL));
    puts("Lesson 11 checks passed: sequencing, entry condition, combined command, reverse paths, masks, rejected inputs, PDO feedback");
    return 0;
}
