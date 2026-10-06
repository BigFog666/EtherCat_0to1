#ifndef PROJECT_JOINT_H
#define PROJECT_JOINT_H
#include <stdint.h>
/* 第一版只实现单轴 CSP/CSV 模拟；时间单位为微秒，位置为教学计数。 */
#define JOINT_LIMIT 100000
#define JOINT_MAX_VELOCITY 20000
#define JOINT_TIMEOUT_US 100000u
typedef enum { JD_DISABLED, JD_READY, JD_SWITCHED, JD_ENABLED,
               JD_QUICK_STOP, JD_FAULT_REACTION, JD_FAULT } JointState;
typedef struct {
    uint16_t controlword;
    int32_t target_position;
    int32_t target_velocity;
    int8_t mode;
} JointCommand;
typedef struct {
    uint16_t statusword;
    int32_t position;
    int32_t velocity;
    int8_t mode;
} JointFeedback;
typedef struct {
    JointCommand command;
    JointFeedback feedback;
    JointState state;
    int64_t position_micro;
    uint32_t last_receive_us, last_update_us, receive_count, timeout_count;
    uint16_t error_code;
    uint8_t have_command, previous_reset, timeout_active;
} Joint;
void joint_init(Joint *joint, uint32_t now_us);
void joint_receive(Joint *joint, const JointCommand *command, uint32_t now_us);
void joint_update(Joint *joint, uint32_t now_us, int communication_op);
#endif
