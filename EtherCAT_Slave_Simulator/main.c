#include <inttypes.h> /* 提供 PRId32，用于按正确格式打印 int32_t。 */
#include <stdio.h>    /* 提供 printf、puts 等打印函数。 */

#include "EtherCAT/ethercat_state.h"
#include "EtherCAT/ethercat_pdo.h"
#include "EtherCAT/ethercat_od.h" /* 第七课新增：按索引访问从站变量。 */
#include "EtherCAT/ethercat_sdo.h" /* 第八课新增：通过模拟邮箱请求读写对象。 */
#include "EtherCAT/ethercat_sdo_wire.h" /* 第二小节：请求和响应的字节布局。 */
#include "EtherCAT/ethercat_sm.h" /* 第九课：本地过程数据区的访问规则。 */
#include "EtherCAT/ethercat_fmmu.h" /* 第九课：逻辑地址换成本地地址。 */
#include "CiA402/cia402.h" /* 第十课：控制字的请求与状态字的报告。 */

/* 辅助观察：按十六进制打印缓冲区，不参与通信或控制。
 * 当前先关注打印出的字节，不需要自己重写这个函数。 */
static void PrintBytes(const char *label, const uint8_t *bytes, size_t count)
{
    printf("%s:", label);
    for (size_t i = 0; i < count; ++i) {
        printf(" %02X", (unsigned int)bytes[i]); /* 例如将字节 3 显示成 03。 */
    }
    putchar('\n');
}

int main(void)
{
    /* 准备阶段：复习第五课。当前状态从 INIT 开始。 */
    EtherCAT_State state = EC_STATE_INIT;
    const EtherCAT_State next_states[] = {
        EC_STATE_PREOP, EC_STATE_SAFEOP, EC_STATE_OP
    };
    /* image 是主站过程映像：outputs 保存命令，inputs 保存反馈。
     * = {0} 将所有成员初始化为 0，避免读取未初始化的数据。 */
    EC_ProcessImage image = {0};
    /* 主站想发送的命令。你已经将目标位置改成了 2000。
     * 控制字暂时保持 0；本课只搬运它，不解释它的各个位。 */
    EC_RxPDO master_command = {.controlword = 0x0000, .target_position = 2000};
    EC_RxPDO slave_command = {0};   /* 从站解包后得到的命令。 */
    EC_TxPDO slave_feedback = {0};  /* 从站准备发送的反馈。 */
    EC_TxPDO master_feedback = {0}; /* 主站解包后得到的反馈。 */

    puts("EtherCAT Slave Simulator Start");
    printf("State: %s\n", EC_StateName(state));
    /* 依次请求 PRE-OP、SAFE-OP、OP；失败时退出程序。
     * 这里只复习状态切换，真实设备的配置检查以后再补充。 */
    for (size_t i = 0; i < sizeof next_states / sizeof next_states[0]; ++i) {
        EtherCAT_State previous = state;
        if (!EC_RequestState(&state, next_states[i])) {
            puts("State request rejected");
            return 1;
        }
        printf("%s -> %s\n", EC_StateName(previous), EC_StateName(state));
    }
    puts("EtherCAT Slave Operational");

    /* 第 1 步：主站打包。
     * 把 master_command 中的两个数放进 image.outputs 的 6 个字节。
     * 当前目标 2000 = 0x000007D0，预期输出：00 00 D0 07 00 00。 */
    EC_PackRxPDO(image.outputs, &master_command);
    PrintBytes("Master outputs / Slave RxPDO", image.outputs, EC_RXPDO_SIZE);

    /* 第 2 步：从站解包。
     * 从 image.outputs 中取出字节，还原成 slave_command 中的两个数。
     * 此处直接访问数组，代替真实的网络传输、ESC 和 PDI 读取。
     * OP 仅表示通信状态就绪，不代表电机已经使能。 */
    if (state == EC_STATE_OP) {
        EC_UnpackRxPDO(&slave_command, image.outputs);
        printf("Slave received: controlword=0x%04X, target_position=%" PRId32 "\n",
               (unsigned int)slave_command.controlword, slave_command.target_position);
    }

    /* 第 3 步：从站准备反馈并打包。
     * 练习：把下一行的 0 改成 300，假装编码器测到了位置 300。
     * 实际位置来自反馈，不应因为目标是 2000 就直接写成 2000。
     * 状态字仍为初始化的 0；CiA402 状态机和电机模型还没有实现。 */
    slave_feedback.actual_position = 300;
    EC_PackTxPDO(image.inputs, &slave_feedback);
    PrintBytes("Slave TxPDO / Master inputs", image.inputs, EC_TXPDO_SIZE);

    /* 第 4 步：主站解包反馈。
     * 从 image.inputs 取出字节，得到从站报告的状态字和实际位置。
     * 到这里，一次「发送命令 -> 接收反馈」的演示完成。 */
    EC_UnpackTxPDO(&master_feedback, image.inputs);
    printf("Master received: statusword=0x%04X, actual_position=%" PRId32 "\n",
           (unsigned int)master_feedback.statusword, master_feedback.actual_position);

    /* 第七课从这里开始。上面的第六课流程保留，方便对照。
     * 这张字典属于从站，关联的是 slave_command 和 slave_feedback。
     * & 取变量地址；字典记住这些地址，就能找到原来的变量。 */
    EC_ObjectDictionary od = {0};
    int32_t od_value = 0; /* 临时接收读取结果，不是字典的独立存储。 */
    if (EC_OD_Init(&od, &slave_command.target_position,
                   &slave_feedback.actual_position) != EC_OD_OK) {
        return 1;
    }
    puts("\nLesson 7: Object Dictionary");

    /* 演示 1：不用成员名，改为凭 0x607A:00 找到目标位置并读取。 */
    if (EC_OD_ReadI32(&od, 0x607A, 0, &od_value) != EC_OD_OK) {
        return 1;
    }
    printf("OD read 0x607A:00: %" PRId32 "\n", od_value);

    /* 演示 2：通过字典读实际位置，读到的是你设置的模拟编码器值。 */
    if (EC_OD_ReadI32(&od, 0x6064, 0, &od_value) != EC_OD_OK) {
        return 1;
    }
    printf("OD read 0x6064:00: %" PRId32 "\n", od_value);

    /* 演示 3：通过本地字典接口修改目标位置。
     * 你已完成练习，将 2500 改为 4000，下面直接打印的原变量也会改变。
     * 这里是本地函数测试，尚未通过主站、Mailbox 或 SDO 发送写请求。 */
    if (EC_OD_WriteI32(&od, 0x607A, 0, 4000) != EC_OD_OK) {
        return 1;
    }
    printf("OD write 0x607A:00: slave target_position=%" PRId32 "\n",
           slave_command.target_position);

    /* 演示 4：尝试把实际位置写成 999，预期被权限检查拒绝。
     * 先检查返回结果，再确认原来的实际位置仍然保留。 */
    EC_OD_Result result = EC_OD_WriteI32(&od, 0x6064, 0, 999);
    printf("OD write 0x6064:00: %s, actual_position=%" PRId32 "\n",
           EC_OD_ResultName(result), slave_feedback.actual_position);
    if (result != EC_OD_READ_ONLY) {
        return 1;
    }

    /* 第八课从这里开始。前面你完成的 4000 练习保持不变。
     * 先只看请求中的 service、index、subindex、value 四个字段。
     * 邮箱用 {0} 初始化，起始状态为 EMPTY。 */
    EC_Mailbox mailbox = {0};
    EC_SDO_Request request = {
        .service = EC_SDO_UPLOAD,
        .index = 0x607A,
        .subindex = 0,
        .value = 0
    };
    EC_SDO_Response response = {0};
    puts("\nLesson 8: SDO / Mailbox semantic simulator");

    /* 演示 1：主站请求读取目标位置。
     * Transfer 内部是发送请求 -> 从站处理 -> 取回响应。
     * true 表示收到响应，result == OK 才表示对象读取成功。 */
    if (!EC_SDO_Transfer(&mailbox, &od, &request, &response) ||
        response.result != EC_OD_OK) {
        return 1;
    }
    printf("SDO Upload 0x607A:00: value=%" PRId32 "\n", response.value);

    /* 演示 2：主站请求修改目标位置。
     * 你已将开始示例中的 5000 改为 6000，写入和读回输出都会改变。
     * 写响应确认操作结果，不要求把写入值再回传一次。 */
    request.service = EC_SDO_DOWNLOAD;
    request.value = 6000;
    if (!EC_SDO_Transfer(&mailbox, &od, &request, &response) ||
        response.result != EC_OD_OK) {
        return 1;
    }
    printf("SDO Download 0x607A:00: OK, slave target_position=%" PRId32 "\n",
           slave_command.target_position);

    /* 演示 3：再发一次读取请求，验证从站保存的是新目标位置。
     * Upload 不使用 request.value，为便于理解仍将它清零。 */
    request.service = EC_SDO_UPLOAD;
    request.value = 0;
    if (!EC_SDO_Transfer(&mailbox, &od, &request, &response) ||
        response.result != EC_OD_OK) {
        return 1;
    }
    printf("SDO Upload 0x607A:00 after write: value=%" PRId32 "\n", response.value);

    /* 演示 4：请求写入只读的实际位置。
     * 邮箱可以正常返回失败响应；实际位置仍保持 300。 */
    request.service = EC_SDO_DOWNLOAD;
    request.index = 0x6064;
    request.value = 999;
    if (!EC_SDO_Transfer(&mailbox, &od, &request, &response)) {
        return 1;
    }
    printf("SDO Download 0x6064:00: %s, actual_position=%" PRId32 "\n",
           EC_OD_ResultName(response.result), slave_feedback.actual_position);
    if (response.result != EC_OD_READ_ONLY) {
        return 1;
    }

    /* 第八课第二小节从这里开始。第一小节的 6000 练习完整保留。
     * 先看终端的 8 个字节，再对照课程中的偏移表。
     * 这里只有 SDO 内容，还没有外层 CoE/Mailbox 头或真实网卡收发。 */
    EC_SDO_Frame wire_request = {0};
    EC_SDO_Frame wire_response = {0};
    int32_t wire_value = 0;
    puts("\nLesson 8 part 2: 8-byte expedited SDO content");

    /* 演示 1：把读请求编码成 40 7A 60 00 00 00 00 00。
     * 从站解析、访问同一张字典，响应中应带回当前目标 6000。 */
    if (!EC_SDO_BuildUpload(&wire_request, 0x607A, 0)) {
        return 1;
    }
    PrintBytes("Wire Upload request", wire_request.bytes, EC_SDO_FRAME_SIZE);
    if (!EC_SDO_ProcessFrame(&mailbox, &od, &wire_request, &wire_response)) {
        return 1;
    }
    PrintBytes("Wire Upload response", wire_response.bytes, EC_SDO_FRAME_SIZE);
    if (!EC_SDO_GetUploadI32(&wire_response, 0x607A, 0, &wire_value)) {
        return 1;
    }
    printf("Wire Upload value=%" PRId32 "\n", wire_value);

    /* 演示 2：通过字节请求把目标位置写成 7000。
     * 本节练习只修改下面的 7000，例如改为 8000，再观察数据四字节。
     * 7000 = 0x00001B58，数据顺序为 58 1B 00 00。 */
    int32_t wire_target = 7000;
    if (!EC_SDO_BuildDownloadI32(&wire_request, 0x607A, 0, wire_target)) {
        return 1;
    }
    PrintBytes("Wire Download request", wire_request.bytes, EC_SDO_FRAME_SIZE);
    if (!EC_SDO_ProcessFrame(&mailbox, &od, &wire_request, &wire_response)) {
        return 1;
    }
    PrintBytes("Wire Download response", wire_response.bytes, EC_SDO_FRAME_SIZE);
    if (wire_response.bytes[0] != EC_SDO_CMD_DOWNLOAD_ACK ||
        slave_command.target_position != wire_target) {
        return 1;
    }

    /* 演示 3：再用字节请求读取目标，确认读回 7000。 */
    if (!EC_SDO_BuildUpload(&wire_request, 0x607A, 0) ||
        !EC_SDO_ProcessFrame(&mailbox, &od, &wire_request, &wire_response) ||
        !EC_SDO_GetUploadI32(&wire_response, 0x607A, 0, &wire_value)) {
        return 1;
    }
    PrintBytes("Wire Upload after write", wire_response.bytes, EC_SDO_FRAME_SIZE);
    printf("Wire Upload after write value=%" PRId32 "\n", wire_value);

    /* 演示 4：只读拒绝现在变成标准 Abort 响应。
     * 响应开头为 80，最后四字节编码错误码 0x06010002。 */
    uint32_t abort_code = 0;
    if (!EC_SDO_BuildDownloadI32(&wire_request, 0x6064, 0, 999) ||
        !EC_SDO_ProcessFrame(&mailbox, &od, &wire_request, &wire_response) ||
        !EC_SDO_GetAbortCode(&wire_response, &abort_code)) {
        return 1;
    }
    PrintBytes("Wire read-only Abort", wire_response.bytes, EC_SDO_FRAME_SIZE);
    printf("Abort code=0x%08" PRIX32 ", actual_position=%" PRId32 "\n",
           abort_code, slave_feedback.actual_position);
    if (abort_code != EC_SDO_ABORT_READ_ONLY) {
        return 1;
    }

    /* 第九课从这里开始。前面各课的练习及输出保持原样。
     * 第六课直接把 image.outputs 给从站解包；这次中间增加
     * FMMU 地址换算 -> SM 管理的本地数据区 -> PDI 读取。
     * 这里只演示已经配置好的 OP 数据交换，不模拟真实启动配置。 */
    puts("\nLesson 9: FMMU / SyncManager process data path");
    EC_SyncManager sm2 = {0}; /* 本例选择 SM2 管命令，SM3 管反馈。 */
    EC_SyncManager sm3 = {0};
    if (EC_SM_Init(&sm2, 0x1020, EC_SM_RXPDO) != EC_SM_OK ||
        EC_SM_Init(&sm3, 0x1010, EC_SM_TXPDO) != EC_SM_OK) {
        return 1;
    }
    EC_FMMU rx_fmmu = {
        .logical_start = 0x00000000, .physical_start = 0x1020,
        .length = EC_RXPDO_SIZE, .enabled = true,
        .read_enabled = false, .write_enabled = true
    };
    EC_FMMU tx_fmmu = {
        .logical_start = 0x00000010, .physical_start = 0x1010,
        .length = EC_TXPDO_SIZE, .enabled = true,
        .read_enabled = true, .write_enabled = false
    };
    uint16_t physical_address = 0; /* 接收 FMMU 换算得到的本地地址。 */
    uint8_t pdi_rx[EC_RXPDO_SIZE] = {0}; /* 应用从本地数据区取出的副本。 */
    uint8_t pdi_tx[EC_TXPDO_SIZE] = {0}; /* 应用准备交给本地数据区的反馈。 */

    /* 第 1 步：主站仍用原打包函数。你已将本阶段目标改为 10000。
     * 只改变后面的新演示，不覆盖前面各课的 2000/4000/6000/7000。 */
    master_command.target_position = 10000;
    EC_PackRxPDO(image.outputs, &master_command);
    PrintBytes("Mapped master outputs", image.outputs, EC_RXPDO_SIZE);

    /* 第 2 步：EtherCAT 侧写逻辑地址 0，FMMU 换算成你的新地址 0x1020。
     * Translate 成功只表示地址匹配；随后 SM_Write 才复制数据。
     * 真实硬件中由 ESC 完成；本程序用两个调用展示职责。 */
    if (!EC_FMMU_Translate(&rx_fmmu, EC_FMMU_WRITE, 0x00000000,
                           EC_RXPDO_SIZE, &physical_address) ||
        EC_SM_Write(&sm2, EC_SM_ETHERCAT_SIDE, physical_address,
                    image.outputs, EC_RXPDO_SIZE) != EC_SM_OK) {
        return 1;
    }
    printf("Rx mapping: logical=0x00000000 -> physical=0x%04X, length=6\n",
           (unsigned int)physical_address);

    /* 第 3 步：从站应用经 PDI 侧读 SM2，再解包为原来的命令变量。
     * 目标位置更新发生在 Unpack，不是仅凭写 DPRAM 就自动更新。
     * 字典一直指向此变量，因此也能读到新的 10000。 */
    if (EC_SM_Read(&sm2, EC_SM_PDI_SIDE, sm2.physical_start,
                   pdi_rx, EC_RXPDO_SIZE) != EC_SM_OK) {
        return 1;
    }
    EC_UnpackRxPDO(&slave_command, pdi_rx);
    printf("PDI RxPDO: slave target_position=%" PRId32 "\n",
           slave_command.target_position);
    if (EC_OD_ReadI32(&od, 0x607A, 0, &od_value) != EC_OD_OK) {
        return 1;
    }
    printf("OD sees mapped target=%" PRId32 "\n", od_value);

    /* 第 4 步：反馈沿反方向返回。实际位置仍使用你的 300。
     * 应用打包 -> PDI 写 SM3 -> 主站逻辑读映射到 0x1010 -> 解包。
     * 与 Rx 相反，这次 PDI 侧是生产者，EtherCAT 侧是消费者。 */
    EC_PackTxPDO(pdi_tx, &slave_feedback);
    if (EC_SM_Write(&sm3, EC_SM_PDI_SIDE, sm3.physical_start,
                    pdi_tx, EC_TXPDO_SIZE) != EC_SM_OK ||
        !EC_FMMU_Translate(&tx_fmmu, EC_FMMU_READ, 0x00000010,
                           EC_TXPDO_SIZE, &physical_address) ||
        EC_SM_Read(&sm3, EC_SM_ETHERCAT_SIDE, physical_address,
                   image.inputs, EC_TXPDO_SIZE) != EC_SM_OK) {
        return 1;
    }
    printf("Tx mapping: logical=0x00000010 -> physical=0x%04X, length=6\n",
           (unsigned int)physical_address);
    PrintBytes("Mapped master inputs", image.inputs, EC_TXPDO_SIZE);
    EC_UnpackTxPDO(&master_feedback, image.inputs);
    printf("Mapped feedback: actual_position=%" PRId32 "\n",
           master_feedback.actual_position);

    /* 第 5 步：两个有意构造的失败，只检查，不改原数据。
     * 0x20 不在 Rx 映射区间内；PDI 侧也不能向 Rx 通道写命令。 */
    bool matched = EC_FMMU_Translate(&rx_fmmu, EC_FMMU_WRITE, 0x00000020,
                                    EC_RXPDO_SIZE, &physical_address);
    printf("Unmapped logical 0x00000020: %s\n", matched ? "MATCHED" : "REJECTED");
    EC_SM_Result wrong_side = EC_SM_Write(&sm2, EC_SM_PDI_SIDE,
                                          sm2.physical_start, image.outputs,
                                          EC_RXPDO_SIZE);
    printf("PDI write to Rx SM2: %s, slave target_position=%" PRId32 "\n",
           EC_SM_ResultName(wrong_side), slave_command.target_position);
    if (matched || wrong_side != EC_SM_WRONG_DIRECTION) {
        return 1;
    }

    /* 第十课从这里开始。先解释两个已有的 16 位 PDO 字段。
     * 此处继续使用你的目标 10000、Rx 本地起点 0x1020、反馈 300。
     * 我们发送一份控制字，然后观察三份人为提供的反馈样例。
     * 样例不是根据控制字自动生成的，真正状态转换留到第十一课。 */
    puts("\nLesson 10: CiA402 Controlword / Statusword basics");
    printf("Control commands: Shutdown=0x%04X, SwitchOn=0x%04X, EnableOperation=0x%04X\n",
           (unsigned int)CIA402_CW_SHUTDOWN, (unsigned int)CIA402_CW_SWITCH_ON,
           (unsigned int)CIA402_CW_ENABLE_OPERATION);

    /* 第 1 步：主站请求使能运行。0x000F 的 bit3～0 都为 1。
     * 控制字是命令，不把这个数直接复制成状态字。 */
    master_command.controlword = CIA402_CW_ENABLE_OPERATION;
    EC_PackRxPDO(image.outputs, &master_command);
    PrintBytes("CiA402 RxPDO", image.outputs, EC_RXPDO_SIZE);

    /* 第 2 步：走已经学会的数据路径，将命令送到从站应用。
     * 逻辑起点 0 对应本地 0x1020；控制字放在 PDO 偏移 0～1。
     * 这里读到控制字之后，只解释请求，不改变驱动状态。 */
    if (!EC_FMMU_Translate(&rx_fmmu, EC_FMMU_WRITE, 0x00000000,
                           EC_RXPDO_SIZE, &physical_address) ||
        EC_SM_Write(&sm2, EC_SM_ETHERCAT_SIDE, physical_address,
                    image.outputs, EC_RXPDO_SIZE) != EC_SM_OK ||
        EC_SM_Read(&sm2, EC_SM_PDI_SIDE, sm2.physical_start,
                   pdi_rx, EC_RXPDO_SIZE) != EC_SM_OK) {
        return 1;
    }
    EC_UnpackRxPDO(&slave_command, pdi_rx);
    printf("Slave controlword=0x%04X, requests_enable_operation=%u\n",
           (unsigned int)slave_command.controlword,
           (unsigned int)CIA402_ControlRequestsEnableOperation(slave_command.controlword));

    /* 第 3 步：准备反馈样例。这里只模拟“收到了这些状态报告”。
     * 首份 0x0040 表示 Switch On Disabled，即使命令已请求使能，
     * 主站也必须根据反馈判断当前驱动状态。
     * 练习只改 lesson10_status 为 0x0023，再观察后两份报告。 */
    uint16_t lesson10_status = 0x0023; /* 你完成的练习：Switched On，尚未使能运行。 */
    const uint16_t status_samples[] = {
        0x0040,
        lesson10_status,
        (uint16_t)(lesson10_status | CIA402_SW_WARNING) /* | 给样例加上警告位。 */
    };

    /* 第 4 步：每份状态字都经过反馈 PDO，再由主站解码。
     * Pack/SM/PDI/FMMU 搬运字节；CiA402 解码才解释这些位。
     * 不用整个状态字 == 0x0027 判断，否则带警告位时会误判。
     * printf 中把 bool 转成 unsigned int，打印成 0 或 1。 */
    for (size_t i = 0; i < sizeof status_samples / sizeof status_samples[0]; ++i) {
        slave_feedback.statusword = status_samples[i];
        EC_PackTxPDO(pdi_tx, &slave_feedback);
        if (EC_SM_Write(&sm3, EC_SM_PDI_SIDE, sm3.physical_start,
                        pdi_tx, EC_TXPDO_SIZE) != EC_SM_OK ||
            !EC_FMMU_Translate(&tx_fmmu, EC_FMMU_READ, 0x00000010,
                               EC_TXPDO_SIZE, &physical_address) ||
            EC_SM_Read(&sm3, EC_SM_ETHERCAT_SIDE, physical_address,
                       image.inputs, EC_TXPDO_SIZE) != EC_SM_OK) {
            return 1;
        }
        PrintBytes("CiA402 TxPDO", image.inputs, EC_TXPDO_SIZE);
        EC_UnpackTxPDO(&master_feedback, image.inputs);
        CIA402_DriveState drive_state = CIA402_DecodeStatusword(master_feedback.statusword);
        printf("Master statusword=0x%04X, drive_state=%s, operation_enabled=%u, warning=%u\n",
               (unsigned int)master_feedback.statusword, CIA402_StateName(drive_state),
               (unsigned int)CIA402_IsOperationEnabled(master_feedback.statusword),
               (unsigned int)((master_feedback.statusword & CIA402_SW_WARNING) != 0u));
    }
    printf("Communication state=%s; feedback samples are supplied by the lesson.\n",
           EC_StateName(state));
    return 0;
}
