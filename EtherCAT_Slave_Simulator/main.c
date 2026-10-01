#include <inttypes.h> /* 提供 PRId32，用于按正确格式打印 int32_t。 */
#include <stdio.h>    /* 提供 printf、puts 等打印函数。 */

#include "EtherCAT/ethercat_state.h"
#include "EtherCAT/ethercat_pdo.h"

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

    return 0;
}
