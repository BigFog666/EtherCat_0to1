#include <inttypes.h>
#include <stdio.h>
#include "joint.h"
#include "../CiA402/cia402_state.h"

int main(void)
{
    puts("Lesson 12: independent CSP / CSV / CST model, logical dt=1 ms");
    int32_t lesson12_target_position = 1000; /* 练习：改成 500。不影响旧 main 的 10000。 */
    CIA402_DriveState drive = CIA402_SWITCH_ON_DISABLED;
    const uint16_t enable[] = {0x0006, 0x0007, 0x000F};
    for (size_t i = 0; i < sizeof enable / sizeof enable[0]; ++i) {
        if (!CIA402_UpdateState(&drive, enable[i], true)) return 1;
    }
    uint16_t statusword;
    if (!CIA402_EncodeStatusword(drive, &statusword)) return 1;
    const int8_t modes[] = {JOINT_CSP, JOINT_CSV, JOINT_CST};
    const char *names[] = {"CSP", "CSV", "CST"};
    for (size_t phase = 0; phase < 3; ++phase) {
        JointModel joint;
        if (!Joint_Init(&joint, 300, 100000)) return 1; /* 三个独立场景都从 300 开始。 */
        ModeCommand master = {.position = {.controlword = 0x000F,
                              .target_position = lesson12_target_position},
                              .mode = modes[phase], .target_velocity = 500, .target_torque = 100};
        for (unsigned int tick = 1; tick <= 10; ++tick) {
            uint8_t rx_bytes[MODE_PDO_SIZE], tx_bytes[MODE_PDO_SIZE];
            ModeCommand received;
            ModeFeedback slave, feedback;
            /* 显式经过字节编解码；此独立例子不接旧的固定 6 字节 SM。 */
            if (!ModePDO_PackRx(rx_bytes, sizeof rx_bytes, &master) ||
                !ModePDO_UnpackRx(&received, rx_bytes, sizeof rx_bytes) ||
                !Joint_Step(&joint, &received, drive == CIA402_OPERATION_ENABLED, 1) ||
                !Joint_ReadFeedback(&joint, statusword, &slave) ||
                !ModePDO_PackTx(tx_bytes, sizeof tx_bytes, &slave) ||
                !ModePDO_UnpackTx(&feedback, tx_bytes, sizeof tx_bytes)) return 1;
            printf("%s tick=%u, mode=%d, position=%" PRId32 ", velocity=%" PRId32
                   ", torque=%d, sw=0x%04X\n", names[phase], tick,
                   (int)feedback.mode_display, feedback.position.actual_position,
                   feedback.actual_velocity, (int)feedback.actual_torque, (unsigned int)feedback.position.statusword);
        }
    }
    return 0;
}
