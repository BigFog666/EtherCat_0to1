#include <inttypes.h> /* 提供 PRId32，用于按正确格式打印 int32_t。 */
#include <stdio.h>    /* 提供 printf、puts 等打印函数。 */

#include "EtherCAT/ethercat_state.h"
#include "EtherCAT/ethercat_pdo.h"
#include "EtherCAT/ethercat_od.h" /* 第七课新增：按索引访问从站变量。 */
#include "EtherCAT/ethercat_sdo.h" /* 第八课新增：通过模拟邮箱请求读写对象。 */

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

    /* 演示 2：主站请求把目标位置写为 5000。
     * 小练习：只把下面的 5000 改成 6000，再编译运行。
     * 写响应确认操作结果，不要求把写入值再回传一次。 */
    request.service = EC_SDO_DOWNLOAD;
    request.value = 5000;
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
    return 0;
}
