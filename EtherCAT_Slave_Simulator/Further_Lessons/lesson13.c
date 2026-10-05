#include <inttypes.h>
#include <stdio.h>
#include "joint.h"
#include "watchdog.h"
#include "../CiA402/cia402_fault.h"
#include "../CiA402/cia402_state.h"
#include "../EtherCAT/ethercat_state.h"

int main(void)
{
    puts("Lesson 13: virtual 1 ms cycles, NOT a measured real-time task");
    uint32_t lesson13_timeout_ms = 3; /* 练习：改成 5，预测第 6 步是否会超时。 */
    JointModel joint;
    ProcessWatchdog watchdog;
    if (!Joint_Init(&joint, 300, 100000) || !Watchdog_Init(&watchdog, 0, lesson13_timeout_ms)) return 1;
    EtherCAT_State communication = EC_STATE_INIT;
    if (!EC_RequestState(&communication, EC_STATE_PREOP) ||
        !EC_RequestState(&communication, EC_STATE_SAFEOP) ||
        !EC_RequestState(&communication, EC_STATE_OP)) return 1;
    CIA402_DriveState drive = CIA402_SWITCH_ON_DISABLED;
    bool previous_reset_bit = false;
    bool communication_stop_latched = false;
    unsigned int accepted = 0, missing = 0, rejected = 0, trips = 0, published = 0;
    ModeCommand cached = {.position = {.controlword = 0, .target_position = 1000}, .mode = JOINT_CSP};

    /* 逻辑时刻 0～12 ms。循环不等待墙钟，不声称 Windows 实际每毫秒运行。
     * 4～6 没有新报文；9 收到错误长度。其余报文逐步使能并保持目标。 */
    for (uint32_t now_ms = 0; now_ms <= 12; ++now_ms) {
        ModeCommand master = {.position = {.controlword = now_ms == 0 ? 0x0006 :
                                 (now_ms == 1 ? 0x0007 : 0x000F), .target_position = 1000},
                                 .mode = JOINT_CSP};
        uint8_t rx_bytes[MODE_PDO_SIZE], tx_bytes[MODE_PDO_SIZE];
        if (!ModePDO_PackRx(rx_bytes, sizeof rx_bytes, &master)) return 1;
        bool arrived = now_ms < 4 || now_ms > 6;
        bool valid_pdo = false;
        if (arrived) {
            size_t length = now_ms == 9 ? MODE_PDO_SIZE - 1 : MODE_PDO_SIZE;
            valid_pdo = ModePDO_UnpackRx(&cached, rx_bytes, length);
            if (valid_pdo) ++accepted; else ++rejected;
        } else {
            ++missing;
        }

        /* 第 1 步：接收并验证之后、任何运动之前检查超时。
         * 没收到或长度错误时，既不更新缓存，也不刷新看门狗。 */
        bool expired;
        if (!Watchdog_Update(&watchdog, now_ms, valid_pdo, &expired)) return 1;
        if (expired && !communication_stop_latched) {
            communication_stop_latched = true;
            ++trips;
        }
        if (communication_stop_latched) {
            /* 旧状态模块尚未实现 OP->SAFEOP 向下请求，这里直接设置教学状态。
             * 不是在真实 ESC 写 AL 状态；恢复流程另行授权/设计，不自动重启。 */
            communication = EC_STATE_SAFEOP;
        }

        /* 第 2 步：故障优先。非法模式视为本地原因；本模型故障动作下一轮完成。
         * 持续使用上一次控制字时复位历史也持续，不制造新的复位沿。 */
        bool supported_mode = Joint_ModeSupported(cached.mode);
        if (!CIA402_ProcessFault(&drive, cached.position.controlword, &previous_reset_bit,
                                 !supported_mode, drive == CIA402_FAULT_REACTION_ACTIVE)) return 1;
        bool fault_state = drive == CIA402_FAULT_REACTION_ACTIVE || drive == CIA402_FAULT;
        if (communication_stop_latched && !fault_state) {
            drive = CIA402_SWITCH_ON_DISABLED; /* 已有故障锁存不被超时覆盖。 */
        } else if (communication == EC_STATE_OP && !fault_state &&
                   (cached.position.controlword & CIA402_CW_FAULT_RESET) == 0u &&
                   !CIA402_UpdateState(&drive, cached.position.controlword, true)) {
            return 1;
        }

        /* 第 3 步：通信许可和驱动许可都成立才允许模型运动。
         * 未超时时短暂丢包保持最后目标；超时当轮立即禁止更新位置。
         * 此模型停用时清零速度和扭矩，不模拟真实机械制动。 */
        bool allow_motion = communication == EC_STATE_OP && !communication_stop_latched &&
                            drive == CIA402_OPERATION_ENABLED && supported_mode;
        int64_t before_position = joint.position_milli;
        if (supported_mode) {
            if (!Joint_Step(&joint, &cached, allow_motion, 1)) return 1;
        } else {
            Joint_Stop(&joint);
        }
        if (!allow_motion && (joint.position_milli != before_position ||
                              joint.velocity != 0 || joint.torque != 0)) return 1;

        /* 第 4 步：仍然发布反馈，即使运动不允许。
         * 在真实设备还要由 ESC/AL 状态决定哪些通信通道可用。 */
        uint16_t statusword;
        ModeFeedback slave, master_feedback;
        if (!CIA402_EncodeStatusword(drive, &statusword) ||
            !Joint_ReadFeedback(&joint, statusword, &slave) ||
            !ModePDO_PackTx(tx_bytes, sizeof tx_bytes, &slave) ||
            !ModePDO_UnpackTx(&master_feedback, tx_bytes, sizeof tx_bytes)) return 1;
        ++published;
        printf("t=%" PRIu32 ", valid=%u, expired=%u, latched=%u, EC=%s, drive=%s, move=%u, position=%" PRId32 "\n",
               now_ms, (unsigned int)valid_pdo, (unsigned int)expired,
               (unsigned int)communication_stop_latched, EC_StateName(communication),
               CIA402_StateName(CIA402_DecodeStatusword(master_feedback.position.statusword)),
               (unsigned int)allow_motion, master_feedback.position.actual_position);
    }
    printf("Diagnostics: accepted=%u, missing=%u, rejected=%u, watchdog_trips=%u, published=%u\n",
           accepted, missing, rejected, trips, published);
    return 0;
}
