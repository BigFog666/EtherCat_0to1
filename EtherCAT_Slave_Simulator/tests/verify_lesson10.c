/* 课堂可先跳过：固定反馈样例验证状态识别，特别检查不能只看 bit2。
 * 测试使用自己的 PDO 变量，不改学习者 main 中的练习。 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../CiA402/cia402.h"
#include "../EtherCAT/ethercat_pdo.h"

int main(void)
{
    const struct {
        uint16_t statusword;
        CIA402_DriveState expected;
    } samples[] = {
        {0x0000, CIA402_NOT_READY_TO_SWITCH_ON},
        {0x0020, CIA402_NOT_READY_TO_SWITCH_ON},
        {0x0040, CIA402_SWITCH_ON_DISABLED},
        {0x0070, CIA402_SWITCH_ON_DISABLED},
        {0x0021, CIA402_READY_TO_SWITCH_ON},
        {0x0231, CIA402_READY_TO_SWITCH_ON},
        {0x0023, CIA402_SWITCHED_ON},
        {0x1233, CIA402_SWITCHED_ON},
        {0x0027, CIA402_OPERATION_ENABLED},
        {0x0037, CIA402_OPERATION_ENABLED},
        {0x00A7, CIA402_OPERATION_ENABLED},
        {0x1237, CIA402_OPERATION_ENABLED},
        {0x0007, CIA402_QUICK_STOP_ACTIVE},
        {0x0017, CIA402_QUICK_STOP_ACTIVE},
        {0x000F, CIA402_FAULT_REACTION_ACTIVE},
        {0x003F, CIA402_FAULT_REACTION_ACTIVE},
        {0x0008, CIA402_FAULT},
        {0x0218, CIA402_FAULT},
        {0x0005, CIA402_UNKNOWN},
        {0x0067, CIA402_UNKNOWN},
        {0x0048, CIA402_UNKNOWN},
        {0xFFFF, CIA402_UNKNOWN}
    };
    for (size_t i = 0; i < sizeof samples / sizeof samples[0]; ++i) {
        EC_TxPDO feedback = {.statusword = samples[i].statusword, .actual_position = 300};
        EC_TxPDO received = {0};
        uint8_t bytes[EC_TXPDO_SIZE] = {0};
        EC_PackTxPDO(bytes, &feedback);
        EC_UnpackTxPDO(&received, bytes);
        assert(received.statusword == samples[i].statusword);
        assert(received.actual_position == 300);
        assert(CIA402_DecodeStatusword(received.statusword) == samples[i].expected);
        assert(CIA402_IsOperationEnabled(received.statusword) ==
               (samples[i].expected == CIA402_OPERATION_ENABLED));
    }
    assert(strcmp(CIA402_StateName((CIA402_DriveState)99), "UNKNOWN") == 0);
    assert(CIA402_ControlRequestsEnableOperation(0x000F));
    assert(CIA402_ControlRequestsEnableOperation(0x001F)); /* 附加模式位。 */
    assert(!CIA402_ControlRequestsEnableOperation(0x008F)); /* 故障复位标志。 */
    assert(!CIA402_ControlRequestsEnableOperation(0x0006));
    assert(!CIA402_ControlRequestsEnableOperation(0x0007));
    assert(!CIA402_ControlRequestsEnableOperation(0x0002));
    assert(!CIA402_ControlRequestsEnableOperation(0x0000));

    EC_RxPDO command = {.controlword = 0x000F, .target_position = 10000};
    uint8_t command_bytes[EC_RXPDO_SIZE] = {0};
    const uint8_t expected_bytes[EC_RXPDO_SIZE] = {0x0F, 0, 0x10, 0x27, 0, 0};
    EC_PackRxPDO(command_bytes, &command);
    assert(memcmp(command_bytes, expected_bytes, EC_RXPDO_SIZE) == 0);
    /* 发出了使能命令，同时收到禁用反馈：不能把请求当成结果。 */
    assert(CIA402_ControlRequestsEnableOperation(command.controlword));
    assert(!CIA402_IsOperationEnabled(0x0040));
    puts("CiA402 checks passed: 8 states, extra flags, unknown patterns, request vs feedback, PDO bytes");
    return 0;
}
