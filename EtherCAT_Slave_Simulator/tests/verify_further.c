/* 后续课程的协议字节、运动边界和权限验证，不改课程示例数值。 */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "../Further_Lessons/joint.h"

int main(void)
{
    uint8_t bytes[MODE_PDO_SIZE];
    ModeCommand sent = {.position = {.controlword = 0x000F, .target_position = INT32_MIN},
                        .mode = -128, .target_velocity = INT32_MAX, .target_torque = INT16_MIN};
    ModeCommand received = {0};
    assert(ModePDO_PackRx(bytes, sizeof bytes, &sent));
    assert(bytes[6] == 0x80 && bytes[11] == 0 && bytes[12] == 0x80);
    assert(ModePDO_UnpackRx(&received, bytes, sizeof bytes));
    assert(received.position.target_position == INT32_MIN && received.mode == -128);
    assert(received.target_velocity == INT32_MAX && received.target_torque == INT16_MIN);
    assert(!ModePDO_UnpackRx(&received, bytes, sizeof bytes - 1));
    assert(received.position.target_position == INT32_MIN);
    memset(bytes, 0xAA, sizeof bytes);
    assert(!ModePDO_PackRx(bytes, sizeof bytes - 1, &sent));
    for (size_t i = 0; i < sizeof bytes; ++i) assert(bytes[i] == 0xAA);
    ModeFeedback tx = {.position = {.statusword = 0x0027, .actual_position = INT32_MAX},
                       .mode_display = 10, .actual_velocity = INT32_MIN, .actual_torque = INT16_MAX};
    ModeFeedback rx = {0};
    assert(ModePDO_PackTx(bytes, sizeof bytes, &tx));
    assert(ModePDO_UnpackTx(&rx, bytes, sizeof bytes));
    assert(rx.position.actual_position == INT32_MAX && rx.position.statusword == 0x0027);
    assert(rx.actual_velocity == INT32_MIN && rx.actual_torque == INT16_MAX && rx.mode_display == 10);
    assert(!ModePDO_PackTx(NULL, sizeof bytes, &tx));
    assert(!ModePDO_UnpackTx(NULL, bytes, sizeof bytes));

    JointModel joint;
    assert(!Joint_Init(&joint, 0, 0));
    assert(Joint_Init(&joint, 300, 100000));
    ModeCommand command = {.position = {.target_position = 350}, .mode = JOINT_CSP};
    assert(Joint_Step(&joint, &command, true, 1));
    assert(joint.position_milli == 350000); /* 距离小于最大步长，不越过目标。 */
    assert(Joint_Step(&joint, &command, true, 1));
    assert(joint.velocity == 0);
    command.position.target_position = -10;
    for (int i = 0; i < 5; ++i) assert(Joint_Step(&joint, &command, true, 1));
    assert(joint.position_milli == -10000 && joint.velocity == 0);
    command.mode = JOINT_CSV;
    command.target_velocity = 500;
    assert(Joint_Init(&joint, 0, 100000));
    assert(Joint_Step(&joint, &command, true, 1));
    assert(joint.position_milli == 500); /* 保留半个计数。 */
    assert(Joint_Step(&joint, &command, true, 1));
    assert(joint.position_milli == 1000);
    command.target_velocity = INT32_MIN;
    assert(Joint_Step(&joint, &command, true, 1));
    assert(joint.velocity == -100000); /* 不对 INT32_MIN 直接取负或 abs。 */
    int64_t stopped_position = joint.position_milli;
    assert(Joint_Step(&joint, &command, false, 1));
    assert(joint.position_milli == stopped_position && joint.velocity == 0 && joint.torque == 0);
    assert(Joint_Init(&joint, 0, 150));
    command.mode = JOINT_CST;
    command.target_torque = 100;
    assert(Joint_Step(&joint, &command, true, 1));
    assert(joint.velocity == 100 && joint.torque == 100);
    assert(Joint_Step(&joint, &command, true, 1));
    assert(joint.velocity == 150); /* 扭矩积分仍受模拟速度限幅。 */
    int64_t unchanged = joint.position_milli;
    command.mode = 7;
    assert(!Joint_Step(&joint, &command, true, 1));
    assert(joint.position_milli == unchanged);
    command.mode = JOINT_CSV;
    assert(!Joint_Step(&joint, &command, true, 0));
    assert(Joint_Init(&joint, INT32_MAX, 1000));
    command.target_velocity = 1000;
    assert(!Joint_Step(&joint, &command, true, 1));
    assert(joint.position_milli == (int64_t)INT32_MAX * 1000);
    assert(Joint_ReadFeedback(&joint, 0x0040, &rx));
    assert(rx.position.actual_position == INT32_MAX);
    puts("Further checks passed: 13-byte PDO, signed boundaries, rejected lengths, CSP no overshoot, CSV fractions, CST limit, motion gate, overflow");
    return 0;
}
